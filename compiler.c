#include <stdio.h>
#include <string.h>
#include "compiler.h"

PrepareResult prepare_statement(InputBuffer *b, Statement *stmt) {
  if (strncmp(b->buf, "insert", 6) == 0) {
    stmt->type = STATEMENT_INSERT;
    int args_assigned = sscanf(
        b->buf, "insert %d %s %s",
        &(stmt->row_to_insert.id),
        stmt->row_to_insert.username,
        stmt->row_to_insert.email);
    if (args_assigned < 3) {
      return PREPARE_SYNTAX_ERROR;
    }
    return PREPARE_SUCCESS;
  }
  if (strncmp(b->buf, "select", 6) == 0) {
    stmt->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }
  return PREPARE_UNRECOGNIZED_STATEMENT;
}

