CC = gcc -g
CCO = gcc -o
INCLUDE = include/
SOURCE = src/
BIN = bin/
OBJ = obj/
FLAGS = -Wextra -Werror -Wall
MOVEBIN = mv $@ $(BIN)

all: target client server clean

target:
	mkdir -p $(BIN)
	mkdir -p $(OBJ)

client: client.o
	$(CCO) client $(OBJ)client.o -lncurses
	$(MOVEBIN)

server: server.o
	$(CCO) server $(OBJ)server.o
	$(MOVEBIN)

server.o: $(SOURCE)server.c 
	$(CCO) server.o -c $(SOURCE)server.c $(FLAGS)
	mv server.o $(OBJ)

client.o: $(SOURCE)client.c
	$(CCO) client.o -c $(SOURCE)client.c $(FLAGS) -lncurses
	mv client.o $(OBJ)

clean: 
	rm -rf $(OBJ)

