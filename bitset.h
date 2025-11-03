#ifndef BITSET
#define BITSET

#include <stdbool.h>
#include <stddef.h>

/*
  - Incapsulation in C???
  - HELL YEAH!!!!
*/

struct __bitset__ {
  size_t length;
  size_t* data;
};
typedef struct __bitset__ bitset;

bitset bitset_create();

bool bitset_contains(const bitset* bs, size_t n);

void bitset_add(bitset* bs, size_t n);
void bitset_remove(bitset* bs, size_t n);

void bitset_union(bitset* dest, const bitset* src);
void bitset_difference(bitset* dest, const bitset* src);
void bitset_intersection(bitset* dest, const bitset* src);

void bitset_free(bitset* bs);

#endif
