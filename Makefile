CC=gcc
CFLAGS=-pthread -ggdb3 -O2
DEPS = encrypt.h
OBJ = encrypt.o main.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

encrypt: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf *.o encrypt outfile.txt lshay lshay.zip

zip: clean
	mkdir lshay
	cp Makefile encrypt.c encrypt.h lshay
	zip -r lshay lshay
