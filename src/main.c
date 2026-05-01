#include <stddef.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "sdl_frontend.h"
#include "sim.h"
#include "team.h"

int main(void) {
	SDLCtx ctx;
	if (!init_sdl_ctx(&ctx, "assets/JetBrainsMono-Regular.ttf", "assets/JetBrainsMono-Bold.ttf", 1400, 800)) {
		fprintf(stderr, "SDL context init failed\n");
		return 1;
	}

	const char* start_menu[] = { "New Sim", "Exit" };
	size_t n_menu_options = 2;
	size_t choice = sdl_main_menu(&ctx, start_menu, n_menu_options);
	switch(choice) {
		case 0:
			// New Save
			Team **al_teams = init_al_teams();
			Team **nl_teams = init_nl_teams();
			const char *sel = sdl_team_select(&ctx, al_teams, N_AL_TEAMS, 
													nl_teams, N_NL_TEAMS);
			if (!sel) {
				destroy_sdl_ctx(&ctx);
				return 1;
			}
			Sim *sim = init_sim(al_teams, nl_teams, sel);
			ctx.theme.title = team_color_lookup(sim->selected_team->short_name);
			sdl_season_ui(&ctx, sim);
			break;
		case 1:
			// Exit
			exit(EXIT_SUCCESS);
	}
	return 0;
}
