#include "encrypt.h"
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct buffer {
  char *data;
  unsigned int size;
} buffer_t;

typedef struct counter_args {
  sem_t *counter_full_sem;
  sem_t *counter_empty_sem;
  buffer_t *buffer;
} counter_args_t;

buffer_t *buffer_in;
buffer_t *buffer_out;

sem_t *in_counter_empty_sem;
sem_t *in_counter_full_sem;

sem_t *in_encryptor_empty_sem;
sem_t *in_encryptor_full_sem;

sem_t *out_counter_empty_sem;
sem_t *out_counter_full_sem;

sem_t *out_writer_empty_sem;
sem_t *out_writer_full_sem;

sem_t *print_order_sem;

/**
 * Prints the the current count for the inputs.
 */
void print_input_count() {
  printf("Input file contains\n");

  for (int i = 0; i < 26; i++) {
    printf("%c:%d ", i + 'A', get_input_count(i));
  }
  printf("\r\n");
}

/**
 * Prints the the current count for the outputs.
 */
void print_output_count() {
  printf("Output file contains\n");

  for (int i = 0; i < 26; i++) {
    printf("%c:%d ", i + 'A', get_output_count(i));
  }
  printf("\r\n");
}

void reset_requested() {
  // Wait for bother counters to be empty
  sem_wait(in_counter_empty_sem);
  sem_wait(out_counter_empty_sem);
  // Print input and output
  print_input_count();
  print_output_count();
}

void reset_finished() {
  printf("Reset finished\r\n");
  // Signal that the counters are empty
  sem_post(in_counter_empty_sem);
  sem_post(out_counter_empty_sem);
}

/**
 * Gets the char from the buffer at the given pos. Increments the position by 1
 * or goes back to 0 if it hits the end of the buffer.
 */
void get_c_and_pos(buffer_t *buffer, int *ch, int *pos) {
  *ch = buffer->data[*pos];
  *pos = (*pos + 1) % buffer->size;
}

/**
 * Sets the buffer to the given char at the given position. Increments the
 * position by 1 or goes back to 0 if it hits the end of the buffer.
 */
void set_buffer_and_pos(buffer_t *buffer, int ch, int *pos) {
  buffer->data[*pos] = ch;
  *pos = (*pos + 1) % buffer->size;
}

/**
 * Function to run on a thread that reads the input file.
 */
void *reader_func() {
  int ch = 0;
  int pos = 0;

  while (ch != EOF) {
    // Get the input from the file.
    ch = read_input();

    // Wait for the encryptor and in counters to be empty
    sem_wait(in_counter_empty_sem);
    sem_wait(in_encryptor_empty_sem);

    // Update the in buffer
    set_buffer_and_pos(buffer_in, ch, &pos);

    // Signal that the in counter and encryptor are full
    sem_post(in_counter_full_sem);
    sem_post(in_encryptor_full_sem);
  }

  // Exit this thread
  pthread_exit(NULL);
}

/**
 * Function to run on a thread that counts input or output.
 */
void *counter_func(void *counter_args) {
  counter_args_t *args = (counter_args_t *)counter_args;

  int counts[26] = {0};
  int i;

  int ch = '\0';
  int pos = 0;
  while (EOF != ch) {
    // Wait for the given sem
    sem_wait(args->counter_full_sem);

    // Get the char and update the position
    get_c_and_pos(args->buffer, &ch, &pos);

    // Gets the index of the char
    i = toupper(ch) - 'A';
    if (i >= 0 && i < 26) {

      // Count char as input if this is the in counter
      if (args->buffer == buffer_in) {
        count_input(i);
        // Count char as output if this is the out counter
      } else if (args->buffer == buffer_out) {
        count_output(i);
      }
    }

    // Signal the given sem
    sem_post(args->counter_empty_sem);
  }

  // Print the output
  if (args->buffer == buffer_in) {
    print_input_count();
  } else if (args->buffer == buffer_out) {
    sem_wait(print_order_sem);
    print_output_count();
  }

  sem_post(print_order_sem);

  // Exit this thread
  pthread_exit(NULL);
}

/**
 * Function to run on a thread that encrypts the input.
 */
void *encryptor_func() {
  int s = 1;
  int pos_o = 0;
  int pos_i = 0;
  int ch = '\0';

  while (EOF != ch) {
    // Wait for the encryptor to be full
    sem_wait(in_encryptor_full_sem);

    // Get the unecrypted char
    get_c_and_pos(buffer_in, &ch, &pos_i);

    // Signal that the encryptor is empty
    sem_post(in_encryptor_empty_sem);

    // Encrypt the char
    ch = caesar_encrypt(ch);

    // Wait for the out counter and writer to be empty
    sem_wait(out_counter_empty_sem);
    sem_wait(out_writer_empty_sem);

    // Update the out buffer
    set_buffer_and_pos(buffer_out, ch, &pos_o);

    // Signal the buffer is ready to be counted and outputted
    sem_post(out_counter_full_sem);
    sem_post(out_writer_full_sem);
  }

  // Exit this thread
  pthread_exit(NULL);
}

/**
 * Function to run on a thread that writes to the output file.
 */
void *writer_func() {
  int pos = 0;
  int ch = '\0';
  while (EOF != ch) {

    // Wait for the writer to be full
    sem_wait(out_writer_full_sem);

    get_c_and_pos(buffer_out, &ch, &pos);

    // Signal that the writer is empty
    sem_post(out_writer_empty_sem);

    // Write the output if not EOF
    if (EOF != ch)
      write_output(ch);
  }

  // Exit this thread
  pthread_exit(NULL);
}

/**
 * Gets the buffer size from stdin.
 */
unsigned get_input() {
  int buffer = 0;
  while (buffer <= 0) {
    printf("Enter buffer size: ");
    scanf("%d", &buffer);
  }
  return buffer;
}

/**
 * Allocates a new buffer.
 */
buffer_t *new_buffer(unsigned buffer_size) {
  buffer_t *buffer = malloc(sizeof(buffer_t));
  buffer->size = buffer_size;
  buffer->data = malloc(sizeof(char) * buffer_size);
  return buffer;
}

/**
 * Frees the given buffer.
 */
void cleanup_buffer(buffer_t *buffer) {
  free(buffer->data);
  free(buffer);
}

/**
 * Initializes in and out buffers.
 */
void initialize_buffers(unsigned buffer_size) {
  buffer_in = new_buffer(buffer_size);
  buffer_out = new_buffer(buffer_size);
}

/**
 * Cleans in and out buffers.
 */
void cleanup_buffers() {
  cleanup_buffer(buffer_in);
  cleanup_buffer(buffer_out);
}

/**
 * Allocate and initializes semaphores.
 */
void initialize_semaphores(int buffer_size) {
  in_counter_empty_sem = malloc(sizeof(sem_t));
  in_counter_full_sem = malloc(sizeof(sem_t));
  sem_init(in_counter_empty_sem, 0, buffer_size);
  sem_init(in_counter_full_sem, 0, 0);

  in_encryptor_empty_sem = malloc(sizeof(sem_t));
  in_encryptor_full_sem = malloc(sizeof(sem_t));
  sem_init(in_encryptor_empty_sem, 0, buffer_size);
  sem_init(in_encryptor_full_sem, 0, 0);

  out_counter_empty_sem = malloc(sizeof(sem_t));
  out_counter_full_sem = malloc(sizeof(sem_t));
  sem_init(out_counter_empty_sem, 0, buffer_size);
  sem_init(out_counter_full_sem, 0, 0);

  out_writer_empty_sem = malloc(sizeof(sem_t));
  out_writer_full_sem = malloc(sizeof(sem_t));
  sem_init(out_writer_empty_sem, 0, buffer_size);
  sem_init(out_writer_full_sem, 0, 0);

  print_order_sem = malloc(sizeof(sem_t));
  sem_init(print_order_sem, 0, 0);
}

void cleanup_semaphores() {
  free(in_counter_empty_sem);
  free(in_counter_full_sem);
  free(in_encryptor_empty_sem);
  free(in_encryptor_full_sem);

  free(out_counter_empty_sem);
  free(out_counter_full_sem);
  free(out_writer_empty_sem);
  free(out_writer_full_sem);

  free(print_order_sem);
}

int main(int argc, char **argv) {
  char *infile;
  char *outfile;
  int buffer_size;
  pthread_t reader_t, writer_t, encryptor_t, input_counter_t, output_counter_t;

  if (argc != 3) {
    printf("Invalid arguments\n");
    return 1;
  }

  open_input(argv[1]);
  open_output(argv[2]);

  buffer_size = get_input();

  initialize_buffers(buffer_size);
  initialize_semaphores(buffer_size);

  counter_args_t counter_in_args;
  counter_in_args.counter_empty_sem = in_counter_empty_sem;
  counter_in_args.counter_full_sem = in_counter_full_sem;
  counter_in_args.buffer = buffer_in;

  counter_args_t counter_out_args;
  counter_out_args.counter_empty_sem = out_counter_empty_sem;
  counter_out_args.counter_full_sem = out_counter_full_sem;
  counter_out_args.buffer = buffer_out;

  if (pthread_create(&reader_t, NULL, reader_func, NULL) == -1) {
    printf("Error starting reader thread\n");
    return 1;
  }
  if (pthread_create(&input_counter_t, NULL, counter_func, &counter_in_args) ==
      -1) {
    printf("Error starting in counter thread\n");
    return 2;
  }
  if (pthread_create(&encryptor_t, NULL, encryptor_func, NULL) == -1) {
    printf("Error starting encryptor thread\n");
    return 3;
  }
  if (pthread_create(&output_counter_t, NULL, counter_func,
                     &counter_out_args) == -1) {
    printf("Error starting out counter thread\n");
    return 4;
  }
  if (pthread_create(&writer_t, NULL, writer_func, NULL) == -1) {
    printf("Error starting writer thread\n");
    return 5;
  }

  pthread_join(reader_t, NULL);
  pthread_join(input_counter_t, NULL);
  pthread_join(encryptor_t, NULL);
  pthread_join(output_counter_t, NULL);
  pthread_join(writer_t, NULL);

  cleanup_semaphores();
  cleanup_buffers();

  return 0;
}
