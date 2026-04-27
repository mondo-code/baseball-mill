#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "sim.h"
#include "player.h"
#include "team.h"
#include "utils.h"

Sim *init_sim(Team **al_teams, Team **nl_teams, const char *selected_team) {
	Sim *s = calloc(1, sizeof(Sim));
	s->al_teams = al_teams;
	s->nl_teams = nl_teams;
	s->al_team_count = N_AL_TEAMS;
	s->nl_team_count = N_NL_TEAMS;
	s->month = APRIL;
	s->current_year = 2000;
	s->sel_ws_won = 0;
	s->n_top_hitters = 0;
	s->n_top_pitchers = 0;

	for (int i = 0; i < N_AL_TEAMS; i++) {
		if (strcmp(s->al_teams[i]->name, selected_team) == 0) {
			s->selected_team = s->al_teams[i];
		}
	}

	if (!s->selected_team) {
		for (int i = 0; i < N_NL_TEAMS; i++) {
			if (strcmp(s->nl_teams[i]->name, selected_team) == 0) {
				s->selected_team = s->nl_teams[i];
			}
		}
	}

	if (!s->selected_team) { perror("Error while designating selected team"); exit(1); }
	return s;
}

// weighted splits for doubles, triples, homers for random gen
const double w2B = 0.60, w3B = 0.05;
const double avgBA = 0.240;
const char* team_names[] = TEAM_NAMES;
const char* team_short_names[] = TEAM_SHORT_NAMES;

int binom_draw(int n, double p) {
    int k = 0;
    for (int i = 0; i < n; i++)
        if ((double)rand() / RAND_MAX < p) k++;
    return k;
}

void sim_hitter_stats(Hitter *h) {
	HitterStats game_stats = (HitterStats){0};
	HitterStats *hs = &(h->season_stats);
	// yes, sorry, these constants used to make the stats more realistic are magic numbers
	// they don't really represent anything so it would be awkward to try to name them as constants
	// define probabilities for binomial draws
	double p_walk_base = clamp(0.02 + 0.0015 * h->ratings.eye);
	// this keeps elite players from getting wildly unrealistic walk rates
	double p_walk = clamp(p_walk_base * (1.0 - 0.3 * (h->ratings.eye / 99.0)));
	double p_hit = clamp(avgBA + 0.002 * h->ratings.contact);
	double p_hr = clamp(0.02 + 0.001 * h->ratings.power);
	double p_sb = clamp(0.02 + 0.002 * h->ratings.speed);
	double p_rbi = clamp(0.02 + 0.005 * h->ratings.power);

	// generate counting stats for this sim
	game_stats.GP = 1;
	game_stats.PA = 4;
	game_stats.BB = binom_draw(game_stats.PA, p_walk);
	game_stats.AB = game_stats.PA - game_stats.BB;
	game_stats.H = binom_draw(game_stats.AB, p_hit);
	game_stats.HR = binom_draw(game_stats.H, p_hr);
	unsigned int xb = game_stats.H - game_stats.HR;
	game_stats.H2 = (unsigned int)xb * w2B;
	game_stats.H3 = (unsigned int)xb * w3B;
	unsigned int on_base = game_stats.H + game_stats.BB;
	game_stats.SB = binom_draw(on_base, p_sb);
	game_stats.RBI = game_stats.HR + binom_draw(game_stats.H, p_rbi);

	add_hitter_stats(hs, &game_stats);
}

void sim_pitcher_stats(Pitcher *p) {
	PitcherStats game_stats = (PitcherStats){0};
	PitcherStats *ps = &(p->season_stats);
	// generate probabilities
	game_stats.GS = 1;
	game_stats.BF = round(15.0 + 0.1 * p->ratings.stamina);
	// generate probability of contact outs and strikeouts, binomial draw them
	// both over BF. contact outs should be a LC w/ command and strikeouts
	// should be a LC w/ stuff

	double p_walk = clamp(0.16 - 0.0016 * p->ratings.command);
	if (p_walk == 0.0) { p_walk = 0.06; }
	double p_contact_out = clamp(0.4 + 0.002 * p->ratings.command);
	if (p_contact_out > 0.5) { p_contact_out = 0.5; }
	double p_strikeout = clamp(0.08 + 0.005 * p->ratings.stuff);
	if (p_strikeout > 0.5) { p_strikeout = 0.5; }
	if (p_strikeout < 0.15) { p_strikeout = 0.15; }
	double p_runs = clamp(0.7 - (0.005 * p->ratings.stuff) - (0.005 * p->ratings.command));
	if (p_runs < 0.2) { p_runs = 0.2; }

	// binomial draws and stats for this sim
	game_stats.BBA = binom_draw(game_stats.BF, p_walk);
	unsigned int contact_sample = game_stats.BF - game_stats.BBA;
	unsigned int contact_outs = binom_draw(contact_sample, p_contact_out);
	unsigned int strikeout_sample = contact_sample - contact_outs;
	game_stats.SO = binom_draw(strikeout_sample, p_strikeout);
	unsigned int outs = contact_outs + game_stats.SO;
	game_stats.HA = game_stats.BF - (game_stats.BBA + outs);
	game_stats.ER = binom_draw(game_stats.HA + game_stats.BBA, p_runs);
	Innings ip = { .whole = outs / 3, .thirds = outs % 3 };
	game_stats.IP = ip;

	add_pitcher_stats(ps, &game_stats);
}

void sim_match(Match *m) {
	const double average_runs = 4;
	const double hitting_impact = 1.2;
	const double pitching_impact = 1.2;
	unsigned int t1_hitting = get_hitters_rating(m->t1);
	unsigned int t1_pitching = get_pitchers_rating(m->t1);
	unsigned int t2_hitting = get_hitters_rating(m->t2);
	unsigned int t2_pitching = get_pitchers_rating(m->t2);

	double expected_runs_t1 = sqrt(average_runs + hitting_impact * (t1_hitting / 100.0)
								- pitching_impact * (t2_pitching / 100.0));
	double expected_runs_t2 = sqrt(average_runs + hitting_impact * (t2_hitting / 100.0)
								- pitching_impact * (t1_pitching / 100.0));

	if (expected_runs_t1 < 0) expected_runs_t1 = 0;
	if (expected_runs_t2 < 0) expected_runs_t2 = 0;
	if (expected_runs_t1 > 10) expected_runs_t1 = 10;
	if (expected_runs_t2 > 10) expected_runs_t2 = 10;

	double p1 = expected_runs_t1 / 9.0;
	double p2 = expected_runs_t2 / 9.0;
	if (p1 < 0.0) p1 = 0.0;
	if (p2 < 0.0) p2 = 0.0;
	if (p1 > 1.0) p1 = 1.0;
	if (p2 > 1.0) p2 = 1.0;

draw_sim:
	unsigned int runs1 = binom_draw(9, p1);
	unsigned int runs2 = binom_draw(9, p2);

	m->t1_runs = runs1;
	m->t2_runs = runs2;

	for (size_t h = 0; h < m->t1->n_hitters; h++)
		sim_hitter_stats(m->t1->hitters[h]);
	sim_pitcher_stats(m->t1->pitchers[m->t1->next_pitcher]);
	m->t1->next_pitcher = (m->t1->next_pitcher >= m->t1->n_pitchers-1) ? 0 : (m->t1->next_pitcher + 1);

	for (size_t h = 0; h < m->t2->n_hitters; h++)
		sim_hitter_stats(m->t2->hitters[h]);
	sim_pitcher_stats(m->t2->pitchers[m->t2->next_pitcher]);
	m->t2->next_pitcher = (m->t2->next_pitcher >= m->t2->n_pitchers-1) ? 0 : (m->t2->next_pitcher + 1);

	if (runs1 > runs2) {
		m->t1->wins++;
		m->t2->losses++;
	} else if (runs1 < runs2) {
		m->t1->losses++;
		m->t2->wins++;
	} else {
		goto draw_sim;
	}
}

void sim_series(Series *s) {
	Team *t1 = s->matches[0].t1;
	Team *t2 = s->matches[0].t2;
	unsigned int t1_reg_wins = t1->wins;
	unsigned int t2_reg_wins = t2->wins;
	s->t1_wins = 0;
	s->t2_wins = 0;
	unsigned int max_wins = (MAX_SERIES_MATCHES / 2) + 1;
	size_t i;
	for (i = 0; i < MAX_SERIES_MATCHES && s->t1_wins < max_wins && s->t2_wins < max_wins; i++) {
		sim_match(&s->matches[i]);
		s->t1_wins = t1->wins - t1_reg_wins;
		s->t2_wins = t2->wins - t2_reg_wins;
	}
	s->n_matches = i;
}

static int comp_teams(const void *a, const void *b) {
	const Team *t1 = *(const Team **)a;
	const Team *t2 = *(const Team **)b;
	// descending order
	return (t2->wins > t1->wins) - (t2->wins < t1->wins);
}

void sim_month(Sim *sim) {
	if (sim->month == OCTOBER) return;
	gen_month_schedule(sim);
	for (size_t g = 0; g < sim->n_matches; g++) {
		Match *m = &sim->matches[g];
		sim_match(m);
	}
	qsort(sim->al_teams, N_AL_TEAMS, sizeof(Team*), comp_teams);
	qsort(sim->nl_teams, N_NL_TEAMS, sizeof(Team*), comp_teams);
	advance_month(sim);
}

void advance_month(Sim *sim) {
	switch (sim->month) {
		case APRIL:
			sim->month = MAY;
			break;
		case MAY:
			sim->month = JUNE;
			break;
		case JUNE:
			sim->month = JULY;
			break;
		case JULY:
			sim->month = AUGUST;
			break;
		case AUGUST:
			sim->month = SEPTEMBER;
			break;
		case SEPTEMBER:
			sim->month = OCTOBER;
			break;
		case OCTOBER:
			break;
	}
}

static void shuffle(size_t *arr, size_t n) {
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        size_t tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
    }
}

void gen_month_schedule(Sim *sim) {
    sim->n_matches = 0;
	size_t n_teams = sim->al_team_count + sim->nl_team_count;
	// put AL and NL team pointers into one array to be sorted. at the end, each array
	// will be separately sorted as well
	Team *all_teams[n_teams];
	for (size_t i = 0; i < sim->al_team_count; i++)
		all_teams[i] = sim->al_teams[i];
	for (size_t i = 0; i < sim->nl_team_count; i++)
		all_teams[sim->al_team_count + i] = sim->nl_teams[i];

	size_t remaining[n_teams];
	for (size_t i = 0; i < n_teams; i++) remaining[i] = GAMES_PER_TEAM;

    size_t attempts = 0;
    while (sim->n_matches < GAMES_PER_MONTH && attempts++ < 100000) {
        // each team appears once per remaining game needed
        size_t pool[GAMES_PER_TEAM * N_TEAMS];
        size_t pool_size = 0;
        for (size_t i = 0; i < n_teams; i++)
            for (size_t r = 0; r < remaining[i]; r++)
                pool[pool_size++] = i;

        shuffle(pool, pool_size);

        int used[pool_size];
        memset(used, 0, sizeof(int) * pool_size);

        for (size_t i = 0; i < pool_size && sim->n_matches < GAMES_PER_MONTH; i++) {
            if (used[i]) continue;
            for (size_t j = i + 1; j < pool_size; j++) {
                if (used[j] || pool[j] == pool[i]) continue;
                sim->matches[sim->n_matches++] = (Match){
                    all_teams[pool[i]],
                    all_teams[pool[j]]
                };
                used[i] = used[j] = 1;
                remaining[pool[i]]--;
                remaining[pool[j]]--;
                break;
            }
        }
    }
}

// helper for setting team records to 0 and resetting player stats
static void reset_teams_stats(Team **teams, size_t n_teams) {
	for (int i = 0; i < n_teams; i++) {
		Team *t = teams[i];
		t->wins = 0;
		t->losses = 0;
		for (int j = 0; j < t->n_hitters; j++) {
			Hitter *h = t->hitters[j];
			memset(&h->season_stats, 0, sizeof(HitterStats));
		}
		for (int j = 0; j < t->n_pitchers; j++) {
			Pitcher *p = t->pitchers[j];
			memset(&p->season_stats, 0, sizeof(PitcherStats));
		}
	}
}

// comparison functions for qsort
static int comp_top_hitters(const void *a, const void *b) {
	const Hitter *h1 = *(const Hitter **)a;
	const Hitter *h2 = *(const Hitter **)b;
	return (h2->career_stats.H > h1->career_stats.H) - (h2->career_stats.H < h1->career_stats.H);
}

static int comp_top_pitchers(const void *a, const void *b) {
	const Pitcher *h1 = *(const Pitcher **)a;
	const Pitcher *h2 = *(const Pitcher **)b;
	return (h2->career_stats.SO > h1->career_stats.SO) - (h2->career_stats.SO < h1->career_stats.SO);
}

// helpers for inserting players into the top players list by insertion sort approach
static void update_top_hitters(Sim *sim) {
	qsort(sim->top_hitters, sim->n_top_hitters, sizeof(Hitter*), comp_top_hitters);
	Team *sel = sim->selected_team;
	for (int i = 0; i < sel->n_hitters; i++) {
		Hitter *h = sel->hitters[i];

		// find insertion index
		int j = sim->n_top_hitters - 1;
		while (j >= 0 && h->career_stats.H > sim->top_hitters[j]->career_stats.H) j--;
		int ins = j + 1;
		if (ins >= N_TOP_PLAYERS) continue;

		// check if player is already in the top list and remove them first
		for (int k = 0; k < sim->n_top_hitters; k++) {
			if (sim->top_hitters[k]->base.id == h->base.id) {
				// shift everyone above k down
				for (int m = k; m < sim->n_top_hitters - 1; m++)
					sim->top_hitters[m] = sim->top_hitters[m+1];
				sim->n_top_hitters--;
				if (ins > 0) ins--;
				break;
			}
		}

		int end = sim->n_top_hitters < N_TOP_PLAYERS ? sim->n_top_hitters : N_TOP_PLAYERS - 1;
		for (int k = end; k > ins; k--)
			sim->top_hitters[k] = sim->top_hitters[k-1];

		// copying ensures no dangling pointer if the player owned by the team is freed
		sim->top_hitters[ins] = copy_hitter(h);
		if (sim->n_top_hitters < N_TOP_PLAYERS) sim->n_top_hitters++;
	}
}

static void update_top_pitchers(Sim *sim) {
	qsort(sim->top_pitchers, sim->n_top_pitchers, sizeof(Pitcher*), comp_top_pitchers);
	Team *sel = sim->selected_team;
	for (int i = 0; i < sel->n_pitchers; i++) {
		Pitcher *p = sel->pitchers[i];

		// find insertion index
		int j = sim->n_top_pitchers - 1;
		while (j >= 0 && p->career_stats.SO > sim->top_pitchers[j]->career_stats.SO) j--;
		int ins = j + 1;
		if (ins >= N_TOP_PLAYERS) continue;

		// check if player is already in the top list and remove them first
		for (int k = 0; k < sim->n_top_pitchers; k++) {
			if (sim->top_pitchers[k]->base.id == p->base.id) {
				// shift everyone above k down
				for (int m = k; m < sim->n_top_pitchers - 1; m++)
					sim->top_pitchers[m] = sim->top_pitchers[m+1];
				sim->n_top_pitchers--;
				if (ins > 0) ins--;
				break;
			}
		}

		int end = sim->n_top_pitchers < N_TOP_PLAYERS ? sim->n_top_pitchers : N_TOP_PLAYERS - 1;
		for (int k = end; k > ins; k--)
			sim->top_pitchers[k] = sim->top_pitchers[k-1];

		// sim->top_pitchers[ins] = p;
		sim->top_pitchers[ins] = copy_pitcher(p);
		if (sim->n_top_pitchers < N_TOP_PLAYERS) sim->n_top_pitchers++;
	}
}

void reset_season_stats(Sim *sim) {
	// add to career stats for user team
	// it's pointless to do this for other teams because it's never shown
	Team *user = sim->selected_team;
	for (int i = 0; i < user->n_hitters; i++) {
		Hitter *h = user->hitters[i];
		add_hitter_stats(&h->career_stats, &h->season_stats);
	}

	for (int i = 0; i < user->n_pitchers ; i++) {
		Pitcher *p = user->pitchers[i];
		add_pitcher_stats(&p->career_stats, &p->season_stats);
	}

	// reset records
	reset_teams_stats(sim->al_teams, N_AL_TEAMS);
	reset_teams_stats(sim->nl_teams, N_NL_TEAMS);
	update_top_hitters(sim);
	update_top_pitchers(sim);

	// automated roster changes for non-user teams
	for (int i = 0; i < N_AL_TEAMS; i++)
		if (sim->al_teams[i] != sim->selected_team) automatic_roster_changes(sim->al_teams[i]);
	for (int i = 0; i < N_NL_TEAMS; i++)
		if (sim->nl_teams[i] != sim->selected_team) automatic_roster_changes(sim->nl_teams[i]);

	sim->month = APRIL;
	sim->current_year++;
}

const char *month_str(Month m) {
	switch (m) {
		case APRIL:
			return "April";
		case MAY:
			return "May";
		case JUNE:
			return "June";
		case JULY:
			return "July";
		case AUGUST:
			return "August";
		case SEPTEMBER:
			return "September";
		case OCTOBER:
			return "October";
	}
	return "None";
}

void age_curve_players(Sim *sim) {
    for (int t = 0; t < sim->al_team_count; t++) {
        Team *team = sim->al_teams[t];
        for (int h = 0; h < team->n_hitters; h++) {
            Hitter *hitter = team->hitters[h];
            hitter->base.age += 1;
            hitter_age_curve(hitter);
        }
        for (int p = 0; p < team->n_pitchers; p++) {
            Pitcher *pitcher = team->pitchers[p];
            pitcher->base.age += 1;
            pitcher_age_curve(pitcher);
        }
    }

    for (int t = 0; t < sim->nl_team_count; t++) {
        Team *team = sim->nl_teams[t];
        for (int h = 0; h < team->n_hitters; h++) {
            Hitter *hitter = team->hitters[h];
            hitter->base.age += 1;
            hitter_age_curve(hitter);
        }
        for (int p = 0; p < team->n_pitchers; p++) {
            Pitcher *pitcher = team->pitchers[p];
            pitcher->base.age += 1;
            pitcher_age_curve(pitcher);
        }
    }
}
