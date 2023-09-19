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

static int byte_instr(const char *name, Chunk *chunk, int offset) {
  uint8_t slot = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2;
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
  case OP_ADD:
    return simple_instr("OP_ADD", offset);
  case OP_CONSTANT:
    return const_instr("OP_CONSTANT", chunk, offset);
  case OP_DEFINE_GLOBAL:
    return const_instr("OP_DEFINE_GLOBAL", chunk, offset);
  case OP_DIVIDE:
    return simple_instr("OP_DIVIDE", offset);
  case OP_GET_GLOBAL:
    return const_instr("OP_GET_GLOBAL", chunk, offset);
  case OP_GET_LOCAL:
    return byte_instr("OP_GET_LOCAL", chunk, offset);
  case OP_POP:
    return simple_instr("OP_POP", offset);
  case OP_EQUAL:
    return simple_instr("OP_EQUAL", offset);
  case OP_FALSE:
    return simple_instr("OP_FALSE", offset);
  case OP_GREATER:
    return simple_instr("OP_GREATER", offset);
  case OP_LESS:
    return simple_instr("OP_LESS", offset);
  case OP_MULTIPLY:
    return simple_instr("OP_MULTIPLY", offset);
  case OP_NEGATE:
    return simple_instr("OP_NEGATE", offset);
  case OP_NIL:
    return simple_instr("OP_NIL", offset);
  case OP_NOT:
    return simple_instr("OP_NOT", offset);
  case OP_PRINT:
    return simple_instr("OP_PRINT", offset);
  case OP_RETURN:
    return simple_instr("OP_RETURN", offset);
  case OP_SET_GLOBAL:
    return const_instr("OP_SET_GLOBAL", chunk, offset);
  case OP_SET_LOCAL:
    return byte_instr("OP_SET_LOCAL", chunk, offset);
  case OP_SUBTRACT:
    return simple_instr("OP_SUBTRACT", offset);
  case OP_TRUE:
    return simple_instr("OP_TRUE", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}
