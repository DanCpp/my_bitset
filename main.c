#ifndef USE_STD_ALLOC
  #include "alloc.h"
#endif

#include "bitset.h"

#include <stdio.h>
#include <stdlib.h>

#define max(a, b) (a < b ? b : a)

int main() {
#ifndef USE_STD_ALLOC
  create();
  atexit(delete);
#endif

  size_t sizes[4];
  for (size_t index = 0; index < 4; index++) {
    scanf("%zu", &sizes[index]);
  }

  size_t max_element = 0;
  bitset bss[4];

  for (size_t index = 0; index < 4; index++) {
    bss[index] = bitset_create();

    size_t element;
    for (size_t j = 0; j < sizes[index]; j++) {
      scanf("%zu", &element);
      bitset_add(&bss[index], element);

      max_element = max(max_element, element);
    }
  }

  bitset_union(&bss[0], &bss[1]);
  bitset_intersection(&bss[0], &bss[2]);
  bitset_difference(&bss[0], &bss[3]);

  bool first_print = true;
  for (size_t candidate = 0; candidate <= max_element; candidate++) {
    if (!bitset_contains(&bss[0], candidate))
      continue;

    if (!first_print)
      putchar(' ');
    printf("%zu", candidate);
    first_print = false;
  }

  putchar('\n');

  for (size_t index = 0; index < 4; index++) {
    bitset_free(&bss[index]);
  }
}