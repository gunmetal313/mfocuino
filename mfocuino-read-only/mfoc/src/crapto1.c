/*  crapto1.c

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301, US$

    Copyright (C) 2008-2008 bla <blapost@gmail.com>
*/
#include "crapto1.h"
#include <stdlib.h>

static void quicksort(uint32_t* const start, uint32_t* const stop)
{
	uint32_t *it = start + 1, *rit = stop;

	if(it > rit)
		return;

	while(it < rit)
		if(*it <= *start)
			++it;
		else if(*rit > *start)
			--rit;
		else
			*it ^= (*it ^= *rit, *rit ^= *it);

	if(*rit >= *start)
		--rit;
	if(rit != start)
		*rit ^= (*rit ^= *start, *start ^= *rit);

	quicksort(start, rit - 1);
	quicksort(rit + 1, stop);
}
/** binsearch
 * Binary search for the first occurence of *stop's MSB in sorted [start,stop]
 */
static inline uint32_t*
binsearch(uint32_t *start, uint32_t *stop)
{
	uint32_t mid, val = *stop & 0xff000000;
	while(start != stop)
		if(start[mid = (stop - start) >> 1] > val)
			stop = &start[mid];
		else
			start += mid + 1;

	return start;
}

/** update_contribution
 * helper, calculates the partial linear feedback contributions and puts in MSB
 */
static inline void
update_contribution(uint32_t *item, const uint32_t mask1, const uint32_t mask2)
{
	uint32_t p = *item >> 25;

	p = p << 1 | parity(*item & mask1);
	p = p << 1 | parity(*item & mask2);
	*item = p << 24 | (*item & 0xffffff);
}

/** extend_table
 * using a bit of the keystream extend the table of possible lfsr states
 */
static inline void
extend_table(uint32_t *tbl, uint32_t **end, int bit, int m1, int m2, uint32_t in)
{
	in <<= 24;
	for(*tbl <<= 1; tbl <= *end; *++tbl <<= 1)
		if(filter(*tbl) ^ filter(*tbl | 1)) {
			*tbl |= filter(*tbl) ^ bit;
			update_contribution(tbl, m1, m2);
			*tbl ^= in;
		} else if(filter(*tbl) == bit) {
			*++*end = tbl[1];
			tbl[1] = tbl[0] | 1;
			update_contribution(tbl, m1, m2);
			*tbl++ ^= in;
			update_contribution(tbl, m1, m2);
			*tbl ^= in;
		} else
			*tbl-- = *(*end)--;
}
/** extend_table_simple
 * using a bit of the keystream extend the table of possible lfsr states
 */
static inline void
extend_table_simple(uint32_t *tbl, uint32_t **end, int bit)
{
	for(*tbl <<= 1; tbl <= *end; *++tbl <<= 1)
		if(filter(*tbl) ^ filter(*tbl | 1)) {
			*tbl |= filter(*tbl) ^ bit;
		} else if(filter(*tbl) == bit) {
			*++*end = *++tbl;
			*tbl = tbl[-1] | 1;
		} else
			*tbl-- = *(*end)--;
}
/** recover
 * recursively narrow down the search space, 4 bits of keystream at a time
 */
static struct Crypto1State*
recover(uint32_t *o_head, uint32_t *o_tail, uint32_t oks,
	uint32_t *e_head, uint32_t *e_tail, uint32_t eks, int rem,
	struct Crypto1State *sl, uint32_t in)
{
	uint32_t *o, *e, i;

	if(rem == -1) {
		for(e = e_head; e <= e_tail; ++e) {
			*e = *e << 1 ^ parity(*e & LF_POLY_EVEN) ^ !!(in & 4);
			for(o = o_head; o <= o_tail; ++o, ++sl) {
				sl->even = *o;
				sl->odd = *e ^ parity(*o & LF_POLY_ODD);
				sl[1].odd = sl[1].even = 0;
			}
		}
		return sl;
	}

	for(i = 0; i < 4 && rem--; i++) {
		extend_table(o_head, &o_tail, (oks >>= 1) & 1,
			LF_POLY_EVEN << 1 | 1, LF_POLY_ODD << 1, 0);
		if(o_head > o_tail)
			return sl;

		extend_table(e_head, &e_tail, (eks >>= 1) & 1,
			LF_POLY_ODD, LF_POLY_EVEN << 1 | 1, (in >>= 2) & 3);
		if(e_head > e_tail)
			return sl;
	}

	quicksort(o_head, o_tail);
	quicksort(e_head, e_tail);

	while(o_tail >= o_head && e_tail >= e_head)
		if(((*o_tail ^ *e_tail) >> 24) == 0) {
			o_tail = binsearch(o_head, o = o_tail);
			e_tail = binsearch(e_head, e = e_tail);
			sl = recover(o_tail--, o, oks,
				     e_tail--, e, eks, rem, sl, in);
		}
		else if(*o_tail > *e_tail)
			o_tail = binsearch(o_head, o_tail) - 1;
		else
			e_tail = binsearch(e_head, e_tail) - 1;

	return sl;
}
/** lfsr_recovery
 * recover the state of the lfsr given 32 bits of the keystream
 * additionally you can use the in parameter to specify the value
 * that was fed into the lfsr at the time the keystream was generated
 */
struct Crypto1State* lfsr_recovery32(uint32_t ks2, uint32_t in)
{
	struct Crypto1State *statelist;
	uint32_t *odd_head = 0, *odd_tail = 0, oks = 0;
	uint32_t *even_head = 0, *even_tail = 0, eks = 0;
	int i;

	for(i = 31; i >= 0; i -= 2)
		oks = oks << 1 | BEBIT(ks2, i);
	for(i = 30; i >= 0; i -= 2)
 		eks = eks << 1 | BEBIT(ks2, i);

	odd_head = odd_tail = malloc(sizeof(uint32_t) << 21);
	even_head = even_tail = malloc(sizeof(uint32_t) << 21);
	statelist =  malloc(sizeof(struct Crypto1State) << 18);
	if(!odd_tail-- || !even_tail-- || !statelist)
		goto out;

	statelist->odd = statelist->even = 0;

	for(i = 1 << 20; i >= 0; --i) {
		if(filter(i) == (oks & 1))
			*++odd_tail = i;
		if(filter(i) == (eks & 1))
			*++even_tail = i;
	}

	for(i = 0; i < 4; i++) {
		extend_table_simple(odd_head,  &odd_tail, (oks >>= 1) & 1);
		extend_table_simple(even_head, &even_tail, (eks >>= 1) & 1);
	}

	in = (in >> 16 & 0xff) | (in << 16) | (in & 0xff00);
	recover(odd_head, odd_tail, oks,
		even_head, even_tail, eks, 11, statelist, in << 1);

out:
	free(odd_head);
	free(even_head);
	return statelist;
}

uint8_t lfsr_rollback_bit(struct Crypto1State *s, uint32_t in, int fb);
uint32_t lfsr_rollback_word(struct Crypto1State *s, uint32_t in, int fb);

/** lfsr_rollback_bit
 * Rollback the shift register in order to get previous states
 */
uint8_t lfsr_rollback_bit(struct Crypto1State *s, uint32_t in, int fb)
{
	int out;
	uint8_t ret;

	s->odd &= 0xffffff;
	s->odd ^= (s->odd ^= s->even, s->even ^= s->odd);

	out = s->even & 1;
	out ^= LF_POLY_EVEN & (s->even >>= 1);
	out ^= LF_POLY_ODD & s->odd;
	out ^= !!in;
	out ^= (ret = filter(s->odd)) & !!fb;

	s->even |= parity(out) << 23;
	return ret;
}
/** lfsr_rollback_word
 * Rollback the shift register in order to get previous states
 */
uint32_t lfsr_rollback_word(struct Crypto1State *s, uint32_t in, int fb)
{
	int i;
	uint32_t ret = 0;
	for (i = 31; i >= 0; --i)
		ret |= lfsr_rollback_bit(s, BEBIT(in, i), fb) << (i ^ 24);
	return ret;
}

/** nonce_distance
 * x,y valid tag nonces, then prng_successor(x, nonce_distance(x, y)) = y
 */
static uint16_t *dist = 0;
int nonce_distance(uint32_t from, uint32_t to)
{
	uint16_t x, i;
	if(!dist) {
		dist = malloc(2 << 16);
		if(!dist)
			return -1;
		for (x = i = 1; i; ++i) {
			dist[(x & 0xff) << 8 | x >> 8] = i;
			x = x >> 1 | (x ^ x >> 2 ^ x >> 3 ^ x >> 5) << 15;
		}
	}
	return (65535 + dist[to >> 16] - dist[from >> 16]) % 65535;
}
