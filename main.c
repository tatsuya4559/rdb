#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "util.h"
#include "query.h"
#include "engine.h"
#include "storage.h"

static void print_prompt() {
  printf("db> ");
}

static bool read_input(InputBuffer *b) {
  ssize_t bytes_read = getline(&(b->buf), &(b->buf_len), stdin);
  if (feof(stdin)) {
    return false;
  }
  if (bytes_read < 0) {
    die("getline");
  }

  // ignore trailing newline
  b->input_len = bytes_read - 1;
  b->buf[bytes_read - 1] = '\0';
  return true;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Must supply a database filename.\n");
    exit(EXIT_FAILURE);
  }

  char *filename = argv[1];
  Table *table = db_open(filename);
  InputBuffer *b = new_input_buffer();
  for (;;) {
    print_prompt();
    if (!read_input(b)) {
      do_exit(b, table);
    }

    if (b->buf[0] == '.') {
      switch (do_meta_command(b, table)) {
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
    case PREPARE_SYNTAX_ERROR:
      printf("Syntax error. Could not parse statement '%s'\n", b->buf);
      continue;
    case PREPARE_STRING_TOO_LONG:
      printf("String is too long.\n");
      continue;
    case PREPARE_NEGATIVE_ID:
      printf("ID must be positive.\n");
      continue;
    }

    switch (execute_statement(&stmt, table)) {
    case EXECUTE_SUCCESS:
      printf("Executed.\n");
      break;
    case EXECUTE_TABLE_FULL:
      printf("Error: Table full.\n");
      break;
    case EXECUTE_DUPLICATE_KEY:
      printf("Error: Duplicate key.\n");
      break;
    }
  }
}
