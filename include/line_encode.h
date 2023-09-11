#ifndef clox_line_h
#define clox_line_h

#include "common.h"

typedef struct {
  int capacity;
  int count;
  int *arr;
} LineEncode;

void init_lines(LineEncode *encode);
void write_lines(LineEncode *encode, int line);
void free_lines(LineEncode *encode);
int get_line(LineEncode *encode, int offset);

#endif
