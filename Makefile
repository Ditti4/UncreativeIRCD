CC?=gcc
OBJ=build/connection.o
OBJ+=build/irc.o
OBJ+=build/internal.o
OBJ+=build/callback.o
OBJ+=build/callback_functions.o
OBJ+=build/main.o
BIN=ircd

.PHONY: all
all: mkbuilddir $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $@ $^ -lrt

build/%.o: src/%.c include/%.h
	$(CC) -Wall -Wextra -Wshadow -O2 -Iinclude -o $@ -c $<

.PHONY: mkbuilddir
mkbuilddir:
	mkdir -p build

.PHONY: clean
clean:
	rm -rf $(BIN)
	rm -rf build/*
