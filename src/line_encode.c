#include "line_encode.h"
#include "memory.h"
#include <stdio.h>

void init_lines(LineEncode *encode) {
  encode->capacity = 0;
  encode->count = 0;
  encode->arr = NULL;
}

void write_lines(LineEncode *encode, int line) {
  if (encode->capacity < encode->count + 2) {
    int prev_capacity = encode->capacity;
    encode->capacity = GROW_CAPACITY(prev_capacity);
    encode->arr = GROW_ARRAY(int, encode->arr, prev_capacity, encode->capacity);
  }
  if (encode->count < 1) {
    encode->arr[0] = line;
    encode->arr[1] = 1;
    encode->count = 2;
  } else {
    int curr_line = encode->arr[encode->count - 2];
    if (curr_line == line) {
      encode->arr[encode->count - 1] = encode->arr[encode->count - 1] + 1;
    } else {
      encode->arr[encode->count] = line;
      encode->arr[encode->count + 1] = 1;
      encode->count = encode->count + 2;
    }
  }
}

void free_lines(LineEncode *encode) {
  FREE_ARRAY(int, encode->arr, encode->capacity);
  init_lines(encode);
}

int get_line(LineEncode *encode, int offset) {
  int curr = 0;
  for (int i = 1; i < encode->count + 1; i = i + 2) {
    curr += encode->arr[i];
    if (curr > offset) {
      return encode->arr[i - 1];
    }
  }
  return -1;
}
