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
