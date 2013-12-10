/*
 * SuperFastHash implementation is from
 *
 * http://www.azillionmonkeys.com/qed/hash.html
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#ifdef __linux__
#include <stdint.h>
#endif
#include "hashtbl.h"

hashtbl
*hash_create(int N, hashfunc *hasher, hashkeycmp *cmp, hashfree *datafree)
{
	hashtbl *new = calloc(1, sizeof(*new));
	assert(new);
	new->modulus = N;
	new->hasher = hasher;
	new->keycmp = cmp;
	new->datafree = datafree;
	new->items = calloc(N, sizeof(hashitem*));
	return new;
}

int
hash_add(const void *key, void *data, hashtbl *tbl)
{
	hashitem *new = calloc(1, sizeof(*new));
	hashitem **I;
	int slot;
	new->key = key;
	new->data = data;
	slot = tbl->hasher(key) % tbl->modulus;
	for (I = &tbl->items[slot]; *I; I = &(*I)->next);
	*I = new;
	tbl->cnt++;
	return 0;
}

void *
hash_find(const void *key, hashtbl *tbl)
{
	int slot = tbl->hasher(key) % tbl->modulus;
	hashitem *i;
	for (i = tbl->items[slot]; i; i = i->next) {
		if (0 == tbl->keycmp(key, i->key))
		    return i->data;
	}
	return NULL;
}

unsigned int
hash_count(hashtbl *tbl)
{
	return tbl->cnt;
}

void
hash_analyze(hashtbl *tbl)
{
        unsigned int slot;
	unsigned int num_empty_slots = 0;
	unsigned int max = 0;
        for(slot = 0; slot < tbl->modulus; slot++) {
                int count = 0;
                hashitem *i;
                for (i = tbl->items[slot]; i; i=i->next)
                        count++;
		if (0 == count) {
			num_empty_slots++;
		} else if (count > max) {
			max = count;
		}
        }
	fprintf(stderr, "  Total Slots: %u\n", tbl->modulus);
	fprintf(stderr, "  Empty Slots: %u\n", num_empty_slots);
	fprintf(stderr, "    Hash Keys: %u\n", tbl->cnt);
	fprintf(stderr, "Expected Mean: %u\n", tbl->cnt / tbl->modulus);
	fprintf(stderr, " Longest List: %u\n", max);
}

void
hash_remove(const void *key, hashtbl * tbl)
{
	hashitem **I, *i;
	int slot;
	slot = tbl->hasher(key) % tbl->modulus;
	for (I = &tbl->items[slot]; *I; I = &(*I)->next) {
		if (0 == tbl->keycmp(key, (*I)->key)) {
			i = *I;
			*I = (*I)->next;
			tbl->datafree(i->data);
			free(i);
			break;
		}
	}
}

void
hash_free(hashtbl *tbl)
{
	int slot;
	for(slot = 0; slot < tbl->modulus; slot++) {
		hashitem *i;
		hashitem *next;
		for (i = tbl->items[slot]; i; i=next) {
			next = i->next;
			tbl->datafree(i->data);
			free(i);
		}
		tbl->items[slot] = NULL;
	}
}

void
hash_destroy(hashtbl *tbl)
{
	hash_free(tbl);
	free(tbl->items);
	free(tbl);
}

static void
hash_iter_next_slot(hashtbl *tbl)
{
	while (tbl->iter.next == NULL) {
		tbl->iter.slot++;
		if (tbl->iter.slot == tbl->modulus)
			break;
		tbl->iter.next = tbl->items[tbl->iter.slot];
	}
}

void
hash_iter_init(hashtbl *tbl)
{
	tbl->iter.slot = 0;
	tbl->iter.next = tbl->items[tbl->iter.slot];
	if (NULL == tbl->iter.next)
		hash_iter_next_slot(tbl);
}

void *
hash_iterate(hashtbl *tbl)
{
	hashitem *this = tbl->iter.next;
	if (this) {
		tbl->iter.next = this->next;
		if (NULL == tbl->iter.next)
			hash_iter_next_slot(tbl);
	}
	return this ? this->data : NULL;
}

/*
 * http://www.azillionmonkeys.com/qed/hash.html
 */

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
		       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

unsigned int
SuperFastHash (const char * data, int len)
{
    unsigned int hash = len, tmp;
    int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
	hash  += get16bits (data);
	tmp    = (get16bits (data+2) << 11) ^ hash;
	hash   = (hash << 16) ^ tmp;
	data  += 2*sizeof (uint16_t);
	hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
	case 3: hash += get16bits (data);
		hash ^= hash << 16;
		hash ^= data[sizeof (uint16_t)] << 18;
		hash += hash >> 11;
		break;
	case 2: hash += get16bits (data);
		hash ^= hash << 11;
		hash += hash >> 17;
		break;
	case 1: hash += *data;
		hash ^= hash << 10;
		hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}
