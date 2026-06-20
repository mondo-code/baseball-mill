#ifndef PLAYER_H
#define PLAYER_H

#define N_HITTER_RATINGS 4
#define N_PITCHER_RATINGS 3

typedef struct {
	unsigned int id;
    const char *first_name;
    const char *last_name;
	unsigned int age;
    unsigned int jersey_number;
} Player;

typedef struct {
	unsigned int contact, power, eye, speed;
} HitterRatings;

typedef struct {
	unsigned int GP, PA, AB, H, H2, H3, HR, BB, SB, RBI;
	double AVG, OBP, SLG, OPS;
} HitterStats;

typedef struct {
	Player base;
	HitterRatings ratings;
	HitterStats season_stats;
	HitterStats career_stats;
} Hitter;

typedef struct {
	unsigned int command, stuff, stamina;
} PitcherRatings;

typedef struct {
	unsigned int whole;
	unsigned int thirds;
} Innings;

typedef struct {
	unsigned int GS, BF, HA, ER, BBA, SO;
	Innings IP;
	double AVGA, WHIP, ERA;
} PitcherStats;

typedef struct {
	Player base;
	PitcherRatings ratings;
	PitcherStats season_stats;
	PitcherStats career_stats;
} Pitcher;

Hitter *gen_hitter(unsigned int age);
Pitcher *gen_pitcher(unsigned int age);
Hitter *copy_hitter(const Hitter *h);
Pitcher *copy_pitcher(const Pitcher *p);
void free_hitter(Hitter *h);
void free_pitcher(Pitcher *p);
void add_innings(Innings *a, Innings *b);
void hitter_age_curve(Hitter *h);
void pitcher_age_curve(Pitcher *p);
void add_hitter_stats(HitterStats *a, HitterStats *b);
void add_pitcher_stats(PitcherStats *a, PitcherStats *b);
double innings_to_double(Innings *i);
unsigned int total_hitter_rating(Hitter *h);
unsigned int total_pitcher_rating(Pitcher *p);

#endif  // PLAYER_H
