#include <stdlib.h>

#include "chunk.h"
#include "line_encode.h"
#include "memory.h"

void init_chunk(Chunk *chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  init_lines(&chunk->encode);
  init_value_array(&chunk->constants);
}

void write_chunk(Chunk *chunk, uint8_t byte, int line) {
  if (chunk->capacity < chunk->count + 1) {
    int prev_capacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(prev_capacity);
    chunk->code =
        GROW_ARRAY(uint8_t, chunk->code, prev_capacity, chunk->capacity);
  }
  chunk->code[chunk->count] = byte;
  write_lines(&chunk->encode, line);
  chunk->count++;
}

void free_chunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  free_lines(&chunk->encode);
  free_value_array(&chunk->constants);
  init_chunk(chunk);
}

int add_constant(Chunk *chunk, Value value) {
  write_value_array(&chunk->constants, value);
  return chunk->constants.count - 1;
}
