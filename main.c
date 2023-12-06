#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void die(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

typedef struct {
  char *buf;
  size_t buf_len;
  ssize_t input_len;
} InputBuffer;

InputBuffer *InputBuffer_new() {
  InputBuffer *b = malloc(sizeof(InputBuffer));
  if (!b) {
    die("malloc");
  }
  b->buf = NULL;
  b->buf_len = 0;
  b->input_len = 0;
  return b;
}

void InputBuffer_close(InputBuffer *b) {
  free(b->buf);
  free(b);
}

void print_prompt() {
  printf("db> ");
}

void read_input(InputBuffer *b) {
  ssize_t bytes_read = getline(&(b->buf), &(b->buf_len), stdin);
  if (bytes_read <= 0) {
    die("getline");
  }

  // ignore trailing newline
  b->input_len = bytes_read - 1;
  b->buf[bytes_read - 1] = '\0';
}

int main(int argc, char **argv) {
  InputBuffer *b = InputBuffer_new();
  for (;;) {
    print_prompt();
    read_input(b);

    if (strcmp(b->buf, ".exit") == 0) {
      InputBuffer_close(b);
      exit(EXIT_SUCCESS);
    } else {
      printf("Unrecognized command '%s'.\n", b->buf);
    }
  }
}
