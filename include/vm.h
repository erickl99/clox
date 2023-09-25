#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "common.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
  ObjFunction *function;
  uint8_t *ip;
  Value *slots;
} CallFrame;

typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frame_count;
  Value stack[STACK_MAX];
  Value *stack_top;
  Table globals;
  Table strings;
  Obj *objects;
} VM;

typedef enum { OK, COMPILE_ERROR, RUNTIME_ERROR } Result;

extern VM vm;

void init_vm();
void free_vm();
void push(Value value);
Value pop();
Result interpret(char *source);

#endif
