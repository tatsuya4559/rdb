#include <stdbool.h>
#include <assert.h>
#include "vm.h"

static ExecuteResult execute_insert(Statement *stmt, Table *table) {
  if (table->num_rows >= TABLE_MAX_ROWS) {
    return EXECUTE_TABLE_FULL;
  }

  Row *row_to_insert = &(stmt->row_to_insert);
  serialize_row(row_to_insert, row_slot(table, table->num_rows));
  table->num_rows++;

  return EXECUTE_SUCCESS;
}

static ExecuteResult execute_select(Statement *stmt, Table *table) {
  Row row;
  for (uint32_t i = 0; i < table->num_rows; i++) {
    deserialize_row(row_slot(table, i), &row);
    print_row(&row);
  }
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
