#ifndef UTIL_HPP
#define UTIL_HPP

#include <stdio.h>
#define LARGE_BUF_SIZE 65536
#define DISCARD_BUF_SIZE 256

int random_int_range(int min, int max);
int random_int_limit(int limit);
double random_double(double min, double max);
unsigned int line_count(FILE* f);
int goto_line(FILE* f, unsigned int line_number);
int max(int num1, int num2);
char *file_random_line(const char *path);

static inline double clamp(double p) {
	if (p < 0.0) { return 0.0; }
	if (p > 1.0) { return 1.0; }
	return p;
}

#endif
