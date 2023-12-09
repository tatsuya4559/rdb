#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "engine.h"
#include "storage.h"

static ExecuteResult execute_insert(Statement *stmt, Table *table) {
  void *node = get_page(table->pager, table->root_page_num);
  if (*leaf_node_num_cells(node) >= LEAF_NODE_MAX_CELLS) {
    return EXECUTE_TABLE_FULL;
  }

  Row *row_to_insert = &(stmt->row_to_insert);
  Cursor *c = table_end(table);
  leaf_node_insert(c, row_to_insert->id, row_to_insert);

  free(c);

  return EXECUTE_SUCCESS;
}

static ExecuteResult execute_select(Statement *stmt, Table *table) {
  Cursor *c = table_start(table);
  Row row;
  while (!c->end_of_table) {
    deserialize_row(cursor_get_slot(c), &row);
    print_row(&row);
    cursor_advance(c);
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
    db_close(table);
    InputBuffer_close(b);
    exit(EXIT_SUCCESS);
  } else if (strcmp(b->buf, ".constants") == 0) {
    printf("Constants:\n");
    print_constants();
    return META_COMMAND_SUCCESS;
  } else if (strcmp(b->buf, ".btree") == 0) {
    printf("Tree:\n");
    print_leaf_node(get_page(table->pager, table->root_page_num));
    return META_COMMAND_SUCCESS;
  } else {
    return META_COMMAND_UNRECOGNIZED_COMMAND;
  }
}
