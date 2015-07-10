CC?=gcc
OBJ=connection.o irc.o internal.o callback.o callback_functions.o main.o
BIN=ircd

.PHONY: all
all: $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $@ $^

%.o: %.c %.h
	$(CC) -Wall -Wextra -o $@ -c $<

.PHONY: clean
clean:
	rm -rf $(OBJ)
	rm -rf $(BIN)