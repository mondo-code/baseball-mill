#ifndef TEAM_H
#define TEAM_H

#include <stddef.h>
#include <stdbool.h>
#include "player.h"

#define MAX_HITTERS 5
#define MAX_PITCHERS 4

typedef struct Team {
    const char* name;
	const char* short_name;
    Hitter* hitters[MAX_HITTERS];
	Pitcher* pitchers[MAX_PITCHERS];
	size_t n_hitters;
	size_t n_pitchers;
	size_t next_pitcher;
	unsigned int wins;
	unsigned int losses;
} Team;

Team *init_team(const char* name, const char* short_name);
Team **init_al_teams();
Team **init_nl_teams();
bool insert_hitter(Team *team, Hitter *hitter);
bool insert_pitcher(Team *team, Pitcher *pitcher);
bool swap_hitter(Team *team, size_t index, Hitter *h);
bool swap_pitcher(Team *team, size_t index, Pitcher *p);
void gen_roster(Team *team);
void destroy_team(Team *team);
unsigned int get_hitters_rating(Team *team);
unsigned int get_pitchers_rating(Team *team);
void automatic_roster_changes(Team *team);
void quicksort_teams(Team **teams, int low, int high);

#endif  // TEAM_H
