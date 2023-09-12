#include <stdio.h>

#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

int main(int argc, char *argv[]) {
  init_vm();
  Chunk chunk;
  init_chunk(&chunk);
  int c1 = add_constant(&chunk, 1.2);
  write_chunk(&chunk, OP_CONSTANT, 123);
  write_chunk(&chunk, c1, 123);

  int c2 = add_constant(&chunk, 3.4);
  write_chunk(&chunk, OP_CONSTANT, 123);
  write_chunk(&chunk, c2, 123);
  
  write_chunk(&chunk, OP_ADD, 123);

  int c3 = add_constant(&chunk, 5.6);
  write_chunk(&chunk, OP_CONSTANT, 123);
  write_chunk(&chunk, c3, 123);

  write_chunk(&chunk, OP_DIVIDE, 123);
  write_chunk(&chunk, OP_NEGATE, 123);

  write_chunk(&chunk, OP_RETURN, 123);
  interpret(&chunk);
  free_vm();
  free_chunk(&chunk);
  printf("We finished the function\n");
  return 0;
}
