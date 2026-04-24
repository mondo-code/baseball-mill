#ifndef SDL_FRONTEND_H
#define SDL_FRONTEND_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include "team.h"
#include "sim.h"

// SDL boilerplate stuff
typedef struct {
	SDL_Window		*window;
	SDL_Renderer	*renderer;
	TTF_Font		*font;
	TTF_Font		*font_bold;
	int				font_width;
	int				font_height;
} SDLCtx;

typedef struct { int x, y, w, h; } Panel;

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
