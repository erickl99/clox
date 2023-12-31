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
#include <time.h>

VM vm;

static Value clock_native(int arg_count, Value *args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void reset_stack() {
  vm.stack_top = vm.stack;
  vm.frame_count = 0;
  vm.open_upvalues = NULL;
}

static void runtime_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = vm.frame_count - 1; i >= 0; i--) {
    CallFrame *frame = &vm.frames[i];
    ObjFunction *function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    ObjUpvalue *open_upvalues;
    fprintf(stderr, "[line %d] in ",
            get_line(&function->chunk.encode, instruction));
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s\n", function->name->chars);
    }
  }
  reset_stack();
}

static void define_native(const char *name, NativeFn function, int arity) {
  push(OBJ_VAL(copy_string(name, (int)strlen(name))));
  push(OBJ_VAL(new_native(function, arity)));
  table_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

static Value peek(int distance) { return vm.stack_top[-1 - distance]; }

static bool call(ObjClosure *closure, int arg_count) {
  if (arg_count != closure->function->arity) {
    runtime_error("Expected %d number of arguments, received %d instead.",
                  closure->function->arity, arg_count);
    return false;
  }

  if (vm.frame_count == FRAMES_MAX) {
    runtime_error("Stack overflow");
    return false;
  }

  CallFrame *frame = &vm.frames[vm.frame_count++];
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  frame->slots = vm.stack_top - arg_count - 1;
  return true;
}

static bool call_value(Value callee, int arg_count) {
  if (IS_OBJ(callee)) {
    switch (AS_OBJ(callee)->type) {
    case OBJ_CLOSURE: {
      return call(AS_CLOSURE(callee), arg_count);
    }
    // case OBJ_FUNCTION:
    //   return call(AS_FUNCTION(callee), arg_count);
    case OBJ_NATIVE: {
      int arity = ((ObjNative *)AS_OBJ(callee))->arity;
      if (arity != arg_count) {
        runtime_error("Expected %d number of arguments, received %d instead.",
                      arity, arg_count);
        return false;
      }
      NativeFn native = AS_NATIVE(callee);
      Value result = native(arg_count, vm.stack_top - arg_count);
      vm.stack_top -= arg_count + 1;
      push(result);
      return true;
    }
    default:
      break;
    }
  }
  runtime_error("Can only call functions and classes.");
  return false;
}

static ObjUpvalue *capture_upvalue(Value *local) {
  ObjUpvalue *prev_upvalue = NULL;
  ObjUpvalue *upvalue = vm.open_upvalues;
  while (upvalue != NULL && upvalue->location > local) {
    prev_upvalue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local) {
    return upvalue;
  }
  ObjUpvalue *created_upvalue = new_upvalue(local);
  created_upvalue->next = upvalue;

  if (prev_upvalue == NULL) {
    vm.open_upvalues = created_upvalue;
  } else {
    prev_upvalue->next = created_upvalue;
  }

  return created_upvalue;
}

static void close_upvalues(Value *last) {
  while (vm.open_upvalues != NULL && vm.open_upvalues->location >= last) {
    ObjUpvalue *upvalue = vm.open_upvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.open_upvalues = upvalue->next;
  }
}

static bool is_falsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
  ObjString *b = AS_STRING(peek(0));
  ObjString *a = AS_STRING(peek(1));
  int length = a->length + b->length;
  char *chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString *result = take_string(chars, length);
  FREE_ARRAY(char, chars, length);
  pop();
  pop();
  push(OBJ_VAL(result));
}

static Result run() {
  CallFrame *frame = &vm.frames[vm.frame_count - 1];
  register uint8_t *ip = frame->ip;
#define READ_BYTE() (*ip++)
#define READ_CONSTANT()                                                        \
  (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(type, op)                                                    \
  do {                                                                         \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                          \
      frame->ip = ip;                                                          \
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
    disassemble_instr(&frame->closure->function->chunk,
                      (int)(ip - frame->closure->function->chunk.code));
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
        frame->ip = ip;
        runtime_error("Operands must be two numbers or two strings");
        return RUNTIME_ERROR;
      }
      break;
    }
    case OP_CALL: {
      int arg_count = READ_BYTE();
      frame->ip = ip;
      if (!call_value(peek(arg_count), arg_count)) {
        return RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frame_count - 1];
      ip = frame->ip;
      break;
    }
    case OP_CLOSURE: {
      ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
      ObjClosure *closure = new_closure(function);
      push(OBJ_VAL(closure));
      for (int i = 0; i < closure->upvalue_count; i++) {
        uint8_t is_local = READ_BYTE();
        uint8_t index = READ_BYTE();
        if (is_local) {
          closure->upvalues[i] = capture_upvalue(frame->slots + index);
        } else {
          closure->upvalues[i] = frame->closure->upvalues[index];
        }
      }
      break;
    }
    case OP_CLOSE_UPVALUE: {
      close_upvalues(vm.stack_top - 1);
      pop();
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
        frame->ip = ip;
        runtime_error("Undefined variable '%s'.", name->chars);
        return RUNTIME_ERROR;
      }
      push(value);
      break;
    }
    case OP_GET_LOCAL: {
      uint8_t slot = READ_BYTE();
      push(frame->slots[slot]);
      break;
    }
    case OP_GET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      Value val = *frame->closure->upvalues[slot]->location;
      push(val);
      break;
    }
    case OP_GREATER: {
      BINARY_OP(BOOL_VAL, >);
      break;
    }
    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      ip += offset;
      break;
    }
    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      if (is_falsey(peek(0))) {
        ip += offset;
      }
      break;
    }
    case OP_JUMP_IF_TRUE: {
      uint16_t offset = READ_SHORT();
      if (!is_falsey(peek(0))) {
        ip += offset;
      }
      break;
    }
    case OP_JUMP_SWITCH: {
      uint16_t offset = READ_SHORT();
      Value a = pop();
      if (!values_equal(a, peek(0))) {
        ip += offset;
      }
      break;
    }
    case OP_LESS: {
      BINARY_OP(BOOL_VAL, <);
      break;
    }
    case OP_LOOP: {
      uint16_t offset = READ_SHORT();
      ip -= offset;
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
        frame->ip = ip;
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
      Value result = pop();
      close_upvalues(frame->slots);
      vm.frame_count--;
      if (vm.frame_count == 0) {
        pop();
        return OK;
      }
      vm.stack_top = frame->slots;
      push(result);
      frame = &vm.frames[vm.frame_count - 1];
      ip = frame->ip;
      break;
    }
    case OP_SET_GLOBAL: {
      ObjString *name = READ_STRING();
      if (table_set(&vm.globals, name, peek(0))) {
        table_delete(&vm.globals, name);
        frame->ip = ip;
        runtime_error("Undefined variable '%s'.", name->chars);
        return RUNTIME_ERROR;
      }
      break;
    }
    case OP_SET_LOCAL: {
      uint8_t slot = READ_BYTE();
      frame->slots[slot] = peek(0);
      break;
    }
    case OP_SET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      *frame->closure->upvalues[slot]->location = peek(0);
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
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

void init_vm() {
  reset_stack();
  vm.objects = NULL;
  vm.gray_count = 0;
  vm.gray_capacity = 0;
  vm.gray_stack = NULL;
  vm.bytes_allocated = 0;
  vm.next_gc = 1024 * 1024;
  init_table(&vm.globals);
  init_table(&vm.strings);

  define_native("clock", clock_native, 0);
}

void free_vm() {
  free_objects();
  free_table(&vm.globals);
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
  ObjFunction *function = compile(source);
  if (function == NULL) {
    return COMPILE_ERROR;
  }

  push(OBJ_VAL(function));
  ObjClosure *closure = new_closure(function);
  pop();
  push(OBJ_VAL(closure));
  call(closure, 0);

  return run();
}
