#ifndef SDL_FRONTEND_H
#define SDL_FRONTEND_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <string.h>
#include "team.h"
#include "sim.h"

#define COL_BG				((SDL_Color){  15,  15,  20, 255 })
#define COL_PANEL			((SDL_Color){  25,  28,  36, 255 })
#define COL_BORDER			((SDL_Color){  70,  80, 100, 255 })
#define COL_TITLE			((SDL_Color){ 255, 200,  60, 255 })
#define COL_TEXT			((SDL_Color){ 210, 215, 225, 255 })
#define COL_DIM				((SDL_Color){ 110, 120, 140, 255 })
#define COL_HIGHLIGHT		((SDL_Color){  50, 120, 220, 255 })
#define COL_HIGHLIGHT_TXT	((SDL_Color){ 255, 255, 255, 255 })
#define COL_SELECTED		((SDL_Color){  60, 200, 100, 255 })
#define COL_URGENT			((SDL_Color){ 220, 80,	80,	 255 })

// team specific highlight colors
#define COL_OAK	((SDL_Color){ 238, 178, 30, 255 })
#define COL_BAL	((SDL_Color){ 221, 73, 38, 255 })
#define COL_BOS	((SDL_Color){ 189, 49, 57, 255 })
#define COL_DET	((SDL_Color){ 242, 103, 34, 255 })
#define COL_MIN	((SDL_Color){ 221, 30, 71, 255 })
#define COL_SEA	((SDL_Color){ 2, 126, 126, 255 })
#define COL_CHW	((SDL_Color){ 196, 206, 211, 255 })
#define COL_NYY	((SDL_Color){ 196, 206, 211, 255 })
#define COL_ATL	((SDL_Color){ 206, 31, 67, 255 })
#define COL_CHC	((SDL_Color){ 204, 52, 51, 255 })
#define COL_STL	((SDL_Color){ 196, 32, 59, 255 })
#define COL_LAD	((SDL_Color){ 0, 104, 179, 255 })
#define COL_SFG	((SDL_Color){ 241, 91, 40, 255 })
#define COL_NYM	((SDL_Color){ 255, 89, 16, 255 })
#define COL_PHI	((SDL_Color){ 231, 29, 42, 255 })
#define COL_CIN	((SDL_Color){ 198, 33, 39, 255 })

// lookup table for team colors

typedef struct {
	const char *short_name;
	SDL_Color	color;
} TeamColorEntry;

static const TeamColorEntry team_colors[] = {
	{ "OAK", COL_OAK },
    { "BAL", COL_BAL },
    { "BOS", COL_BOS },
    { "DET", COL_DET },
    { "MIN", COL_MIN },
    { "SEA", COL_SEA },
    { "CHW", COL_CHW },
    { "NYY", COL_NYY },
    { "ATL", COL_ATL },
    { "CHC", COL_CHC },
    { "STL", COL_STL },
    { "LAD", COL_LAD },
    { "SFG", COL_SFG },
    { "NYM", COL_NYM },
    { "PHI", COL_PHI },
    { "CIN", COL_CIN },
};

static SDL_Color team_color_lookup(const char *team_name) {
	for (int i = 0; i < N_TEAMS; i++) {
		if (strcmp(team_colors[i].short_name, team_name) == 0) {
			return team_colors[i].color;
		}
	}
	return COL_SELECTED;
}

typedef struct {
	SDL_Color bg;
	SDL_Color panel;
	SDL_Color border;
	SDL_Color title;
	SDL_Color text;
	SDL_Color dim;
	SDL_Color highlight;
	SDL_Color highlight_txt;
	SDL_Color selected;
	SDL_Color urgent;
} Theme;

// SDL boilerplate stuff
typedef struct {
	SDL_Window		*window;
	SDL_Renderer	*renderer;
	TTF_Font		*font;
	TTF_Font		*font_bold;
	int				font_width;
	int				font_height;
	Theme			theme;
} SDLCtx;

typedef struct { int x, y, w, h; } Panel;

Theme init_default_theme();
bool init_sdl_ctx(SDLCtx *ctx, const char *font_path, const char *font_bold_path, int win_width, int win_height);
void destroy_sdl_ctx(SDLCtx *ctx);

// actual screen functions
size_t sdl_main_menu(SDLCtx *ctx, const char **options, size_t n_options);
const char *sdl_team_select(SDLCtx *ctx, Team **al_teams, size_t n_al, Team **nl_teams, size_t n_nl); 
void sdl_season_ui(SDLCtx *ctx, Sim *sim);
void sdl_world_series_ui(SDLCtx *ctx, Sim *sim);
void sdl_season_end_ui(SDLCtx *ctx, Sim *sim);
void sdl_offseason_ui(SDLCtx *ctx, Sim *sim);
void sdl_history_ui(SDLCtx *ctx, Sim *sim);

#endif  // SDL_FRONTEND_H
