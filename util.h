#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdlib.h>

void die(const char *msg);

typedef struct {
  char *buf;
  size_t buf_len;
  ssize_t input_len;
} InputBuffer;

InputBuffer *InputBuffer_new();
void InputBuffer_close(InputBuffer *b);

#endif /* _UTIL_H_ */