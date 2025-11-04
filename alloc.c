#include "alloc.h"

#undef malloc
#undef realloc
#undef calloc
#undef free

#include <stdint.h>

#define PTR_SIZE sizeof(char*)
#define MEGABYTE (1 << 20)
#define MIN_SIZE 8
#define MAX_SIZE 128
#define SPECIAL_ARENAS 16

#define min(a, b) (a < b) ? a : b

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void* nc_malloc(size_t nmemb) {
  void* ptr = malloc(nmemb);
  if (!ptr) {
    abort();
  }

  return ptr;
}

typedef struct __block__ {
  struct __block__* next;
  char* memory;
  size_t block_size;
} block_t;

typedef struct {
  char* memory;
  size_t size;
  block_t* unused;
} large_arena_t;

typedef struct {
  char* memory;
  char** head;
  size_t bs;
  size_t size;
} special_arena_t;

typedef struct {
  char* memory;
  special_arena_t s_arenas[SPECIAL_ARENAS];
  large_arena_t large;
} allocator_t;

static allocator_t allocator;

static void initialize_specials() {
  size_t bs_next = MIN_SIZE;
  for (size_t index = 0; index < SPECIAL_ARENAS; index++, bs_next += 8) {
    special_arena_t* arena = &allocator.s_arenas[index];
    arena->bs = bs_next;
    arena->size = 10 * MEGABYTE;
    if (index)
      arena->memory = allocator.s_arenas[index - 1].memory + 10 * MEGABYTE;
    else
      arena->memory = allocator.memory;

    arena->head = (char**) arena->memory;
    char** next = arena->head;
    char** prev = NULL;
    while ((char*) next < arena->memory + arena->size) {
      if (prev)
        *next = *prev + arena->bs;
      else
        *next = (char*) arena->head + arena->bs;

      prev = next;
      next = (char**) (*next);
    }
    *prev = NULL;
    ///
  }
}

static void initialize_large() {
  large_arena_t* large = &allocator.large;
  large->memory = allocator.memory + SPECIAL_ARENAS * 10 * MEGABYTE;
  large->size = 300 * MEGABYTE;
  large->unused = nc_malloc(sizeof(block_t));
  *large->unused = (block_t){
      .block_size = large->size,
      .memory = large->memory,
      .next = NULL,
  };
}

void create() {
  allocator.memory = nc_malloc(460 * MEGABYTE);
  initialize_specials();
  initialize_large();
}

void delete() {
  free(allocator.memory);
  block_t* to_delete = allocator.large.unused;
  block_t* next = to_delete->next;
  while (to_delete) {
    next = to_delete->next;
    free(to_delete);
    to_delete = next;
  }
}

static void* allocate_in_specials(size_t nmemb) {
  size_t round_to_eight = (nmemb / 8 + (nmemb % 8 ? 1 : 0)) * 8;
  if (nmemb == 0)
    round_to_eight = 8;

  special_arena_t* allocation_arena = &allocator.s_arenas[(round_to_eight - 8) / 8];
  if (*allocation_arena->head == NULL)
    return NULL;

  char** new_head = (char**) (*allocation_arena->head);
  void* ptr = ((char*) allocation_arena->head);

  allocation_arena->head = new_head;
  return ptr;
}

static void* allocate_in_large(size_t nmemb) {
  nmemb = (nmemb / sizeof(size_t) + (nmemb % sizeof(size_t) ? 1 : 0)) * sizeof(size_t);
  block_t* avaliable = allocator.large.unused;
  while (avaliable && nmemb + PTR_SIZE > avaliable->block_size) {
    avaliable = avaliable->next;
  }

  if (!avaliable)
    return NULL;

  char* ptr = avaliable->memory + PTR_SIZE;
  *(size_t*) (avaliable->memory) = nmemb;
  avaliable->memory = ptr + nmemb;
  avaliable->block_size -= (nmemb + PTR_SIZE);

  return ptr;
}

static size_t get_size(void* ptr) {
  if ((char*) ptr > allocator.large.memory)
    return *(size_t*) ((char*) ptr - PTR_SIZE);

  size_t arena_index = (size_t) ((char*) ptr - allocator.memory) / (10 * MEGABYTE);
  return allocator.s_arenas[arena_index].bs;
}

void* allocate(size_t nmemb) {
  if (nmemb > 128)
    return allocate_in_large(nmemb);

  void* ptr = allocate_in_specials(nmemb);
  if (!ptr)
    return allocate_in_large(nmemb);

  return ptr;
}

void* allocate_filled(size_t n, size_t memb) {
  void* ptr = allocate(n * memb);
  if (!ptr)
    return NULL;

  memset(ptr, 0, n * memb);
  return ptr;
}

void* reallocate(void* old, size_t nmemb) {
  if (!old)
    return allocate(nmemb);

  char* c_new = allocate(nmemb);
  if (!c_new)
    return NULL;

  char* c_old = (char*) old;
  size_t new_copied_block = min(get_size(old), nmemb);

  memmove(c_new, c_old, new_copied_block);

  deallocate(old);
  return c_new;
}

static void deallocate_specials(void* ptr, size_t ptr_size) {
  special_arena_t* arena = &allocator.s_arenas[(ptr_size - 8) / 8];

  char** c_ptr = (char**) ((char*) ptr);
  *c_ptr = (char*) arena->head;
  arena->head = c_ptr;
}

static void deallocate_large(void* ptr, size_t ptr_size) {
  block_t* next = allocator.large.unused;
  block_t* prev = NULL;
  while (next && next->memory > (char*) ptr) {
    if (!next->next)
      break;
    if (next->next->memory < (char*) ptr) {
      break;
    }

    prev = next;
    next = next->next;
  }

  if (prev && (prev->memory + prev->block_size == (char*) ptr - PTR_SIZE)) {
    prev->block_size += (PTR_SIZE + ptr_size);
    if (prev->memory + prev->block_size == next->memory) {
      prev->block_size += next->block_size;
      prev->next = next->next;
      free(next);
    }

    return;
  }

  if ((char*) ptr + ptr_size == next->memory) {
    next->memory = (char*) ptr - PTR_SIZE;
    next->block_size += (PTR_SIZE + ptr_size);

    return;
  }

  block_t* new = nc_malloc(sizeof(block_t));
  *new = (block_t){
      .memory = (char*) ptr - PTR_SIZE,
      .block_size = PTR_SIZE + ptr_size,
      .next = next,
  };

  if (prev)
    prev->next = new;
  else
    allocator.large.unused = new;
}

void deallocate(void* ptr) {
  if (!ptr)
    return;

  size_t ptr_size = get_size(ptr);
  // deallocation in large happened
  if (ptr_size > 128) {
    deallocate_large(ptr, ptr_size);
    return;
  }

  deallocate_specials(ptr, ptr_size);
}