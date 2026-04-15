#ifndef SIM_H
#define SIM_H

#include <math.h>
#include "player.h"
#include "team.h"

// eventually, it would make sense to decouple these default values from the
// rest of the codebase and allow this to be customized. for now I only intend
// for these teams to be in the game.
// these are ordered AL first, then NL
#define TEAM_NAMES { "Athletics", "Orioles", "Red Sox", "Tigers", "Twins", "Mariners", "White Sox", "Yankees", "Braves", "Cubs", "Cardinals", "Dodgers", "Giants", "Mets", "Phillies", "Reds" }
#define TEAM_SHORT_NAMES { "OAK", "BAL", "BOS", "DET", "MIN", "SEA", "CHW", "NYY", "ATL", "CHC", "STL", "LAD", "SFG", "NYM", "PHI", "CIN" }
#define N_TEAMS 16
#define N_AL_TEAMS 8
#define N_NL_TEAMS 8
#define N_MONTHS 6
#define N_GAMES 162
#define GAMES_PER_MONTH 216
#define GAMES_PER_TEAM 27
#define MAX_SERIES_MATCHES 7

extern const char *team_names[];
extern const char *team_short_names[];

typedef enum {
	APRIL,
	MAY,
	JUNE,
	JULY,
	AUGUST,
	SEPTEMBER,
	OCTOBER
} Month;

typedef struct {
	Team *t1;
	Team *t2;
	unsigned int t1_runs;
	unsigned int t2_runs;
} Match;

typedef struct {
	Match matches[MAX_SERIES_MATCHES];
	size_t n_matches;
	unsigned int t1_wins;
	unsigned int t2_wins;
} Series;

typedef struct {
	Team **al_teams;
	Team **nl_teams;
	Team *selected_team;
	size_t al_team_count;
	size_t nl_team_count;
	Month month;
	Match matches[GAMES_PER_MONTH];
	size_t n_matches;
	unsigned int current_year;
} Sim;

int binom_draw(int n, double p);
void sim_hitter_stats(Hitter *h);
void sim_pitcher_stats(Pitcher *p);
Sim *init_sim(Team **al_teams, Team **nl_teams, const char *selected_team);
void sim_match(Match *m);
void sim_series(Series *s);
void sim_month(Sim *sim);
void advance_month(Sim *sim);
void gen_opponents(Sim *s, size_t n_opponents);
void gen_month_schedule(Sim *sim);
void sim_offseason(Sim *sim);
void age_curve_players(Sim *sim);
const char *month_str(Month m);

#endif // SIM_H
