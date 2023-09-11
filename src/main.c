#include <stdio.h>

#include "chunk.h"
#include "common.h"
#include "debug.h"

int main(int argc, char *argv[]) {
  Chunk chunk;
  init_chunk(&chunk);
  int constant = add_constant(&chunk, 1.2);
  write_chunk(&chunk, OP_CONSTANT, 1);
  write_chunk(&chunk, constant, 1);
  write_chunk(&chunk, OP_CONSTANT, 2);
  write_chunk(&chunk, constant, 2);
  write_chunk(&chunk, OP_CONSTANT, 2);
  write_chunk(&chunk, constant, 2);
  write_chunk(&chunk, OP_RETURN, 3);
  write_chunk(&chunk, OP_CONSTANT, 4);
  write_chunk(&chunk, constant, 4);
  disassemble_chunk(&chunk, "test chunk");
  free_chunk(&chunk);
  printf("We finished the function\n");
  return 0;
}
