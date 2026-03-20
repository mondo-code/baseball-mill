#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "sim.h"
#include "player.h"
#include "team.h"
#include "utils.h"

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
	HitterStats *hs = &(h->stats);
	// yes, sorry, these constants used to make the stats more realistic are magic numbers
	// they don't really represent anything so it would be awkward to try to name them as constants
	// define probabilities for binomial draws
	double p_walk_base = clamp(0.02 + 0.0015 * h->ratings.eye);
	// this keeps elite players from getting wildly unrealistic walk rates
	double p_walk = clamp(p_walk_base * (1.0 - 0.3 * (h->ratings.eye / 99.0)));
	double p_hit = clamp(avgBA + 0.003 * h->ratings.contact);
	double p_hr = clamp(0.02 + 0.002 * h->ratings.power);
	double p_sb = clamp(0.02 + 0.002 * h->ratings.speed);
	double p_rbi = clamp(0.02 + 0.005 * h->ratings.power);

	// generate counting stats for this sim
	unsigned int plate_appearances = random_int_range(4, 5);
	unsigned int walks = binom_draw(plate_appearances, p_walk);
	unsigned int at_bats = plate_appearances - walks;
	unsigned int hits = binom_draw(at_bats, p_hit);
	unsigned int homers = binom_draw(hits, p_hr);
	unsigned int xb = hits - homers;
	unsigned int doubles = (unsigned int)xb * w2B;
	unsigned int triples = (unsigned int)xb * w3B;
	unsigned int on_base = hits + walks;
	unsigned int stolen_bases = binom_draw(on_base, p_sb);
	unsigned int rbi = homers + binom_draw(hits, p_rbi);

	// increment counting stats
	hs->GP++;
	hs->PA += plate_appearances;
	hs->AB += (plate_appearances - walks);
	hs->BB += walks;
	hs->H += hits;
	hs->H2 += doubles;
	hs->H3 += triples;
	hs->HR += homers;
	hs->SB += stolen_bases;
	hs->RBI += rbi;

	// recalculate average stats
	if (hs->AB > 0) {
		hs->AVG = (double)hs->H / hs->AB;
		hs->OBP = (double)(hs->H + hs->BB) / (hs->AB + hs->BB);
		hs->SLG = (double)((hs->H - hs->H2 - hs->H3 - hs->HR)
				+ 2*hs->H2 + 3*hs->H3 + 4*hs->HR) / hs->AB;
	} else {
		hs->AVG = 0.0;
		hs->OBP = 0.0;
		hs->SLG = 0.0;
	}
	hs->OPS = hs->OBP + hs->SLG;
}

void sim_pitcher_stats(Pitcher *p) {
	PitcherStats *ps = &(p->stats);
	// generate probabilities
	unsigned int bf = round(15.0 + 0.1 * p->ratings.stamina);
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
	unsigned int walks = binom_draw(bf, p_walk);
	unsigned int contact_sample = bf - walks;
	unsigned int contact_outs = binom_draw(contact_sample, p_contact_out);
	unsigned int strikeout_sample = contact_sample - contact_outs;
	unsigned int strikeouts = binom_draw(strikeout_sample, p_strikeout);
	unsigned int outs = contact_outs + strikeouts;
	unsigned int hits_allowed = bf - (walks + outs);
	unsigned int runs = binom_draw(hits_allowed + walks, p_runs);
	Innings ip = { .whole = outs / 3, .thirds = outs % 3 };
	add_innings(&ps->IP, &ip);

	// increment counting, recalculate average stats
	ps->BBA += walks;
	ps->ER += runs;
	ps->GS++;
	ps->BF += bf;
	ps->HA += hits_allowed;
	ps->SO += strikeouts;
    double ip_total = (double)ps->IP.whole + (ps->IP.thirds / 3.0);
    ps->ERA = (ip_total > 0.0) ? (ps->ER / ip_total) * 9.0 : 0.0;
}

Sim *init_sim(Team **al_teams, Team **nl_teams, const char *selected_team) {
	Sim *s = calloc(1, sizeof(Sim));
	s->al_teams = al_teams;
	s->nl_teams = nl_teams;
	s->al_team_count = N_AL_TEAMS;
	s->nl_team_count = N_NL_TEAMS;
	s->month = APRIL;

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
	unsigned int t1_wins = 0;
	unsigned int t2_wins = 0;
	unsigned int max_wins = (MAX_SERIES_MATCHES / 2) + 1;
	size_t i;
	for (i = 0; i < MAX_SERIES_MATCHES && t1_wins < max_wins && t2_wins < max_wins; i++) {
		sim_match(&s->matches[i]);
		t1_wins = t1->wins - t1_reg_wins; 
		t2_wins = t2->wins - t2_reg_wins;
	}
	s->n_matches = i;
}

void sim_month(Sim *sim) {
	if (sim->month == OCTOBER) return;
	gen_month_schedule(sim);
	for (size_t g = 0; g < sim->n_matches; g++) {
		Match *m = &sim->matches[g];
		sim_match(m);
	}
	quicksort_teams(sim->al_teams, 0, N_AL_TEAMS-1);
	quicksort_teams(sim->nl_teams, 0, N_NL_TEAMS-1);
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
