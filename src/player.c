#include <stdint.h>
#include <stdlib.h>
#include "player.h"
#include "utils.h"

Hitter *gen_hitter(unsigned int age) {
	Hitter *h = malloc(sizeof(Hitter));
	h->base = (Player){0};
	h->base.first_name = file_random_line("firstnames.txt");
	h->base.last_name = file_random_line("lastnames.txt");
	h->base.age = age;

	HitterRatings hr = {0};
	hr.contact = random_int_range(1, 99);
	hr.power = random_int_range(1, 99);
	hr.eye = random_int_range(1, 99);
	hr.speed = random_int_range(1, 99);
	h->ratings = hr;

	HitterStats hs = {0};
	h->stats = hs;

	return h;
}

Pitcher *gen_pitcher(unsigned int age) {
	Pitcher *p = malloc(sizeof(Pitcher));
	p->base = (Player){0};
	p->base.first_name = file_random_line("firstnames.txt");
	p->base.last_name = file_random_line("lastnames.txt");
	p->base.age = age;

	PitcherRatings pr = {0};
	pr.command = random_int_range(1, 99);
	pr.stuff = random_int_range(1, 99);
	pr.stamina = random_int_range(1, 99);
	p->ratings = pr;

	PitcherStats ps = {0};
	p->stats = ps;

	return p;
}

void free_hitter(Hitter *h) {
	free((void *)h->base.first_name);
	free((void *)h->base.last_name);
	free(h);
}

void free_pitcher(Pitcher *p) {
	free((void *)p->base.first_name);
	free((void *)p->base.last_name);
	free(p);
}

void add_innings(Innings *a, Innings *b) {
	a->whole += b->whole;
	unsigned int new_third = a->thirds + b->thirds;
	a->whole += new_third / 3;
	a->thirds = new_third % 3;
}

// simplified implementation of baseball's aging curve adapted to player ratings
// age multiplier: f(delta) = 1.0 - rate * delta^2
static float age_multiplier(unsigned int age, unsigned int peak_age, float decline_rate) {
	float delta = (float)age - (float)peak_age;
	float rate = (delta < 0) ? decline_rate * 0.5f : decline_rate;
	float mult = 1.0f - rate * (delta * delta);
	if (mult > 1.0f) mult = 1.0f;
	if (mult < 0.3f) mult = 0.3f;
	return mult;
}

void hitter_age_curve(Hitter *h) {
    unsigned int age = h->base.age;

	// certain stats are meant to decline slower with age to be more realistic 
	float contact_mult = age_multiplier(age, 20.0f, 0.0020f);
    float power_mult   = age_multiplier(age, 29.0f, 0.0020f);
    float eye_mult     = age_multiplier(age, 30.0f, 0.0010f);
    float speed_mult   = age_multiplier(age, 26.0f, 0.0030f);

    h->ratings.contact = (unsigned int)(h->ratings.contact * contact_mult);
    h->ratings.power   = (unsigned int)(h->ratings.power   * power_mult);
    h->ratings.eye     = (unsigned int)(h->ratings.eye     * eye_mult);
    h->ratings.speed   = (unsigned int)(h->ratings.speed   * speed_mult);
}

void pitcher_age_curve(Pitcher *p) {
    unsigned int age = p->base.age;

	float command_mult	= age_multiplier(age, 30.0f, 0.0015f); 
	float stuff_mult	= age_multiplier(age, 29.0f, 0.0030f);
	float stamina_mult	= age_multiplier(age, 29.0f, 0.0020f); 

	p->ratings.command	= (unsigned int)(p->ratings.command * command_mult);
	p->ratings.stuff	= (unsigned int)(p->ratings.stuff * stuff_mult);
	p->ratings.stamina	= (unsigned int)(p->ratings.stamina * stamina_mult);
}

unsigned int total_hitter_rating(Hitter *h) {
	return (h->ratings.contact + h->ratings.eye + h->ratings.power + h->ratings.speed);
}

unsigned int total_pitcher_rating(Pitcher *p) {
	return (p->ratings.command + p->ratings.stamina + p->ratings.stuff);
}
