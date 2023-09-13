#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"

#define STACK_MAX 256

typedef struct {
  Chunk *chunk;
  uint8_t *ip;
  Value stack[STACK_MAX];
  Value *stack_top;
} VM;

typedef enum { OK, COMPILE_ERROR, RUNTIME_ERROR } Result;

void init_vm();
void free_vm();
void push(Value value);
Value pop();
Result interpret(char *source);

#endif
