/*
 *
 * Let's say you need to generate the top N list of a *lot* of strings.
 * More than you can keep in memory and more than your SQL database can
 * crunch on.  You can use this program.
 *
 * Input is string, whitespace, and an optional count.  If the count is
 * not given, the string is coutned once.
 *
 * Output is a sorted list of string,count pairs.
 *
 * The program reads and counts strings until it has inmem_count_hi
 * strings in memory.  At that point, the program sorts the strings by
 * count and removes the strings with low counts until there are only
 * inmem_count_lo strings left.  Then the program proceeds to read more
 * strings from the input.
 *
 * This program assumes the strings have a skewed popularity distribution
 * such that the popular strings will always be kept in memory.
 * If strings have a uniform popularity distribution, then this program
 * won't work very well.
 *
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>

#include "hashtbl.h"

typedef struct _string_counter {
	char *string;
	uint64_t count;
} string_counter;

hashtbl *tbl = 0;
string_counter **sorted;

unsigned int output_count = 100;
unsigned int inmem_count_lo = 500000;
unsigned int inmem_count_hi = 1000000;

unsigned int
str_hash(const void *s)
{
	return SuperFastHash(s, strlen(s));
}

void
myfree(void *d)
{
	string_counter *sc = d;
	free(sc->string);
	free(sc);
}

int
sort_cmper(const void *a, const void *b)
{
	string_counter *A = *(string_counter**)a;
	string_counter *B = *(string_counter**)b;
	return ((B->count > A->count) - (B->count < A->count));
}

unsigned int
sort(void)
{
	unsigned int i;
	unsigned int n = hash_count(tbl);
	fprintf(stderr, "SORT %u...", n);
	hash_iter_init(tbl);
	for (i = 0; i < n; i++) {
		*(sorted+i) = (string_counter *) hash_iterate(tbl);
	}
	qsort(sorted, n, sizeof(string_counter *), sort_cmper);
	fprintf(stderr, "\n");
	return n;
}

void
cull()
{
	unsigned int i;
	unsigned int n = sort();
	fprintf(stderr, "CULL down to %u...", inmem_count_lo);
	for (i = inmem_count_lo; i < n; i++) {
		hash_remove((*(sorted+i))->string, tbl);
	}
	fprintf(stderr, "\n");
}

void
output(void)
{
	unsigned int i;
	unsigned int n = sort();
	for (i = 0 ; i < n && i < output_count; i++) {
		string_counter *sc = *(sorted+i);
		printf("%s %"PRIu64"\n", sc->string, sc->count);
	}
}


int
main(int argc, char *argv[])
{
	char buf[512];
	tbl = hash_create(inmem_count_lo+1, str_hash, (hashkeycmp *) strcmp, myfree);
	assert(tbl);
	sorted = calloc(inmem_count_hi, sizeof(string_counter *));
	while (fgets(buf, sizeof(buf), stdin)) {
		string_counter *sc;
		char *s, *t;
		uint64_t n;
		s = strtok(buf, " \t\r\n");
		assert(s);
		t = strtok(0, "\r\n");
		if (0 == t)
			n = 1;
		else
			n = strtoull(t, 0, 10);
		sc = hash_find(buf, tbl);
		if (0 == sc) {
			sc = calloc(1, sizeof(*sc));
			assert(sc);
			sc->string = strdup(buf);
			hash_add(sc->string, sc, tbl);
		}
		sc->count += n;
		if (inmem_count_hi == hash_count(tbl))
			cull(tbl);
	}
	output();
	return 0;
}
