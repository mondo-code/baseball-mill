#ifndef PLAYER_H
#define PLAYER_H

#define N_HITTER_RATINGS 4
#define N_PITCHER_RATINGS 3

typedef struct {
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
	HitterStats stats;
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
	double AVGA, ERA;
	Innings IP;
} PitcherStats;

typedef struct {
	Player base;
	PitcherRatings ratings;
	PitcherStats stats;
} Pitcher;

Hitter *gen_hitter(unsigned int age);
Pitcher *gen_pitcher(unsigned int age);
void free_hitter(Hitter *h);
void free_pitcher(Pitcher *p);
void add_innings(Innings *a, Innings *b);

#endif  // PLAYER_H
