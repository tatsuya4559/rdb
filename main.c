#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "compiler.h"
#include "vm.h"

static void print_prompt() {
  printf("db> ");
}

static void read_input(InputBuffer *b) {
  ssize_t bytes_read = getline(&(b->buf), &(b->buf_len), stdin);
  if (bytes_read <= 0) {
    die("getline");
  }

  // ignore trailing newline
  b->input_len = bytes_read - 1;
  b->buf[bytes_read - 1] = '\0';
}

typedef enum {
  META_COMMAND_SUCCESS,
  META_COMMAND_UNRECOGNIZED_COMMAND,
} MetaCommandResult;

static MetaCommandResult do_meta_command(InputBuffer *b) {
  if (strcmp(b->buf, ".exit") == 0) {
    InputBuffer_close(b);
    exit(EXIT_SUCCESS);
  } else {
    return META_COMMAND_UNRECOGNIZED_COMMAND;
  }
}

int main(int argc, char **argv) {
  InputBuffer *b = InputBuffer_new();
  for (;;) {
    print_prompt();
    read_input(b);

    if (b->buf[0] == '.') {
      switch (do_meta_command(b)) {
      case META_COMMAND_SUCCESS:
        continue;
      case META_COMMAND_UNRECOGNIZED_COMMAND:
        printf("Unrecognized command '%s'.\n", b->buf);
        continue;
      }
    }

    Statement stmt;
    switch (prepare_statement(b, &stmt)) {
    case PREPARE_SUCCESS:
      break;
    case PREPARE_UNRECOGNIZED_STATEMENT:
      printf("Unrecognized keyword at start of '%s'.\n", b->buf);
      continue;
    }

    execute_statement(&stmt);
    printf("Executed.\n");
  }
}
