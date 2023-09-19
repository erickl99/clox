#include "vm.h"
#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "line_encode.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

VM vm;

static void reset_stack() { vm.stack_top = vm.stack; }

static void runtime_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = vm.ip - vm.chunk->code - 1;
  int line = get_line(&vm.chunk->encode, instruction);
  fprintf(stderr, "[line %d] in script\n", line);
  reset_stack();
}

static Value peek(int distance) { return vm.stack_top[-1 - distance]; }

static bool is_falsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
  ObjString *b = AS_STRING(pop());
  ObjString *a = AS_STRING(pop());
  int length = a->length + b->length;
  char *chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString *result = take_string(chars, length);
  FREE_ARRAY(char, chars, length);
  push(OBJ_VAL(result));
}

static Result run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(type, op)                                                    \
  do {                                                                         \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                          \
      runtime_error("Operands must be numbers.");                              \
      return RUNTIME_ERROR;                                                    \
    }                                                                          \
    double b = AS_NUMBER(pop());                                               \
    double a = AS_NUMBER(pop());                                               \
    push(type(a op b));                                                        \
  } while (false)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("        ");
    for (Value *slot = vm.stack; slot < vm.stack_top; slot++) {
      printf("[ ");
      print_value(*slot);
      printf(" ]");
    }
    printf("\n");
    disassemble_instr(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
    case OP_ADD: {
      if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
        concatenate();
        break;
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        double a = AS_NUMBER(pop());
        double b = AS_NUMBER(pop());
        push(NUMBER_VAL(a + b));
      } else {
        runtime_error("Operands must be two numbers or two strings");
        return RUNTIME_ERROR;
      }
      break;
    }
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      print_value(constant);
      printf("\n");
      break;
    }
    case OP_DEFINE_GLOBAL: {
      ObjString *name = READ_STRING();
      table_set(&vm.globals, name, peek(0));
      pop();
      break;
    }
    case OP_DIVIDE: {
      BINARY_OP(NUMBER_VAL, /);
      break;
    }
    case OP_EQUAL: {
      Value a = pop();
      Value b = pop();
      push(BOOL_VAL(values_equal(a, b)));
      break;
    }
    case OP_FALSE: {
      push(BOOL_VAL(false));
      break;
    }
    case OP_GET_GLOBAL: {
      ObjString *name = READ_STRING();
      Value value;
      if (!table_get(&vm.globals, name, &value)) {
        runtime_error("Undefined variable '%s'.", name->chars);
        return RUNTIME_ERROR;
      }
      push(value);
      break;
    }
    case OP_GREATER: {
      BINARY_OP(BOOL_VAL, >);
      break;
    }
    case OP_LESS: {
      BINARY_OP(BOOL_VAL, <);
      break;
    }
    case OP_MULTIPLY: {
      BINARY_OP(NUMBER_VAL, *);
      break;
    }
    case OP_NEGATE: {
      // push(-pop());
      Value val = peek(0);
      if (!IS_NUMBER(val)) {
        runtime_error("Operand must be a number.");
        return RUNTIME_ERROR;
      }
      vm.stack_top[-1] = NUMBER_VAL(-AS_NUMBER(val));
      break;
    }
    case OP_NIL: {
      push(NIL_VAL);
      break;
    }
    case OP_NOT: {
      Value val = peek(0);
      vm.stack_top[-1] = BOOL_VAL(is_falsey(val));
      break;
    }
    case OP_POP: {
      pop();
      break;
    }
    case OP_PRINT: {
      print_value(pop());
      printf("\n");
      break;
    }
    case OP_RETURN: {
      // print_value(pop());
      // printf("\n");
      return OK;
    }
    case OP_SET_GLOBAL: {
      ObjString *name = READ_STRING();
      if (table_set(&vm.globals, name, peek(0))) {
        table_delete(&vm.globals, name);
        runtime_error("Undefined variable '%s'.", name->chars);
        return RUNTIME_ERROR;
      }
      break;
    }
    case OP_SUBTRACT: {
      BINARY_OP(NUMBER_VAL, -);
      break;
    }
    case OP_TRUE: {
      push(BOOL_VAL(true));
      break;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

void init_vm() {
  reset_stack();
  vm.objects = NULL;
  init_table(&vm.globals);
  init_table(&vm.strings);
}

void free_vm() {
  free_objects();
  init_table(&vm.globals);
  free_table(&vm.strings);
}

void push(Value value) {
  *vm.stack_top = value;
  vm.stack_top++;
}

Value pop() {
  vm.stack_top--;
  return *vm.stack_top;
}

Result interpret(char *source) {
  Chunk chunk;
  init_chunk(&chunk);

  if (!compile(source, &chunk)) {
    printf("Encountered error, freeing chunk\n");
    free_chunk(&chunk);
    return COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  Result result = run();

  free_chunk(&chunk);
  return result;
}
