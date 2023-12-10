#pragma once

#include <stdlib.h>

#ifdef DEBUG
  #define DEBUG_PRINT(fmt, ...) \
    fprintf(stderr, "[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
  #define DEBUG_PRINT(fmt, ...) do {} while (0)
#endif

void die(const char *msg);

typedef struct {
  char *buf;
  size_t buf_len;
  ssize_t input_len;
} InputBuffer;

InputBuffer *new_input_buffer();
void close_input_buffer(InputBuffer *b);
