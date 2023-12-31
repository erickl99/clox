#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"

void init_value_array(ValueArray *array) {
  array->capacity = 0;
  array->count = 0;
  array->values = NULL;
}

void write_value_array(ValueArray *array, Value value) {
  if (array->capacity < array->count + 1) {
    int prev_capacity = array->capacity;
    array->capacity = GROW_CAPACITY(prev_capacity);
    array->values =
        GROW_ARRAY(Value, array->values, prev_capacity, array->capacity);
  }
  array->values[array->count] = value;
  array->count++;
}

void free_value_array(ValueArray *array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  init_value_array(array);
}

void print_value(Value value) {
  switch (value.type) {
  case (VAL_BOOL):
    printf(AS_BOOL(value) ? "true" : "false");
    break;
  case (VAL_NIL):
    printf("nil");
    break;
  case (VAL_NUMBER):
    printf("%g", AS_NUMBER(value));
    break;
  case (VAL_OBJ):
    print_object(value);
    break;
  }
}

bool values_equal(Value a, Value b) {
  if (a.type != b.type) {
    return false;
  }
  switch (a.type) {
  case VAL_BOOL:
    return AS_BOOL(a) == AS_BOOL(b);
  case VAL_NIL:
    return true;
  case VAL_NUMBER:
    return AS_NUMBER(a) == AS_NUMBER(b);
  case VAL_OBJ: {
    return AS_OBJ(a) == AS_OBJ(b);
  }
  default:
    return false;
  }
}

int find_value(const ValueArray *array, Value value) {
  for (int i = 0; i < array->count; i++) {
    Value curr = array->values[i];
    if (IS_STRING(curr) && values_equal(curr, value)) {
      return i;
    }
  }
  return -1;
}
