CC=gcc
CFLAGS=-pthread -ggdb3 -O2
DEPS = encrypt.h
OBJ = encrypt.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

encrypt352: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf *.o encrypt352
