#include "bitset.h"

#ifndef USE_STD_ALLOC
  #include "alloc.h"
#endif

#include <stdlib.h>
#include <string.h>

#define BYTE 8
#define SSIZE (sizeof(size_t))
#define FIELD_SIZE (BYTE * SSIZE)
#define FIELD_INDEX(n) (n / FIELD_SIZE)
#define FULL_SIZE(bs) (bs->length * FIELD_SIZE)
#define BIT(n) (1ul << (n % FIELD_SIZE))

#define min(a, b) (a < b ? a : b)

static void* nc_realloc(void* old, size_t nmemb) {
  void* ptr = realloc(old, nmemb);
  if (!ptr) {
    abort();
  }

  return ptr;
}

bitset bitset_create() {
  return (bitset){
      .length = 0,
      .data = NULL,
  };
}

inline static size_t* get_field(const bitset* bs, size_t n) {
  if (bs->length <= FIELD_INDEX(n))
    return NULL;

  return &bs->data[FIELD_INDEX(n)];
}

inline static void expand_bitset(bitset* bs, size_t new_length) {
  bs->data = nc_realloc(bs->data, new_length * sizeof(*bs->data));
  memset(bs->data + bs->length, 0, (new_length - bs->length) * sizeof(*bs->data));

  bs->length = new_length;
}

bool bitset_contains(const bitset* bs, size_t n) {
  size_t* field = get_field(bs, n);

  return field && (*field) & BIT(n);
}

void bitset_add(bitset* bs, size_t n) {
  if (FULL_SIZE(bs) <= n) {
    expand_bitset(bs, FIELD_INDEX(n) + 1);
  }

  bs->data[FIELD_INDEX(n)] |= BIT(n);
}

void bitset_remove(bitset* bs, size_t n) {
  if (FULL_SIZE(bs) <= n) {
    return;
  }

  bs->data[FIELD_INDEX(n)] &= ~BIT(n);
}

void bitset_union(bitset* dest, const bitset* src) {
  if (dest->length < src->length) {
    expand_bitset(dest, src->length);
  }

  for (size_t index = 0; index < src->length; index++) {
    dest->data[index] |= src->data[index];
  }
}

void bitset_difference(bitset* dest, const bitset* src) {
  size_t min_length = min(dest->length, src->length);

  for (size_t index = 0; index < min_length; index++) {
    dest->data[index] &= ~src->data[index];
  }
}

void bitset_intersection(bitset* dest, const bitset* src) {
  size_t min_length = min(dest->length, src->length);

  for (size_t index = 0; index < min_length; index++) {
    dest->data[index] &= src->data[index];
  }
}

void bitset_free(bitset* bs) {
  free(bs->data);
}