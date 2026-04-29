#include "utils.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <features.h>
#include <time.h>

#ifdef __GLIBC__ 
// interval between min and max is inclusive
int random_int_range(int min, int max) {
    return min + (int)arc4random_uniform((uint32_t)(max - min + 1));
}
#endif // __GLIBC__

// thank you, stack overflow
int random_int_limit(int limit) {
	srand(time(NULL));
	int divisor = RAND_MAX/(limit+1);
	int retval;

	do {
		retval = rand() / divisor;
	} while (retval > limit);

	return retval + 1;  // there is never a use for this where I want 0 as an output
}

double random_double(double min, double max) {
	srand(time(NULL));
	return min + (rand() / (double)RAND_MAX) * (max-min);
}

unsigned int line_count(FILE* f) {
	char buf[LARGE_BUF_SIZE];
	unsigned int count = 0;
	for (;;) {
		size_t res = fread(buf, 1, LARGE_BUF_SIZE, f);
		if (ferror(f))
			return -1;
		for (int i = 0; i < res; i++)
			if (buf[i] == '\n')
				count += 1;
		if (feof(f))
			break;
	}
	return count;
}

int goto_line(FILE* f, unsigned int line_number) {
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	for (int i = 0; i < line_number; i++) {
		nread = getline(&line, &len, f);
		if (nread == -1) {
			free(line);
			line = NULL;
			return -1;
		}
	}
	free(line);
	line = NULL;
	return 0;
}

char *file_random_line(const char *path) {
	FILE *f = fopen(path, "r");
	char *r_str;
	if (!f) {
		perror("Unable to open file");
		exit(EXIT_FAILURE);
	}

	unsigned int count = line_count(f);
	rewind(f);
	int line = random_int_range(1, count - 1);  // need count - 1 because fgets fails when line == count 
	int moved = goto_line(f, line);

	if (moved == -1) {
		perror("Failed to move file pointer to random spot");
		exit(EXIT_FAILURE);
	}

	char buf[DISCARD_BUF_SIZE];
	if (!fgets(buf, sizeof(buf), f)) {
		perror("Unexpected EOF while reading file.\n");
		exit(EXIT_FAILURE);
	}

	buf[strcspn(buf, "\n")] = 0;
	r_str = strdup(buf);
	fclose(f);
	f = NULL;
	return r_str;
}
