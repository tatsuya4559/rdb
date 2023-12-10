#include <stdio.h>
#include "util.h"

void die(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

InputBuffer *new_input_buffer() {
  InputBuffer *b = malloc(sizeof(InputBuffer));
  if (!b) {
    die("malloc");
  }
  b->buf = NULL;
  b->buf_len = 0;
  b->input_len = 0;
  return b;
}

void close_input_buffer(InputBuffer *b) {
  free(b->buf);
  free(b);
}
