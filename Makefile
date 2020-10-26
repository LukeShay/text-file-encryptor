CC=gcc
CFLAGS=-I.
DEPS = encrypt.h
OBJ = encrypt.o main.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

build: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)