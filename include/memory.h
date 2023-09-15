#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"
#include "object.h"

#define ALLOCATE(type, count)                                                  \
  (type *)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity)*2)

#define GROW_ARRAY(type, pointer, prev_count, new_count)                       \
  (type *)reallocate(pointer, sizeof(type) * (prev_count),                     \
                     sizeof(type) * (new_count))

#define FREE_ARRAY(type, pointer, prev_count)                                  \
  reallocate(pointer, sizeof(type) * (prev_count), 0)

void *reallocate(void *pointer, size_t prev_size, size_t new_size);

void free_objects();

#endif
