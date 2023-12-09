#pragma once

#include <stdlib.h>

void die(const char *msg);

typedef struct {
  char *buf;
  size_t buf_len;
  ssize_t input_len;
} InputBuffer;

InputBuffer *new_input_buffer();
void close_input_buffer(InputBuffer *b);
