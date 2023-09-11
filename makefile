CC=gcc
CFLAGS=-Iinclude
exec = clox
sources = $(wildcard src/*.c)
objects = $(sources:.c=.o)
flags = -Wall -I./include

$(exec): $(objects)
	gcc $(objects) $(flags) -o build/$(exec)

%.o: %.c %.h
	@gcc -c $(flags) $< -o $@

.PHONY: clean
clean:
	@rm src/*.o build/*

.PHONY: debug
debug: $(objects)
	gcc $(objects) -g $(flags) -o build/d_$(exec)
