#include <SDL2/SDL.h>
#include "sdl_frontend.h"
#include "sim.h"
#include "utils.h"

#define COL_BG				((SDL_Color){  15,  15,  20, 255 })  // near-black
#define COL_PANEL			((SDL_Color){  25,  28,  36, 255 })  // panel fill
#define COL_BORDER			((SDL_Color){  70,  80, 100, 255 })  // box lines
#define COL_TITLE			((SDL_Color){ 255, 200,  60, 255 })  // gold heading
#define COL_TEXT			((SDL_Color){ 210, 215, 225, 255 })  // normal text
#define COL_DIM				((SDL_Color){ 110, 120, 140, 255 })  // dimmed text
#define COL_HIGHLIGHT		((SDL_Color){  50, 120, 220, 255 })  // selection bg
#define COL_HIGHLIGHT_TXT	((SDL_Color){ 255, 255, 255, 255 })
#define COL_SELECTED		((SDL_Color){  60, 200, 100, 255 })  // "your team" indicator
#define COL_URGENT			((SDL_Color){ 220, 80,	80,	 255 })

const size_t DEFAULT_BUFFER_LEN = 128;
const unsigned int DEFAULT_DELAY = 16;

// offsets needed for spacing out stat columns in the UI properly

bool init_sdl_ctx(
		SDLCtx *ctx,
		const char *font_path,
		const char *font_bold_path,
		int win_width,
		int win_height) {
	memset(ctx, 0, sizeof(*ctx));
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "error on SDL Init: %s\n", SDL_GetError());
		return false;
	}
	fprintf(stdout, "SDL init successful\n");
	if (TTF_Init() != 0) {
		fprintf(stderr, "error on TTF init: %s\n", SDL_GetError());
		return false;
	}
	fprintf(stdout, "Initialized TTF\n");

	ctx->window = SDL_CreateWindow(
			"Baseball Mill",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			win_width, win_height,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
			);
	if (!ctx->window) {
		fprintf(stderr, "error on SDL context init: %s\n", SDL_GetError());
		return false;
	}
	fprintf(stdout, "Created SDL window\n");

	ctx->renderer = SDL_CreateRenderer(
			ctx->window, -1,
			SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
			);
	if (!ctx->renderer) {
		fprintf(stderr, "error on SDL renderer init: %s\n", SDL_GetError());
		return false;
	}
	fprintf(stdout, "Created SDL renderer\n");

	ctx->font = TTF_OpenFont(font_path, 14);
	if (!ctx->font) {
		fprintf(stderr, "error on TTF font init: %s\n", SDL_GetError());
		return false;
	}
	fprintf(stdout, "Opened TTF font: %s\n", font_path);
	ctx->font_bold = font_bold_path ? TTF_OpenFont(font_bold_path, 14) : ctx->font;
	if (!ctx->font_bold) ctx->font_bold = ctx->font;
	TTF_SizeText(ctx->font, "M", &ctx->font_width, &ctx->font_height);

	return true;
}

void destroy_sdl_ctx(SDLCtx *ctx) {
    if (ctx->font_bold && ctx->font_bold != ctx->font)
        TTF_CloseFont(ctx->font_bold);
    if (ctx->font)      TTF_CloseFont(ctx->font);
    if (ctx->renderer)  SDL_DestroyRenderer(ctx->renderer);
    if (ctx->window)    SDL_DestroyWindow(ctx->window);
    TTF_Quit();
    SDL_Quit();
}

// drawing helpers
static void set_color(SDL_Renderer *r, SDL_Color c) {
	SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

static void fill_rect(SDL_Renderer *r, int x, int y, int w, int h, SDL_Color c) {
	set_color(r, c);
	SDL_Rect rect = {x, y, w, h};
	SDL_RenderFillRect(r, &rect);
}

static void draw_border(SDL_Renderer *r, int x, int y, int w, int h, SDL_Color c) {
	set_color(r, c);
	SDL_Rect rect = {x, y, w, h};
	SDL_RenderDrawRect(r, &rect);
}

static int draw_text(SDLCtx *ctx, TTF_Font *font,
					 const char *text, int x, int y,
					 SDL_Color color) {
	if (!text || text[0] == '\0') return 0;
	SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
	if (!surf) return 0;
	SDL_Texture *texture = SDL_CreateTextureFromSurface(ctx->renderer, surf);
	SDL_Rect dst = {x, y, surf->w, surf->h};
	SDL_RenderCopy(ctx->renderer, texture, NULL, &dst);
	int w = surf->w;
	SDL_FreeSurface(surf);
	SDL_DestroyTexture(texture);
	return w;
}

static void draw_panel(SDLCtx *ctx, int x, int y, int w, int h,
                       const char *title) {
    fill_rect(ctx->renderer, x, y, w, h, COL_PANEL);
    draw_border(ctx->renderer, x, y, w, h, COL_BORDER);
    if (title && title[0]) {
        // small gap in border, title sits right after the top-left corner
        int tx = x + ctx->font_width;
        int ty = y - ctx->font_height / 2;
        // blank a strip so the title sits cleanly on the border line
        fill_rect(ctx->renderer, tx - 2, ty,
                  (int)strlen(title) * ctx->font_width + 4, ctx->font_height, COL_PANEL);
        draw_text(ctx, ctx->font_bold, title, tx, ty, COL_TITLE);
    }
}

static void draw_screen_title(SDLCtx *ctx, const char *text, int win_width, int top_y, SDL_Color col) {
	int tw, th;
	TTF_SizeText(ctx->font_bold, text, &tw, &th);
	draw_text(ctx, ctx->font_bold, text, (win_width - tw) / 2, top_y - th, col);
}

static void draw_screen_footer(SDLCtx *ctx, const char *text, int win_width, int win_height, SDL_Color col) {
	int hw, hh;
	TTF_SizeText(ctx->font, text, &hw, &hh); 
	draw_text(ctx, ctx->font, text, (win_width - hw) / 2, win_height - hh, col);
}

static void draw_menu_row(SDLCtx *ctx, int x, int y, int w, const char *text, bool highlighted, bool is_selected) {
	if (highlighted) {
		fill_rect(ctx->renderer, x, y, w, ctx->font_height + 2, COL_HIGHLIGHT);
		draw_text(ctx, ctx->font, text, x + 4, y + 1, COL_HIGHLIGHT_TXT);
	} else {
		SDL_Color text_color = is_selected ? COL_SELECTED : COL_TEXT;
		draw_text(ctx, ctx->font, text, x + 4, y + 1, text_color);
	}
}

static bool panel_hit(Panel p, int panel_x, int panel_y) {
	return panel_x >= p.x && panel_x < p.x + p.w &&
		panel_y >= p.y && panel_y < p.y + p.h;
}

static int panel_row_at(SDLCtx *ctx, Panel p, int panel_y, int n_rows) {
    int content_y = p.y + ctx->font_height + 6; // below title
    int row = (panel_y - content_y) / (ctx->font_height + 2);
    if (row < 0 || row >= n_rows) return -1;
    return row;
}

// core functions
size_t sdl_main_menu(SDLCtx *ctx, const char **options, size_t n_options) {
	if (n_options == 0 || !options) return -1;
	size_t sel = 0;
	bool running = true;

	while (running) {
		int win_width, win_height;
		SDL_GetWindowSize(ctx->window, &win_width, &win_height);

		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_QUIT: exit(0);
				case SDL_KEYDOWN:
					switch(e.key.keysym.sym) {
						case SDLK_DOWN: case SDLK_j:
							sel = (sel+1) % n_options;
							break;
						case SDLK_UP: case SDLK_k:
							sel = (sel == 0) ? n_options - 1 : sel - 1;
							break;
						case SDLK_RETURN: case SDLK_KP_ENTER:
							running = false;
							break;
					}
					break;
				case SDL_MOUSEMOTION: {
					int my = e.motion.y;
					int total_h = (int)n_options * (ctx->font_height + 4);
					int start_y = (win_height - total_h) / 2;
					int row = (my - start_y) / (ctx->font_height + 4);
					if (row >= 0 && (size_t)row < n_options)
						sel = row;
					break;
				}
				case SDL_MOUSEBUTTONDOWN: {
					if (e.button.button == SDL_BUTTON_LEFT) {
						int my = e.button.y;
						int total_h = (int)n_options * (ctx->font_height + 4);
						int start_y = (win_height - total_h) / 2;
						int row = (my - start_y) / (ctx->font_height + 4);
						if (row >= 0 && (size_t)row < n_options) {
							sel = row;
							running = false;
						}
					}
					break;
				}
			}
		}
		set_color(ctx->renderer, COL_BG);
		SDL_RenderClear(ctx->renderer);
		int row_height = ctx->font_height + 4;
		int total_height = (int)n_options * row_height;
		int start_y = (win_height - total_height) / 2;
		int max_width = 0;
		for (int i = 0; i < n_options; i++) {
			int total_width, total_height;
			TTF_SizeText(ctx->font, options[i], &total_width, &total_height);
			if (total_width > max_width) max_width = total_width;
		}

		int panel_width = max_width + 48;
		int panel_x = (win_width - panel_width) / 2;
		int padding = 16;

		draw_panel(ctx, panel_x - padding, start_y - padding,
					panel_width + padding * 2, total_height + padding * 2, "Main Menu");

		for (int i = 0; i < n_options; i++) {
			int row_y = start_y + i * row_height;
			draw_menu_row(ctx, panel_x, row_y, panel_width, options[i], i == sel, false);
		}

		SDL_RenderPresent(ctx->renderer);
		SDL_Delay(DEFAULT_DELAY);
	}

	return sel;
}

const char *sdl_team_select(SDLCtx *ctx, Team **al_teams, size_t n_al, Team **nl_teams, size_t n_nl) {
	if (!al_teams || n_al == 0 || !nl_teams || n_nl == 0) return NULL;
	size_t al_sel = 0;
	size_t nl_sel = 0;
	bool al_focused= true;
	bool running = true;
	const char *result = NULL;

	while (running) {
		int win_width, win_height;
		SDL_GetWindowSize(ctx->window, &win_width, &win_height);

		int panel_width = win_width / 3;
		int panel_height = (int)(win_height * 0.45);
		int panel_y = (win_height - panel_height) / 2;
		int gap = win_width / 16;
		int total_width = panel_width * 2 + gap;
		int left_x = (win_width - total_width) / 2;
		int right_x = left_x + panel_width + gap;

		Panel al_panel = { left_x, panel_y, panel_width, panel_height };
		Panel nl_panel = { right_x, panel_y, panel_width, panel_height };

		int row_height = ctx->font_height + 2;
		int content_y_off = ctx->font_height + 6;

		// SDL events
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch(e.type) {
				case SDL_QUIT: exit(0);
				case SDL_KEYDOWN:
					switch (e.key.keysym.sym) {
						case SDLK_TAB:
						case SDLK_LEFT:  case SDLK_h:
						case SDLK_RIGHT: case SDLK_l:
							al_focused = !al_focused;
							break;
						case SDLK_UP: case SDLK_k:
							if (al_focused) {
								al_sel = (al_sel == 0) ? n_al - 1 : al_sel - 1;
							} else {
								nl_sel = (nl_sel == 0) ? n_nl - 1 : nl_sel - 1;
							}
							break;
						case SDLK_DOWN: case SDLK_j:
							if (al_focused)
								al_sel = (al_sel + 1) % n_al;
							else
								nl_sel = (nl_sel + 1) % n_nl;
							break;
						case SDLK_RETURN: case SDLK_KP_ENTER:
							result = strdup(al_focused ? al_teams[al_sel]->name : nl_teams[nl_sel]->name);
							running = false;
							break;
					}
					break;
				case SDL_MOUSEMOTION: {
					int mx = e.motion.x, my = e.motion.y;
					if (panel_hit(al_panel, mx, my)) {
						int row = panel_row_at(ctx, al_panel, my, (int)n_al);
						if (row >= 0) { al_sel = row; al_focused = true; }
					} else if (panel_hit(nl_panel, mx, my)) {
						int row = panel_row_at(ctx, nl_panel, my, (int)n_nl);
						if (row >= 0) { nl_sel = row; al_focused = false; }
					}
					break;
				}

				case SDL_MOUSEBUTTONDOWN: {
					int mx = e.button.x, my = e.button.y;
					if (e.button.button == SDL_BUTTON_LEFT) {
						if (panel_hit(al_panel, mx, my)) {
							int row = panel_row_at(ctx, al_panel, my, (int)n_al);
							if (row >= 0) {
								if (al_focused && (size_t)row == al_sel) {
									result = strdup(al_teams[al_sel]->name);
									running = false;
								} else {
									al_sel = row; al_focused = true;
								}
							}
						} else if (panel_hit(nl_panel, mx, my)) {
							int row = panel_row_at(ctx, nl_panel, my, (int)n_nl);
							if (row >= 0) {
								if (al_focused && (size_t)row == nl_sel) {
									result = strdup(nl_teams[nl_sel]->name);
									running = false;
								} else {
									nl_sel = row; al_focused = false;
								}
							}
						}
					}
					break;
				}
			}
		}
		set_color(ctx->renderer, COL_BG);
		SDL_RenderClear(ctx->renderer);

		draw_screen_title(ctx, "SELECT YOUR TEAM", win_width, panel_y - 12, COL_TITLE); 
		const char *hint = "arrows/vim keys to move  |  tab to switch league  |  enter or click to confirm";
		draw_screen_footer(ctx, hint, win_width, panel_y + panel_height + 32, COL_DIM);

		// AL panel
		// border is drawn before panel so the border doesn't go over title
		if (al_focused) {
			draw_border(ctx->renderer, al_panel.x - 1, al_panel.y - 1,
						al_panel.w + 2, al_panel.h + 2, COL_HIGHLIGHT);
		}

		draw_panel(ctx, al_panel.x, al_panel.y, al_panel.w, al_panel.h, " AL Teams: ");
		for (int i = 0; i < n_al; i++) {
			int ry = al_panel.y + content_y_off + i * row_height;
			draw_menu_row(ctx, al_panel.x + 2, ry, al_panel.w - 4,
					al_teams[i]->name,
					al_focused && i == al_sel,
					false);
		}

		// NL panel
		bool nl_focused = !al_focused;
		if (nl_focused) {
			draw_border(ctx->renderer, nl_panel.x - 1, nl_panel.y - 1,
						nl_panel.w + 2, nl_panel.h + 2, COL_HIGHLIGHT);
		}
		draw_panel(ctx, nl_panel.x, nl_panel.y, nl_panel.w, nl_panel.h, " NL Teams: ");
		for (int i = 0; i < n_nl; i++) {
			int ry = nl_panel.y + content_y_off + i * row_height;
			draw_menu_row(ctx, nl_panel.x + 2, ry, nl_panel.w - 4,
					nl_teams[i]->name,
					nl_focused && i == nl_sel,
					false);
		}

		SDL_RenderPresent(ctx->renderer);
		SDL_Delay(DEFAULT_DELAY);
	}

	return result;
}

static void draw_standings(SDLCtx *ctx, Panel p, const char *title, Team **teams, int n_teams, Team *selected) {
	draw_panel(ctx, p.x, p.y, p.w, p.h, title);
	int row_h = ctx->font_height + 2;
	int cy = p.y + ctx->font_height + 6;
	for (int i = 0; i < n_teams && cy + row_h < p.y + p.h; i++) {
		Team *t = teams[i];
		char buf[DEFAULT_BUFFER_LEN];
		snprintf(buf, sizeof(buf), "%s  %d - %d", t->name, t->wins, t->losses);
		SDL_Color col = (t == selected) ? COL_SELECTED : COL_TEXT;
		if (t == selected)
			fill_rect(ctx->renderer, p.x + 2, cy, p.w - 4, row_h, COL_BG);
		draw_text(ctx, ctx->font, buf, p.x + 6, cy + 1, col);
		cy += row_h;
	}
}

// season UI and offseason UI have different column offsets because it looks a little awkward if they don't 
const int SEASON_NAME_OFFSET	= 0;
const int SEASON_STAT1_OFFSET	= 23;
const int SEASON_STAT2_OFFSET	= 30;
const int SEASON_STAT3_OFFSET	= 37;
const int SEASON_STAT4_OFFSET	= 44;
const int SEASON_STAT5_OFFSET	= 52;
const int SEASON_STAT6_OFFSET	= 57;
const int SEASON_STAT7_OFFSET	= 62;

static void draw_hitter_stats_header(SDLCtx *ctx, Panel *p, int char_w) {
	int header_x = p->x + 10;
	int header_y = p->y + 10;
	draw_text(ctx, ctx->font, "Name", header_x + SEASON_NAME_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "AVG", header_x + SEASON_STAT1_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "OBP", header_x + SEASON_STAT2_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "SLG", header_x + SEASON_STAT3_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "OPS", header_x + SEASON_STAT4_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "HR", header_x + SEASON_STAT5_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "BB", header_x + SEASON_STAT6_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "RBI", header_x + SEASON_STAT7_OFFSET * char_w, header_y, COL_DIM);
}

static void draw_hitter_stats(SDLCtx *ctx, Panel *pn, Sim *s) {
    draw_panel(ctx, pn->x, pn->y, pn->w, pn->h, " Hitter Stats ");
	int char_h, char_w;
	TTF_SizeText(ctx->font, "A", &char_w, &char_h);

    int row_h = ctx->font_height + 2;
    int cy = pn->y + ctx->font_height - 8;
	const int COUNTING_STAT_OFFSET = 3;
    // column header
	draw_hitter_stats_header(ctx, pn, char_w);
    cy += row_h;
	char buf[DEFAULT_BUFFER_LEN];

    for (int i = 0; i < s->selected_team->n_hitters && cy + row_h < pn->y + pn->h; i++) {
        Hitter *h = s->selected_team->hitters[i];
		int rx = pn->x + 6; 
		snprintf(buf, sizeof(buf), "%s %s", h->base.first_name, h->base.last_name);
		draw_text(ctx, ctx->font, buf, rx + SEASON_NAME_OFFSET * char_w, cy, COL_TEXT);

		snprintf(buf, sizeof buf, ".%03d", (int)(h->stats.AVG * 1000));
		draw_text(ctx, ctx->font, buf, rx + SEASON_STAT1_OFFSET * char_w, cy, COL_TEXT);

		snprintf(buf, sizeof(buf), ".%03d", (int)(h->stats.OBP * 1000));
        draw_text(ctx, ctx->font, buf, rx + SEASON_STAT2_OFFSET * char_w, cy, COL_TEXT);

        snprintf(buf, sizeof(buf), ".%03d", (int)(h->stats.SLG * 1000));
        draw_text(ctx, ctx->font, buf, rx + SEASON_STAT3_OFFSET * char_w, cy, COL_TEXT);

        snprintf(buf, sizeof(buf), ".%03d", (int)(h->stats.OPS * 1000));
        draw_text(ctx, ctx->font, buf, rx + SEASON_STAT4_OFFSET * char_w, cy, COL_TEXT);

        snprintf(buf, sizeof(buf), "%d",   h->stats.HR);
        draw_text(ctx, ctx->font, buf, (rx + SEASON_STAT5_OFFSET * char_w) + COUNTING_STAT_OFFSET, cy, COL_TEXT);

        snprintf(buf, sizeof(buf), "%d",   h->stats.BB);
        draw_text(ctx, ctx->font, buf, (rx + SEASON_STAT6_OFFSET * char_w) + COUNTING_STAT_OFFSET, cy, COL_TEXT);

        snprintf(buf, sizeof(buf), "%d",   h->stats.RBI);
        draw_text(ctx, ctx->font, buf, (rx + SEASON_STAT7_OFFSET * char_w) + COUNTING_STAT_OFFSET, cy, COL_TEXT);

        cy += row_h;
    }
}

static void draw_pitcher_stats_header(SDLCtx *ctx, Panel *p, int char_w) {
	int header_x = p->x + 10;
	int header_y = p->y + 10;
	draw_text(ctx, ctx->font, "Name", header_x + SEASON_NAME_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "IP", header_x + SEASON_STAT1_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "ERA", header_x + SEASON_STAT2_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "K", header_x + SEASON_STAT3_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "BB", header_x + SEASON_STAT4_OFFSET * char_w, header_y, COL_DIM);
}

static void draw_pitcher_stats(SDLCtx *ctx, Panel *pn, Sim *s) {
    draw_panel(ctx, pn->x, pn->y, pn->w, pn->h, " Pitcher Stats ");
	int char_h, char_w;
	TTF_SizeText(ctx->font, "A", &char_w, &char_h);

    int row_h = ctx->font_height + 2;
    int cy    = pn->y + ctx->font_height - 8;
    // column header
	draw_pitcher_stats_header(ctx, pn, char_w);
    cy += row_h;
	char buf[DEFAULT_BUFFER_LEN];

    for (int i = 0; i < s->selected_team->n_pitchers && cy + row_h < pn->y + pn->h; i++) {
        Pitcher *p = s->selected_team->pitchers[i];
		int rx = pn->x + 6; 
		snprintf(buf, sizeof(buf), "%s %s", p->base.first_name, p->base.last_name);
		draw_text(ctx, ctx->font, buf, rx + SEASON_NAME_OFFSET * char_w, cy, COL_TEXT);

		snprintf(buf, sizeof(buf), "%d.%d", p->stats.IP.whole, p->stats.IP.thirds);
		draw_text(ctx, ctx->font, buf, rx + SEASON_STAT1_OFFSET * char_w, cy, COL_TEXT);

		snprintf(buf, sizeof(buf), "%.2f", p->stats.ERA);
		draw_text(ctx, ctx->font, buf, rx + SEASON_STAT2_OFFSET * char_w, cy, COL_TEXT);

		snprintf(buf, sizeof(buf), "%d", p->stats.SO);
		draw_text(ctx, ctx->font, buf, rx + SEASON_STAT3_OFFSET * char_w, cy, COL_TEXT);

		snprintf(buf, sizeof(buf), "%d", p->stats.BBA);
		draw_text(ctx, ctx->font, buf, rx + SEASON_STAT4_OFFSET * char_w, cy, COL_TEXT);

        cy += row_h;
    }
}

void sdl_season_ui(SDLCtx *ctx, Sim *sim) {
	bool running = true;

	while (running) {
		int win_width, win_height;
		SDL_GetWindowSize(ctx->window, &win_width, &win_height);
		int pad = 48;
		int gap = 12;
		int cols_right = 220;
		int cols_left = win_width - cols_right - pad * 2 - gap;
		int row_height = (win_height - pad * 2 - gap) / 2;
		int left_x = pad;
		int right_x = pad + cols_left + gap;
		int top_y = pad;
		int bot_y = pad + row_height + gap;

		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_QUIT: exit(0);
				case SDL_KEYDOWN:
					switch (e.key.keysym.sym) {
						case SDLK_SPACE:
						case SDLK_RETURN:
						case SDLK_KP_ENTER:
							if (sim->month == OCTOBER) {
								sdl_world_series_ui(ctx, sim);
								running = false;
							} else {
								sim_month(sim);
							}
							break;
						case SDLK_q:
						case SDLK_ESCAPE:
							running = false;
							break;
					}
					break;
				default: break;
			}
		}
		set_color(ctx->renderer, COL_BG);
		SDL_RenderClear(ctx->renderer);

		Panel pitchers_panel = { left_x, top_y, cols_left, row_height }; 
		Panel al_panel = { right_x, top_y, cols_right, row_height };
		Panel hitters_panel = { left_x, bot_y, cols_left, row_height };
		Panel nl_panel = { right_x, bot_y, cols_right, row_height };

		draw_pitcher_stats(ctx, &pitchers_panel, sim);
		draw_hitter_stats(ctx, &hitters_panel, sim);
		draw_standings(ctx, al_panel, "AL Standings", sim->al_teams, sim->al_team_count, sim->selected_team);
		draw_standings(ctx, nl_panel, "NL Standings", sim->nl_teams, sim->nl_team_count, sim->selected_team);

		char season_header[32];
		snprintf(season_header, sizeof(season_header), "%s %d", month_str(sim->month), sim->current_year);
		draw_screen_title(ctx, season_header, win_width, top_y - gap, COL_TITLE);
		draw_screen_footer(ctx, "press enter to advance month", win_width, win_height - 12, COL_DIM);
		SDL_RenderPresent(ctx->renderer);
		SDL_Delay(DEFAULT_DELAY);
	}
}

void sdl_world_series_ui(SDLCtx *ctx, Sim *sim) {
	// top finishers from AL and NL
	Team *al_champ = sim->al_teams[0];
	Team *nl_champ = sim->nl_teams[0];

	// set up and sim best of 7 series
	Series ws = {0};
	for (int i = 0; i < MAX_SERIES_MATCHES; i++)
		ws.matches[i] = (Match){ .t1 = al_champ, .t2 = nl_champ };
	sim_series(&ws);

	// format strings that are printed during the SDL render
	Team *winner = (ws.t1_wins > ws.t2_wins) ? al_champ : nl_champ;
	char winner_str[DEFAULT_BUFFER_LEN];
	snprintf(winner_str, sizeof(winner_str), "%s win the World Series!", winner->name);
	char match_str[ws.n_matches][DEFAULT_BUFFER_LEN];
	for (int i = 0; i < ws.n_matches; i++) {
		snprintf(match_str[i], sizeof(match_str[i]), "Game %d: %s %d - %d %s", i+1, ws.matches[i].t1->short_name, ws.matches[i].t1_runs, ws.matches[i].t2_runs, ws.matches[i].t2->short_name);
	}

	bool running = true;
	while (running) {
		int win_width, win_height;
		SDL_GetWindowSize(ctx->window, &win_width, &win_height);

		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_QUIT: exit(0);
				case SDL_KEYDOWN: case SDL_MOUSEBUTTONDOWN:
					running = false;
					break;
			}
		}

		set_color(ctx->renderer, COL_BG);
		SDL_RenderClear(ctx->renderer);
		int row_height = ctx->font_height + 4;
		int total_height = (((int)ws.n_matches + 1) * row_height) + 10;
		int start_y = (win_height - total_height) / 2;
		int max_width = 0;

		for (int i = 0; i < ws.n_matches; i++) {
			int tw, th;
			TTF_SizeText(ctx->font, match_str[i], &tw, &th);
			if (tw > max_width) max_width = tw;
		}

		int panel_width = max_width + 64;
		int panel_x = (win_width - panel_width) / 2;
		int padding = 24;

		draw_panel(ctx, panel_x - padding, start_y - padding,
				panel_width + padding * 2, total_height + padding * 2, "World Series");

		char series_str[DEFAULT_BUFFER_LEN];
		snprintf(series_str, sizeof(series_str), "%s vs. %s", al_champ->name, nl_champ->name);
		draw_text(ctx, ctx->font, series_str, panel_x, start_y, COL_HIGHLIGHT_TXT);
		int row;
		for (row = 0; row < ws.n_matches; row++) {
			int row_y = start_y + row_height + row * row_height;
			draw_text(ctx, ctx->font, match_str[row], panel_x, row_y, COL_TEXT);
		}
		draw_text(ctx, ctx->font, winner_str, panel_x, start_y + row_height + row * row_height, COL_HIGHLIGHT_TXT);

		SDL_RenderPresent(ctx->renderer);
		SDL_Delay(DEFAULT_DELAY);
	}

	// age curve before roster changes to not deceive the player
	age_curve_players(sim);
	sdl_offseason_ui(ctx, sim);
}

// offseason UI
typedef enum {
	FOCUS_HITTER_ROSTER = 0,
    FOCUS_HITTER_PROSPECTS,
    FOCUS_PITCHER_ROSTER,
    FOCUS_PITCHER_PROSPECTS,
    FOCUS_COUNT
} OffseasonFocus;

const int OFFSEASON_NAME_OFFSET		= 0;
const int OFFSEASON_AGE_OFFSET		= 20;
const int OFFSEASON_STAT1_OFFSET	= 25;
const int OFFSEASON_STAT2_OFFSET	= 30;
const int OFFSEASON_STAT3_OFFSET	= 35;
const int OFFSEASON_STAT4_OFFSET	= 40;
const int OFFSEASON_STAT5_OFFSET	= 45;
const int OFFSEASON_STAT6_OFFSET	= 50;
const int OFFSEASON_STAT7_OFFSET	= 55;

static void draw_hitter_header(SDLCtx *ctx, Panel *p, int char_w) {
	int header_x = p->x + 10;
	int header_y = p->y + 10;
	draw_text(ctx, ctx->font, "Name",  header_x + OFFSEASON_NAME_OFFSET  * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "AGE",   header_x + OFFSEASON_AGE_OFFSET   * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "CON",   header_x + OFFSEASON_STAT1_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "EYE",   header_x + OFFSEASON_STAT2_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "PWR",   header_x + OFFSEASON_STAT3_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "SPD",   header_x + OFFSEASON_STAT4_OFFSET * char_w, header_y, COL_DIM);
}

static void draw_pitcher_header(SDLCtx *ctx, Panel *p, int char_w) {
	int header_x = p->x + 10;
	int header_y = p->y + 10;
	draw_text(ctx, ctx->font, "Name",  header_x + OFFSEASON_NAME_OFFSET  * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "AGE",   header_x + OFFSEASON_AGE_OFFSET   * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "CMD",   header_x + OFFSEASON_STAT1_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "STF",   header_x + OFFSEASON_STAT2_OFFSET * char_w, header_y, COL_DIM);
	draw_text(ctx, ctx->font, "STM",   header_x + OFFSEASON_STAT3_OFFSET * char_w, header_y, COL_DIM);
}

static void draw_hitter_overview(SDLCtx *ctx, Panel *pn, Hitter *h, int char_w, int row, 
								int row_x, int row_y, SDL_Color col) {
	char buf[DEFAULT_BUFFER_LEN];
	snprintf(buf, sizeof(buf), "%s %s", h->base.first_name, h->base.last_name);
	draw_text(ctx, ctx->font, buf, row_x + OFFSEASON_NAME_OFFSET * char_w, row_y + 1, col);
	snprintf(buf, sizeof(buf), "%d", h->base.age);
	draw_text(ctx, ctx->font, buf, row_x + OFFSEASON_AGE_OFFSET * char_w, row_y + 1, col);
	snprintf(buf, sizeof(buf), "%d", h->ratings.contact);
	draw_text(ctx, ctx->font, buf, row_x + OFFSEASON_STAT1_OFFSET * char_w, row_y + 1, col);
	snprintf(buf, sizeof(buf), "%d", h->ratings.power);
	draw_text(ctx, ctx->font, buf, row_x + OFFSEASON_STAT2_OFFSET * char_w, row_y + 1, col);
	snprintf(buf, sizeof(buf), "%d", h->ratings.eye);
	draw_text(ctx, ctx->font, buf, row_x + OFFSEASON_STAT3_OFFSET * char_w, row_y + 1, col);
	snprintf(buf, sizeof(buf), "%d", h->ratings.speed);
	draw_text(ctx, ctx->font, buf, row_x + OFFSEASON_STAT4_OFFSET * char_w, row_y + 1, col);
}

static void draw_pitcher_overview(SDLCtx *ctx, Panel *pn, Pitcher *p, int char_w, int row, 
								int row_x, int row_y, SDL_Color col) {
	char buf[DEFAULT_BUFFER_LEN];
	snprintf(buf, sizeof(buf), "%s %s", p->base.first_name, p->base.last_name);
	draw_text(ctx, ctx->font, buf, row_x + OFFSEASON_NAME_OFFSET * char_w, row_y + 1, col);
	snprintf(buf, sizeof(buf), "%d", p->base.age);
	draw_text(ctx, ctx->font, buf, row_x + OFFSEASON_AGE_OFFSET * char_w, row_y + 1, col);
	snprintf(buf, sizeof(buf), "%d", p->ratings.command);
	draw_text(ctx, ctx->font, buf, row_x + OFFSEASON_STAT1_OFFSET * char_w, row_y + 1, col);
	snprintf(buf, sizeof(buf), "%d", p->ratings.stuff);
	draw_text(ctx, ctx->font, buf, row_x + OFFSEASON_STAT2_OFFSET * char_w, row_y + 1, col);
	snprintf(buf, sizeof(buf), "%d", p->ratings.stamina);
	draw_text(ctx, ctx->font, buf, row_x + OFFSEASON_STAT3_OFFSET * char_w, row_y + 1, col);
}

void sdl_offseason_ui(SDLCtx *ctx, Sim *sim) {
	// generate prospects that the player can choose to replace players with
	const size_t N_HITTER_PROSPECTS = 3;
	const size_t N_PITCHER_PROSPECTS = 2;
	Hitter *hitter_prospects[N_HITTER_PROSPECTS];
	Pitcher *pitcher_prospects[N_PITCHER_PROSPECTS];
	Team *sel_team = sim->selected_team;

	for (size_t h = 0; h < N_HITTER_PROSPECTS; h++)
		hitter_prospects[h] = gen_hitter(random_int_range(18, 24));
	for (size_t p = 0; p < N_PITCHER_PROSPECTS; p++)
		pitcher_prospects[p] = gen_pitcher(random_int_range(18, 24));

	bool hitter_roster_active[MAX_HITTERS];
	bool hitter_prospect_active[N_HITTER_PROSPECTS];
	bool pitcher_roster_active[MAX_PITCHERS];
	bool pitcher_prospect_active[N_PITCHER_PROSPECTS];

	for (size_t i = 0; i < MAX_HITTERS; i++) hitter_roster_active[i] = true;
	for (size_t i = 0; i < N_HITTER_PROSPECTS; i++) hitter_prospect_active[i] = false;
	for (size_t i = 0; i < MAX_PITCHERS; i++) pitcher_roster_active[i] = true;
	for (size_t i = 0; i < N_PITCHER_PROSPECTS; i++) pitcher_prospect_active[i] = false;

	OffseasonFocus focus = FOCUS_HITTER_ROSTER;
	size_t sel[FOCUS_COUNT] = {0};
	char validation_msg[DEFAULT_BUFFER_LEN];
	int char_w, char_h;
	TTF_SizeText(ctx->font, "A", &char_w, &char_h);

	bool running = true;
	while (running) {
		int win_width, win_height;
		SDL_GetWindowSize(ctx->window, &win_width, &win_height);
		int pad = 48;
		int gap = 12;
		int half_w = (win_width - pad * 2 - gap) / 2;
		int half_h = (win_height - pad * 2 - gap * 2 - ctx->font_height - 8) / 2;
		int top_y = pad;
		int bot_y = pad + half_h + gap;
        int left_x = pad;
        int right_x = pad + half_w + gap;

		Panel panels[FOCUS_COUNT] = {
			[FOCUS_HITTER_ROSTER] = { left_x, top_y, half_w, half_h},
			[FOCUS_HITTER_PROSPECTS] = { right_x, top_y, half_w, half_h },
			[FOCUS_PITCHER_ROSTER] = { left_x, bot_y, half_w, half_h },
			[FOCUS_PITCHER_PROSPECTS] = { right_x, bot_y, half_w, half_h },
		};

		size_t panel_counts[FOCUS_COUNT] = {
			[FOCUS_HITTER_ROSTER]    = sel_team->n_hitters,
			[FOCUS_HITTER_PROSPECTS] = N_HITTER_PROSPECTS,
			[FOCUS_PITCHER_ROSTER]   = sel_team->n_pitchers,
			[FOCUS_PITCHER_PROSPECTS]= N_PITCHER_PROSPECTS,
		};

		int row_height = ctx->font_height + 4;
		int content_off = ctx->font_height + 16;

		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_QUIT: exit(0);
				case SDL_KEYDOWN:
					switch (e.key.keysym.sym) {
						case SDLK_ESCAPE:
							running = false;
							break;
						case SDLK_TAB:
							focus = (focus + 1) % FOCUS_COUNT;
							break;
						case SDLK_UP: case SDLK_k:
							if (sel[focus] > 0) sel[focus]--;
							else sel[focus] = panel_counts[focus] - 1;
							break;
						case SDLK_DOWN: case SDLK_j:
							sel[focus] = (sel[focus] + 1) % panel_counts[focus];
							break;
						case SDLK_SPACE:
							validation_msg[0] = '\0';
							switch (focus) {
								case FOCUS_HITTER_ROSTER:
									hitter_roster_active[sel[focus]] = !hitter_roster_active[sel[focus]];
									break;
								case FOCUS_HITTER_PROSPECTS:
									hitter_prospect_active[sel[focus]] = !hitter_prospect_active[sel[focus]];
									break;
								case FOCUS_PITCHER_ROSTER:
									pitcher_roster_active[sel[focus]] = !pitcher_roster_active[sel[focus]];
									break;
								case FOCUS_PITCHER_PROSPECTS:
									pitcher_prospect_active[sel[focus]] = !pitcher_prospect_active[sel[focus]];
									break;
								default: break;
							}
							break;
						case SDLK_RETURN: case SDLK_KP_ENTER: {
							unsigned int active_hitters = 0;
							for (int i = 0; i < sel_team->n_hitters; i++)
								if (hitter_roster_active[i]) active_hitters++;
							for (int i = 0; i < N_HITTER_PROSPECTS; i++)
								if (hitter_prospect_active[i]) active_hitters++;

							unsigned int active_pitchers = 0;
							for (int i = 0; i < sel_team->n_pitchers; i++)
								if (pitcher_roster_active[i]) active_pitchers++;
							for (int i = 0; i < N_PITCHER_PROSPECTS; i++)
								if (pitcher_prospect_active[i]) active_pitchers++;

							if (active_hitters != sel_team->n_hitters || active_pitchers != sel_team->n_pitchers) {
								snprintf(validation_msg, sizeof(validation_msg), " invalid roster size ");
								break;
							}

							// build new rosters
							Hitter *new_hitters[MAX_HITTERS];
							size_t n_new_hitters = 0;
							for (size_t i = 0; i < MAX_HITTERS; i++) {
								if (hitter_roster_active[i])
									new_hitters[n_new_hitters++] = sel_team->hitters[i];
								else
									free_hitter(sel_team->hitters[i]);
							}
							for (size_t i = 0; i < N_HITTER_PROSPECTS; i++)
								if (hitter_prospect_active[i])
									new_hitters[n_new_hitters++] = hitter_prospects[i];
							memcpy(sel_team->hitters, new_hitters, n_new_hitters * sizeof(Hitter *));
							sel_team->n_hitters = n_new_hitters;

							Pitcher *new_pitchers[MAX_PITCHERS];
							size_t n_new_pitchers = 0;
							for (size_t i = 0; i < MAX_PITCHERS; i++) {
								if (pitcher_roster_active[i])
									new_pitchers[n_new_pitchers++] = sel_team->pitchers[i];
								else
									free_pitcher(sel_team->pitchers[i]);
							}
							for (size_t i = 0; i < N_PITCHER_PROSPECTS; i++)
								if (pitcher_prospect_active[i])
									new_pitchers[n_new_pitchers++] = pitcher_prospects[i];
							memcpy(sel_team->pitchers, new_pitchers, n_new_pitchers * sizeof(Pitcher *));
							sel_team->n_pitchers = n_new_pitchers;

							running = false;
							break;
						}
					}
					break;
				case SDL_MOUSEMOTION: {
					int mx = e.motion.x, my = e.motion.y;
					for (int p = 0; p < FOCUS_COUNT; p++) {
						if (panel_hit(panels[p], mx, my)) {
							int row = (my - panels[p].y - content_off) / row_height;
							if (row >= 0 && (size_t)row < panel_counts[p]) {
								focus  = p;
								sel[p] = row;
							}
						}
					}
					break;
				}

				case SDL_MOUSEBUTTONDOWN: {
					if (e.button.button != SDL_BUTTON_LEFT) break;
					int mx = e.button.x, my = e.button.y;
					for (int p = 0; p < FOCUS_COUNT; p++) {
						if (!panel_hit(panels[p], mx, my)) continue;
						int row = (my - panels[p].y - content_off) / row_height;
						if (row < 0 || (size_t)row >= panel_counts[p]) break;
						if ((int)focus == p && (int)sel[p] == row) {
							SDL_Event fake = { .type = SDL_KEYDOWN };
							fake.key.keysym.sym = SDLK_RETURN;
							SDL_PushEvent(&fake);
						} else {
							focus = p;
							sel[p] = row;
						}
					}
					break;
				}
			}
		}

		set_color(ctx->renderer, COL_BG);
		SDL_RenderClear(ctx->renderer);

		draw_screen_title(ctx, "ROSTER CHANGES", win_width, top_y - gap, COL_TITLE);

		const char *titles[FOCUS_COUNT] = {
			" Team Hitters ",
			" Hitter Prospects ",
			" Team Pitchers ",
			" Pitcher Prospects "
		};
		for (int p = 0; p < FOCUS_COUNT; p++) {
			if (focus == p)
				draw_border(ctx->renderer, panels[p].x - 1, panels[p].y - 1,
						panels[p].w + 2, panels[p].h + 2, COL_HIGHLIGHT);
			draw_panel(ctx, panels[p].x, panels[p].y, panels[p].w, panels[p].h, titles[p]);
		}

		draw_hitter_header(ctx, &panels[FOCUS_HITTER_ROSTER], char_w);
		draw_hitter_header(ctx, &panels[FOCUS_HITTER_PROSPECTS], char_w);
		draw_pitcher_header(ctx, &panels[FOCUS_PITCHER_ROSTER], char_w);
		draw_pitcher_header(ctx, &panels[FOCUS_PITCHER_PROSPECTS], char_w);

		// draw rows for roster and prospects
		Panel *pn = &panels[FOCUS_HITTER_ROSTER];
		int rx = pn->x + 10;
		for (int i = 0; i < sel_team->n_hitters; i++) {
			Hitter *h = sel_team->hitters[i];
            bool highlighted = (focus == FOCUS_HITTER_ROSTER && sel[FOCUS_HITTER_ROSTER] == i);
			bool active = hitter_roster_active[i];
			int ry = pn->y + content_off + i * row_height;
			if (highlighted)
				fill_rect(ctx->renderer, pn->x + 2, ry, pn->w - 4, row_height, COL_HIGHLIGHT);
			SDL_Color col = (highlighted ? COL_HIGHLIGHT_TXT : active ? COL_SELECTED : COL_TEXT);
			draw_hitter_overview(ctx, pn, h, char_w, i, rx, ry, col);
		}

		pn = &panels[FOCUS_HITTER_PROSPECTS];
		rx = pn->x + 10;
		for (int i = 0; i < N_HITTER_PROSPECTS; i++) {
			Hitter *h = hitter_prospects[i];
            bool highlighted = (focus == FOCUS_HITTER_PROSPECTS && sel[FOCUS_HITTER_PROSPECTS] == i);
			bool active = hitter_prospect_active[i];
			SDL_Color col = (highlighted ? COL_HIGHLIGHT_TXT : active ? COL_SELECTED : COL_TEXT);
			int ry = pn->y + content_off + i * row_height;
			if (highlighted)
				fill_rect(ctx->renderer, pn->x + 2, ry, pn->w - 4, row_height, COL_HIGHLIGHT);
			draw_hitter_overview(ctx, pn, h, char_w, i, rx, ry, col);
		}

		// pitcher roster rows
		pn = &panels[FOCUS_PITCHER_ROSTER];
		rx = pn->x + 10;
		for (int i = 0; i < sel_team->n_pitchers; i++) {
			Pitcher *p = sel_team->pitchers[i];
            bool highlighted = (focus == FOCUS_PITCHER_ROSTER && sel[FOCUS_PITCHER_ROSTER] == i);
			bool active = pitcher_roster_active[i];
			SDL_Color col = (highlighted ? COL_HIGHLIGHT_TXT : active ? COL_SELECTED : COL_TEXT);
			int ry = pn->y + content_off + i * row_height;
			if (highlighted)
				fill_rect(ctx->renderer, pn->x + 2, ry, pn->w - 4, row_height, COL_HIGHLIGHT);
			draw_pitcher_overview(ctx, pn, p, char_w, i, rx, ry, col);
		}

		pn = &panels[FOCUS_PITCHER_PROSPECTS];
		rx = pn->x + 10;
		for (int i = 0; i < N_PITCHER_PROSPECTS; i++) {
			Pitcher *p = pitcher_prospects[i];
            bool highlighted = (focus == FOCUS_PITCHER_PROSPECTS && sel[FOCUS_PITCHER_PROSPECTS] == i);
			bool active = pitcher_prospect_active[i];
			SDL_Color col = (highlighted ? COL_HIGHLIGHT_TXT : active ? COL_SELECTED : COL_TEXT);
			int ry = pn->y + content_off + i * row_height;
			if (highlighted)
				fill_rect(ctx->renderer, pn->x + 2, ry, pn->w - 4, row_height, COL_HIGHLIGHT);
			draw_pitcher_overview(ctx, pn, p, char_w, i, rx, ry, col);
		}

		// validation error
		if (validation_msg[0] != '\0') {
			int vw, vh;
			TTF_SizeText(ctx->font, validation_msg, &vw, &vh);
			draw_text(ctx, ctx->font, validation_msg,
					(win_width - vw) / 2, win_height - vh * 2 - 10,
					COL_URGENT);
		}

		// footer
        const char *hint = "tab  cycle to next panel    arrows or j/k  navigate    space  toggle    enter  confirm";
		draw_screen_footer(ctx, hint, win_width, win_height - 6, COL_DIM);

        SDL_RenderPresent(ctx->renderer);
        SDL_Delay(DEFAULT_DELAY);
	}

    // free any prospects that weren't used
    for (size_t i = 0; i < N_HITTER_PROSPECTS; i++) {
        if (hitter_prospects[i] && !hitter_prospect_active[i]) free_hitter(hitter_prospects[i]);
	}

    for (size_t i = 0; i < N_PITCHER_PROSPECTS; i++) {
        if (pitcher_prospects[i] && !pitcher_prospect_active[i]) free_pitcher(pitcher_prospects[i]);
	}

	sim_offseason(sim);
	sdl_season_ui(ctx, sim);
}
