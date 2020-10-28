#include "encrypt.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

FILE *input_file;
FILE *output_file;
int input_counts[256];
int output_counts[256];
int key = 1;

void clear_counts() {
	memset(input_counts, 0, sizeof(input_counts));
	memset(output_counts, 0, sizeof(output_counts));
}

void *random_reset() {
	while (1) {
		sleep(rand() % 11 + 5);
		reset_requested();
		key = rand() % 26;
		clear_counts();
		reset_finished();
	}
}

void init() {
	pthread_t pid;
	srand(time(0));
	pthread_create(&pid, NULL, &random_reset, NULL);
}

void open_input(char *name) {
	init();
	input_file = fopen(name, "r");
}

void open_output(char *name) {
	output_file = fopen(name, "w");
}

int read_input() {
	usleep(10000);
	return fgetc(input_file);
}

void write_output(int c) {
	fputc(c, output_file);
}

int caesar_encrypt(int c) {
	if (c >= 'a' && c <= 'z') {
		c += key;
		if (c > 'z') {
			c = c - 'z' + 'a' - 1;
		}
	} else if (c >= 'A' && c <= 'Z') {
		c += key;
		if (c > 'Z') {
			c = c - 'Z' + 'A' - 1;
		}
	}
	return c;
}

void count_input(int c) {
	input_counts[toupper(c)]++;
}

void count_output(int c) {
	output_counts[toupper(c)]++;
}

int get_input_count(int c) {
	return input_counts[c];
}

int get_output_count(int c) {
	return output_counts[c];
}

/* Start of code by Robert Shay */

typedef struct count {
	char c;
	int occur;
} count;

count counts[26];

// TODO
void reset_requested() {

}

// TODO
void reset_finished() {

}

void *read_file() {

}

void get_input(char *cmdline) {
	fflush(stdout);
	printf("Enter buffer size: ");

	fgets(cmdline, 100, stdin);
	ferror(stdin);
}

int main(int argc, char** argv) {
	char* infile;
	char* outfile;
	char cmdline[100];
	int buffer_size;
	pthread_t reader, writer, encryptor, input, output;
	int reader_id = 1, writer_id = 2, encryptor_id = 3, input_id = 4, output_id = 5;

	if (argc != 3) {
  	printf("Invalid arguments\n");
		return 1;
	}

	open_input(argv[1]);
	open_output(argv[2]);

	get_input(cmdline);

	printf("\n%s\n", cmdline);

	buffer_size = atoi(cmdline);

	pthread_create(&reader, NULL, read_file, &reader_id);
	pthread_create(&writer, NULL, read_file, &writer_id);
	pthread_create(&encryptor, NULL, read_file, &encryptor_id);
	pthread_create(&input, NULL, read_file, &input_id);
	pthread_create(&output, NULL, read_file, &output_id);

	pthread_join(reader, NULL);
	pthread_join(writer, NULL);
	pthread_join(encryptor, NULL);
	pthread_join(input, NULL);
	pthread_join(output, NULL);

  return 0;
}
