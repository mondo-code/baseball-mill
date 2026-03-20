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
	team->hitters = calloc(MAX_HITTERS, sizeof(Hitter));
	team->pitchers = calloc(MAX_PITCHERS, sizeof(Pitcher));

	if (!team->hitters || !team->pitchers) {
		free((void *)team->name);
		free((void *)team->short_name);
		free(team->hitters);
		free(team->pitchers);
		free(team);
		return NULL;
	}

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

void gen_roster(Team *team) {
	// populate roster with random generated players
	for (size_t h = 0; h < MAX_HITTERS; h++) {
		Hitter *new_hitter = gen_hitter(file_random_line("firstnames.txt"), file_random_line("lastnames.txt"), random_int_range(18, 36));
		if (!insert_hitter(team, new_hitter)) {
			return;
		}
	}

	for (size_t p = 0; p < MAX_PITCHERS; p++) {
		Pitcher *new_pitcher = gen_pitcher(file_random_line("firstnames.txt"), file_random_line("lastnames.txt"), random_int_range(18, 36));
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
	free(team->hitters);
	free(team->pitchers);
	team->hitters = NULL;
	team->pitchers = NULL;
}

// these functions get the sum of the ratings of the hitter and pitcher groups
// of a team, used as a hidden rating to determine game outcomes
unsigned int get_hitters_rating(Team *team) {
	unsigned int total = 0;
	for (int i = 0; i < team->n_hitters; i++) {
		Hitter *h = team->hitters[i];
		total += (h->ratings.contact + h->ratings.eye + h->ratings.power + h->ratings.speed);
	}
	return total;
}

unsigned int get_pitchers_rating(Team *team) {
	unsigned int total = 0;
	for (int i = 0; i < team->n_pitchers; i++) {
		Pitcher *p = team->pitchers[i];
		total += (p->ratings.command + p->ratings.stamina + p->ratings.stuff);
	}
	return total;
}

// functions for descending order quicksort
void swap_teams(Team **teams, int i, int j) {
	Team *temp = teams[i];
	teams[i] = teams[j];
	teams[j] = temp;
}

int partition_teams(Team **teams, int low, int high) {
	Team *pivot = teams[high];
	int i = low - 1;

	for (int j = low; j < high; j++) {
		if (teams[j]->wins > pivot->wins) {
			i += 1;
			swap_teams(teams, i, j);
		}
	}

	swap_teams(teams, i+1, high);
	return i + 1;
}

void quicksort_teams(Team **teams, int low, int high) {
	if (low < high) {
		int p = partition_teams(teams, low, high);
		quicksort_teams(teams, low, p-1);
		quicksort_teams(teams, p+1, high);
	}
}
