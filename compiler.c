#include <string.h>
#include "compiler.h"

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

