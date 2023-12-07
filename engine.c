#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "engine.h"

static ExecuteResult execute_insert(Statement *stmt, Table *table) {
  if (table->num_rows >= TABLE_MAX_ROWS) {
    return EXECUTE_TABLE_FULL;
  }

  Row *row_to_insert = &(stmt->row_to_insert);
  Cursor *c = table_end(table);
  serialize_row(row_to_insert, Cursor_get_slot(c));
  table->num_rows++;

  free(c);

  return EXECUTE_SUCCESS;
}

static ExecuteResult execute_select(Statement *stmt, Table *table) {
  Cursor *c = table_start(table);
  Row row;
  while (!c->end_of_table) {
    deserialize_row(Cursor_get_slot(c), &row);
    print_row(&row);
    Cursor_advance(c);
  }
  free(c);
  return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement *stmt, Table *table) {
  switch (stmt->type) {
  case STATEMENT_INSERT:
    return execute_insert(stmt, table);
  case (STATEMENT_SELECT):
    return execute_select(stmt, table);
  default:
    assert(false);
  }
}

MetaCommandResult do_meta_command(InputBuffer *b, Table *table) {
  if (strcmp(b->buf, ".exit") == 0) {
    Table_free(table);
    InputBuffer_close(b);
    exit(EXIT_SUCCESS);
  } else {
    return META_COMMAND_UNRECOGNIZED_COMMAND;
  }
}
