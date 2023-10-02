#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, object_type)                                        \
  (type *)allocate_object(sizeof(type), object_type)

static Obj *allocate_object(size_t size, ObjType type) {
  Obj *object = (Obj *)reallocate(NULL, 0, size);
  object->type = type;
  object->is_marked = false;
  object->next = vm.objects;
  vm.objects = object;

#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", (void *)object, size, type);
#endif
  return object;
}

ObjClosure *new_closure(ObjFunction *function) {
  ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue *, function->upvalue_count);

  for (int i = 0; i < function->upvalue_count; i++) {
    upvalues[i] = NULL;
  }
  ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalue_count = function->upvalue_count;
  return closure;
}

ObjFunction *new_function() {
  ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  function->arity = 0;
  function->upvalue_count = 0;
  function->name = NULL;
  init_chunk(&function->chunk);
  return function;
}

ObjNative *new_native(NativeFn function, int arity) {
  ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  native->arity = arity;
  return native;
}

ObjUpvalue *new_upvalue(Value *slot) {
  ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

static ObjString *allocate_string(const char *chars, int length,
                                  uint32_t hash) {
  ObjString *string = malloc(sizeof(ObjString) + sizeof(char) * length + 1);
  if (string == NULL) {
    exit(1);
  }
  string->length = length;
  string->hash = hash;
  memcpy(string->chars, chars, length);
  string->chars[length] = '\0';
  push(OBJ_VAL(string));
  table_set(&vm.strings, string, NIL_VAL);
  pop();
  Obj *object = (Obj *)string;
  object->type = OBJ_STRING;
  object->next = vm.objects;
  vm.objects = object;
  return string;
}

static uint32_t hash_string(const char *key, int length) {
  uint32_t hash = 216613261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

ObjString *copy_string(const char *chars, int length) {
  uint32_t hash = hash_string(chars, length);
  ObjString *interned = table_find_string(&vm.strings, chars, length, hash);

  if (interned != NULL) {
    return interned;
  }

  return allocate_string(chars, length, hash);
}

static void print_function(ObjFunction *function) {
  if (function->name == NULL) {
    printf("<script>");
  } else {
    printf("<fn %s>", function->name->chars);
  }
}

ObjString *take_string(char *chars, int length) {
  uint32_t hash = hash_string(chars, length);
  ObjString *interned = table_find_string(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    return interned;
  }
  return allocate_string(chars, length, hash);
}

void print_object(Value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_CLOSURE:
    print_function(AS_CLOSURE(value)->function);
    break;
  case OBJ_FUNCTION:
    print_function(AS_FUNCTION(value));
    break;
  case OBJ_NATIVE:
    printf("<native fn>");
    break;
  case OBJ_STRING:
    printf("%s", AS_CSTRING(value));
    break;
  case OBJ_UPVALUE:
    printf("upvalue");
    break;
  }
}
