#include <stdio.h>

#include "chunk.h"
#include "debug.h"
#include "line_encode.h"
#include "value.h"

void disassemble_chunk(Chunk *chunk, const char *name) {
  printf("== %s ==\n", name);
  for (int offset = 0; offset < chunk->count;) {
    offset = disassemble_instr(chunk, offset);
  }
}

static int simple_instr(const char *name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

static int const_instr(const char *name, Chunk *chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d ", name, constant);
  print_value(chunk->constants.values[constant]);
  printf("\n");
  return offset + 2;
}

int disassemble_instr(Chunk *chunk, int offset) {
  printf("%04d ", offset);
  int prev_line = get_line(&chunk->encode, offset - 1);
  int line = get_line(&chunk->encode, offset);
  if (offset > 0 && prev_line == line) {
    printf("   | ");
  } else {
    printf("%4d ", line);
  }

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_CONSTANT:
    return const_instr("OP_CONSTANT", chunk, offset);
  case OP_RETURN:
    return simple_instr("OP_RETURN", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}
