#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "team.h"
#include "player.h"
#include "utils.h"
#include "sim.h"

Team *init_team(const char* name, const char* short_name) {
	Team *team = calloc(1, sizeof(Team));  // zero initializes everything
	team->name = strdup(name);
	team->short_name = strdup(short_name);

	gen_roster(team);
	return team;
}

Team **init_al_teams() {
	Team **teams = calloc(N_AL_TEAMS, sizeof(Team*));
	for (int t = 0; t < N_AL_TEAMS; t++) {
		teams[t] = init_team(team_names[t], team_short_names[t]);
	}
	return teams;
}

Team **init_nl_teams() {
	Team **teams = calloc(N_NL_TEAMS, sizeof(Team*));
	for (int t = 0; t < N_NL_TEAMS; t++) {
		size_t index = N_AL_TEAMS + t;  // team names are ordered AL first
		teams[t] = init_team(team_names[index], team_short_names[index]);
	}
	return teams;
}

bool insert_hitter(Team *team, Hitter *hitter) {
	if (team->n_hitters >= MAX_HITTERS) return false;
	team->hitters[team->n_hitters] = hitter;
	team->n_hitters += 1;
	return true;
}

bool insert_pitcher(Team *team, Pitcher *pitcher) {
	if (team->n_pitchers >= MAX_PITCHERS) return false;
	team->pitchers[team->n_pitchers] = pitcher;
	team->n_pitchers += 1;
	return true;
}

bool swap_hitter(Team *team, size_t index, Hitter *h) {
	if (!team || !h || index >= team->n_hitters) return false;
	Hitter *old = team->hitters[index];
	team->hitters[index] = h;
	if (old) free_hitter(old);
	return true;
};

bool swap_pitcher(Team *team, size_t index, Pitcher *p) {
	if (!team || !p || index >= team->n_pitchers) return false;
	Pitcher *old = team->pitchers[index];
	team->pitchers[index] = p;
	if (old) free_pitcher(old);
	return true;
}

void gen_roster(Team *team) {
	// populate roster with random generated players
	for (size_t h = 0; h < MAX_HITTERS; h++) {
		Hitter *new_hitter = gen_hitter(random_int_range(18, 36));
		if (!insert_hitter(team, new_hitter)) {
			return;
		}
	}

	for (size_t p = 0; p < MAX_PITCHERS; p++) {
		Pitcher *new_pitcher = gen_pitcher(random_int_range(18, 36));
		if (!insert_pitcher(team, new_pitcher)) {
			return;
		}
	}
}

void destroy_team(Team *team) {
	free((void *)team->name);
	free((void *)team->short_name);
	team->name = team->short_name = NULL;
	for (int i = 0; i < team->n_hitters; i++) {
		free(team->hitters[i]);
		team->hitters[i] = NULL;
	}

	for (int j = 0; j < team->n_pitchers; j++) {
		free(team->pitchers[j]);
		team->pitchers[j] = NULL;
	}
}

// these functions get the average of the ratings of the hitter and pitcher groups
// of a team, used as a hidden rating to determine game outcomes
unsigned int get_hitters_rating(Team *team) {
	unsigned int total = 0;
	for (int i = 0; i < team->n_hitters; i++) {
		Hitter *h = team->hitters[i];
		total += total_hitter_rating(h);
	}
	return total / team->n_hitters;
}

unsigned int get_pitchers_rating(Team *team) {
	unsigned int total = 0;
	for (int i = 0; i < team->n_pitchers; i++) {
		Pitcher *p = team->pitchers[i];
		total += total_pitcher_rating(p);
	}
	return total / team->n_pitchers;
}

void automatic_roster_changes(Team *team) {
	const unsigned int PITCHER_REPLACEMENT_THRESHOLD = 100;
	const unsigned int HITTER_REPLACEMENT_THRESHOLD = 120;
	unsigned int replaced_hitters = 0;
	unsigned int replaced_pitchers = 0;

	for (size_t j = 0; j < team->n_hitters && replaced_hitters < 3; j++) {
		Hitter *h = team->hitters[j];
		if (total_hitter_rating(h) <= HITTER_REPLACEMENT_THRESHOLD) {
			team->hitters[j] = gen_hitter(random_int_range(18, 24));
			replaced_hitters++;
			free_hitter(h);
		}
	}

	for (size_t j = 0; j < team->n_pitchers && replaced_pitchers < 2; j++) {
		Pitcher *p = team->pitchers[j];
		if (total_pitcher_rating(p) <= PITCHER_REPLACEMENT_THRESHOLD) {
			team->pitchers[j] = gen_pitcher(random_int_range(18, 24));
			replaced_pitchers++;
			free_pitcher(p);
		}
	}
}
