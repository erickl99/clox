#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
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

static ObjString *allocate_string(const char *chars, int length) {
  ObjString *string = malloc(sizeof(ObjString) + sizeof(char) * length + 1);
  string->length = length;
  memcpy(string->chars, chars, length);
  string->chars[length] = '\0';
  Obj *object = (Obj *)string;
  object->type = OBJ_STRING;
  object->next = vm.objects;
  vm.objects = object;
  return string;
}

ObjString *copy_string(const char *chars, int length) {
  return allocate_string(chars, length);
}

ObjString *take_string(char *chars, int length) {
  return allocate_string(chars, length);
}

void print_object(Value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_STRING:
    printf("%s", AS_CSTRING(value));
    break;
  }
}
