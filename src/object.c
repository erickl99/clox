#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  object->next = vm.objects;
  vm.objects = object;
  return object;
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
  table_set(&vm.strings, string, NIL_VAL);
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
  case OBJ_STRING:
    printf("%s", AS_CSTRING(value));
    break;
  }
}
