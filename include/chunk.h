#ifndef clox_hunk_h
#define clox_hunk_h

#include "common.h"
#include "line_encode.h"
#include "value.h"

typedef enum {
  OP_ADD,
  OP_CALL,
  OP_CONSTANT,
  OP_DEFINE_GLOBAL,
  OP_DIVIDE,
  OP_EQUAL,
  OP_FALSE,
  OP_GET_GLOBAL,
  OP_GET_LOCAL,
  OP_GREATER,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_JUMP_IF_TRUE,
  OP_JUMP_SWITCH,
  OP_LESS,
  OP_LOOP,
  OP_MULTIPLY,
  OP_NIL,
  OP_NOT,
  OP_NEGATE,
  OP_POP,
  OP_PRINT,
  OP_SET_GLOBAL,
  OP_SET_LOCAL,
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
