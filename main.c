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

typedef enum {
  PREPARE_SUCCESS,
  PREPARE_UNRECOGNIZED_STATEMENT,
} PrepareResult;

static MetaCommandResult do_meta_command(InputBuffer *b) {
  if (strcmp(b->buf, ".exit") == 0) {
    InputBuffer_close(b);
    exit(EXIT_SUCCESS);
  } else {
    return META_COMMAND_UNRECOGNIZED_COMMAND;
  }
}

typedef enum {
  STATEMENT_INSERT,
  STATEMENT_SELECT,
} StatementType;

typedef struct {
  StatementType type;
} Statement;

PrepareResult prepare_statement(InputBuffer *b, Statement *stmt) {
  if (strncmp(b->buf, "insert", 6) == 0) {
    stmt->type = STATEMENT_INSERT;
    return PREPARE_SUCCESS;
  }
  if (strncmp(b->buf, "select", 6) == 0) {
    stmt->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }
  return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement *stmt) {
  switch (stmt->type) {
  case STATEMENT_INSERT:
    printf("This is where we would do an insert.\n");
    break;
  case (STATEMENT_SELECT):
    printf("This is where we would do a select.\n");
    break;
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
