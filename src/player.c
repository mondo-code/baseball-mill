#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "utils.h"

Hitter *gen_hitter(const char *first_name, const char *last_name, unsigned int age) {
	Hitter *h = malloc(sizeof(Hitter));
	h->base = malloc(sizeof(Player));
	h->base->first_name = strdup(first_name);
	h->base->last_name = strdup(last_name);
	h->base->age = age;

	HitterRatings hr = {0};
	hr.contact = random_int_limit(99);
	hr.power = random_int_limit(99);
	hr.eye = random_int_limit(99);
	hr.speed = random_int_limit(99);
	h->ratings = hr;

	HitterStats hs = {0};
	h->stats = hs;

	return h;
}

Pitcher *gen_pitcher(const char *first_name, const char *last_name, unsigned int age) {
	Pitcher *p = malloc(sizeof(Pitcher));
	p->base = malloc(sizeof(Player));
	p->base->first_name = strdup(first_name);
	p->base->last_name = strdup(last_name);
	p->base->age = age;

	PitcherRatings pr = {0};
	pr.command = random_int_limit(99);
	pr.stuff = random_int_limit(99);
	pr.stamina = random_int_limit(99);
	p->ratings = pr;

	PitcherStats ps = {0};
	p->stats = ps;

	return p;
}

void add_innings(Innings *a, Innings *b) {
	a->whole += b->whole;
	unsigned int new_third = a->thirds + b->thirds;
	a->whole += new_third / 3;
	a->thirds = new_third % 3;
}
