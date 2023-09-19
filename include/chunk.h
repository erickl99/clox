#ifndef clox_hunk_h
#define clox_hunk_h

#include "common.h"
#include "line_encode.h"
#include "value.h"

typedef enum {
  OP_ADD,
  OP_CONSTANT,
  OP_DEFINE_GLOBAL,
  OP_DIVIDE,
  OP_EQUAL,
  OP_FALSE,
  OP_GET_GLOBAL,
  OP_GREATER,
  OP_LESS,
  OP_MULTIPLY,
  OP_NIL,
  OP_NOT,
  OP_NEGATE,
  OP_POP,
  OP_PRINT,
  OP_SET_GLOBAL,
  OP_SUBTRACT,
  OP_TRUE,
  OP_RETURN,
} OpCode;

typedef struct {
  int count;
  int capacity;
  uint8_t *code;
  LineEncode encode;
  ValueArray constants;
} Chunk;

void init_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte, int line);
void free_chunk(Chunk *chunk);
int add_constant(Chunk *chunk, Value value);

#endif
