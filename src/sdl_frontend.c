#include <SDL2/SDL.h>
#include "sdl_frontend.h"

#define COL_BG				((SDL_Color){  15,  15,  20, 255 })  // near-black
#define COL_PANEL			((SDL_Color){  25,  28,  36, 255 })  // panel fill
#define COL_BORDER			((SDL_Color){  70,  80, 100, 255 })  // box lines
#define COL_TITLE			((SDL_Color){ 255, 200,  60, 255 })  // gold heading
#define COL_TEXT			((SDL_Color){ 210, 215, 225, 255 })  // normal text
#define COL_DIM				((SDL_Color){ 110, 120, 140, 255 })  // dimmed text
#define COL_HIGHLIGHT		((SDL_Color){  50, 120, 220, 255 })  // selection bg
#define COL_HIGHLIGHT_TXT	((SDL_Color){ 255, 255, 255, 255 })
#define COL_SELECTED		((SDL_Color){  60, 200, 100, 255 })  // "your team" indicator
#define BUFFER_LEN			128

const unsigned int DEFAULT_DELAY = 16;

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
	fprintf(stdout, "Initialized TTF");

	ctx->window = SDL_CreateWindow(
			"Baseball Mill v0.1",
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

static void draw_menu_row(SDLCtx *ctx, int x, int y, int w, const char *text, bool highlighted, bool is_selected) {
	if (highlighted) {
		fill_rect(ctx->renderer, x, y, w, ctx->font_height + 2, COL_HIGHLIGHT);
		draw_text(ctx, ctx->font, text, x + 4, y + 1, COL_HIGHLIGHT_TXT);
	} else {
		SDL_Color text_color = is_selected ? COL_SELECTED : COL_TEXT;
		draw_text(ctx, ctx->font, text, x + 4, y + 1, text_color);
	}
}

// panel layout stuff
typedef struct { int x, y, w, h; } Panel;

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
				case SDL_QUIT:
					running = false;
					break;
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
				case SDL_QUIT:
					running = false;
					break;

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
		// draw
		set_color(ctx->renderer, COL_BG);
		SDL_RenderClear(ctx->renderer);

		// draw screen title
		{
			const char *title = "SELECT YOUR TEAM";
			int tw; int th;
			TTF_SizeText(ctx->font_bold, title, &tw, &th);
			draw_text(ctx, ctx->font_bold, title,
					(win_width - tw) / 2, panel_y - th - 12, COL_TITLE);
		}

		// hint
		{
            const char *hint = "arrows/vim keys to move  |  tab to switch league  |  enter or click to confirm";
			int hw; int hh;
			TTF_SizeText(ctx->font, hint, &hw, &hh);
			draw_text(ctx, ctx->font, hint, (win_width - hw) / 2, panel_y + panel_height + 10, COL_DIM);
		}

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

// season UI panels
#define N_SEASON_PANELS 4
#define PITCHERS_PANEL	0
#define AL_PANEL		1
#define HITTERS_PANEL	2
#define NL_PANEL		3

static Panel season_layout(int win_width, int win_height, int idx) {
	int pad = 12;
	int gap = 8;
	int cols_right = 220;
	int cols_left = win_width - cols_right - pad * 2 - gap;
	int row_height = (win_height - pad * 2 - gap) / 2;

	int left_x = pad;
	int right_x = pad + cols_left + gap;
	int top_y = pad;
	int bot_y = pad + row_height + gap;

	Panel panels[N_SEASON_PANELS] = {
		{ left_x, top_y, cols_left, row_height },
		{ right_x, top_y, cols_right, row_height },
		{ left_x, bot_y, cols_left, row_height },
		{ right_x, bot_y, cols_right, row_height },
	};
	return panels[idx];
}

static void draw_standings(SDLCtx *ctx, Panel p, const char *title, Team **teams, int n_teams, Team *selected) {
	draw_panel(ctx, p.x, p.y, p.w, p.h, title);
	int row_h = ctx->font_height + 2;
	int cy = p.y + ctx->font_height + 6;
	for (int i = 0; i < n_teams && cy + row_h < p.y + p.h; i++) {
		Team *t = teams[i];
		char buf[BUFFER_LEN];
		snprintf(buf, sizeof(buf), "%s  %d - %d", t->name, t->wins, t->losses);
		SDL_Color col = (t == selected) ? COL_SELECTED : COL_TEXT;
		if (t == selected)
			fill_rect(ctx->renderer, p.x + 2, cy, p.w - 4, row_h, COL_BG);
		draw_text(ctx, ctx->font, buf, p.x + 6, cy + 1, col);
		cy += row_h;
	}
}

static void draw_hitter_stats(SDLCtx *ctx, Panel p, Sim *s) {
    draw_panel(ctx, p.x, p.y, p.w, p.h, " Hitter Stats ");
    int row_h = ctx->font_height + 2;
    int cy    = p.y + ctx->font_height + 6;
    // column header
    draw_text(ctx, ctx->font,
              "Name                     PA    AB   AVG    OBP    SLG    OPS     H   2B   3B   HR   BB  RBI",
              p.x + 6, cy, COL_DIM);
    cy += row_h + 2;

    for (int i = 0; i < s->selected_team->n_hitters && cy + row_h < p.y + p.h; i++) {
        Hitter *h = s->selected_team->hitters[i];
        char buf[512];
        snprintf(buf, sizeof(buf),
                 "%-22s %4d  %4d  .%03d   .%03d   .%03d   .%03d  %4d %4d %4d %4d %4d %4d",
                 ({
                     static char name[32];
                     snprintf(name, sizeof(name), "%s %s", h->base->first_name, h->base->last_name);
                     name;
                 }),
                 h->stats.PA, h->stats.AB,
                 (int)(h->stats.AVG * 1000),
                 (int)(h->stats.OBP * 1000),
                 (int)(h->stats.SLG * 1000),
                 (int)(h->stats.OPS * 1000),
                 h->stats.H, h->stats.H2, h->stats.H3,
                 h->stats.HR, h->stats.BB, h->stats.RBI);
        draw_text(ctx, ctx->font, buf, p.x + 6, cy, COL_TEXT);
        cy += row_h;
    }
}

static void draw_pitcher_stats(SDLCtx *ctx, Panel p, Sim *s) {
	draw_panel(ctx, p.x, p.y, p.w, p.h, " Pitcher Stats ");
	int row_h = ctx->font_height + 2;
	int cy = p.y + ctx->font_height + 6;
	draw_text(ctx, ctx->font,
              "Name                     GS      IP     ERA       K   BB",
			  p.x + 6, cy, COL_DIM);
	cy += row_h + 2;

	for (int i = 0; i < s->selected_team->n_pitchers && cy + row_h < p.y + p.h; i++) {
		Pitcher *pitcher = s->selected_team->pitchers[i];
		char buf[256];
		snprintf(buf, sizeof(buf),
                 "%-22s  %3d   %4d.%d  %6.2f  %5d %4d",
				 ({
				  static char nm[24];
				  snprintf(nm, sizeof(nm), "%s %s", pitcher->base->first_name, pitcher->base->last_name);
				  nm;
				  }),
				 pitcher->stats.GS,
				 pitcher->stats.IP.whole,
				 pitcher->stats.IP.thirds,
				 pitcher->stats.ERA,
				 pitcher->stats.SO,
				 pitcher->stats.BBA);
		draw_text(ctx, ctx->font, buf, p.x + 6, cy, COL_TEXT);
		cy += row_h;
	}
}

static void draw_season_footer(SDLCtx *ctx, int win_width, int win_height, Sim *s) {
	const char *hint = "press enter to advance month";
	int hw, hh;
	TTF_SizeText(ctx->font, hint, &hw, &hh);
	draw_text(ctx, ctx->font, hint, (win_width - hw) / 2, win_height - hh - 16, COL_DIM);
	draw_text(ctx, ctx->font, month_str(s->month), 16, win_height - hh - 16, COL_TITLE);
}

void sdl_season_ui(SDLCtx *ctx, Sim *sim) {
	bool running = true;

	while (running) {
		int win_width, win_height;
		SDL_GetWindowSize(ctx->window, &win_width, &win_height);

		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_QUIT:
					running = false;
					break;
				case SDL_KEYDOWN:
					switch (e.key.keysym.sym) {
						case SDLK_SPACE:
						case SDLK_RETURN:
						case SDLK_KP_ENTER:
							if (sim->month == OCTOBER) {
								sdl_world_series_ui(ctx, sim);
								/* sdl_season_end_ui(ctx, sim); */
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

		Panel pitchers_panel = season_layout(win_width, win_height, PITCHERS_PANEL);
		Panel al_panel = season_layout(win_width, win_height, AL_PANEL); 
		Panel hitters_panel = season_layout(win_width, win_height, HITTERS_PANEL);
		Panel nl_panel = season_layout(win_width, win_height, NL_PANEL);

		draw_pitcher_stats(ctx, pitchers_panel, sim);
		draw_hitter_stats(ctx, hitters_panel, sim);
		draw_standings(ctx, al_panel, "AL Standings", sim->al_teams, sim->al_team_count, sim->selected_team);
		draw_standings(ctx, nl_panel, "NL Standings", sim->nl_teams, sim->nl_team_count, sim->selected_team);
		draw_season_footer(ctx, win_width, win_height, sim);
		SDL_RenderPresent(ctx->renderer);
		SDL_Delay(DEFAULT_DELAY);
	}
}

void sdl_world_series_ui(SDLCtx *ctx, Sim *sim) {
	// pull the top finishers from AL and NL
	Team *al_champ = sim->al_teams[0];
	Team *nl_champ = sim->nl_teams[0];

	// set up and sim best of 7 series
	Series ws = {0};
	for (int i = 0; i < MAX_SERIES_MATCHES; i++)
		ws.matches[i] = (Match){ .t1 = al_champ, .t2 = nl_champ };
	sim_series(&ws);

	// format strings that are printed during the SDL render
	Team *winner = (ws.t1_wins > ws.t2_wins) ? al_champ : nl_champ;
	char winner_str[BUFFER_LEN];
	snprintf(winner_str, sizeof(winner_str), "%s win the World Series!", winner->name);
	char match_str[ws.n_matches][BUFFER_LEN];
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
				case SDL_QUIT: case SDL_KEYDOWN: case SDL_MOUSEBUTTONDOWN:
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

		int panel_width = max_width + 48;
		int panel_x = (win_width - panel_width) / 2;
		int padding = 16;

		draw_panel(ctx, panel_x - padding, start_y - padding, 
				panel_width + padding * 2, total_height + padding * 2, "World Series");

		char series_str[BUFFER_LEN];
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

	sdl_season_end_ui(ctx, sim);
}

// this is a placeholder, offseason UI will be called instead of this
void sdl_season_end_ui(SDLCtx *ctx, Sim *sim) {
	bool running = true;
	while (running) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) running = false;
			if (e.type == SDL_KEYDOWN) running = false;
		}

		int win_width, win_height;
		SDL_GetWindowSize(ctx->window, &win_width, &win_height);
		set_color(ctx->renderer, COL_BG);
		SDL_RenderClear(ctx->renderer);

		const char *msg = "Season complete";
		int tw, th;
		TTF_SizeText(ctx->font_bold, msg, &tw, &th);
        draw_text(ctx, ctx->font_bold, msg,
                  (win_width - tw) / 2, win_height / 2 - th, COL_TITLE);
        SDL_RenderPresent(ctx->renderer);
        SDL_Delay(DEFAULT_DELAY);
	}
}

// TODO: implement offseason roster changes
void sdl_offseason_ui(SDLCtx *ctx, Sim *sim); 
