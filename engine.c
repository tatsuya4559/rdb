#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "engine.h"
#include "storage.h"

static ExecuteResult execute_insert(Statement *stmt, Table *table) {
  void *node = get_page(table->pager, table->root_page_num);
  uint32_t num_cells = *leaf_node_num_cells(node);

  Row *row_to_insert = &(stmt->row_to_insert);
  uint32_t key_to_insert = row_to_insert->id;
  Cursor *c = table_find(table, key_to_insert);

  if (c->cell_num < num_cells) {
    uint32_t key_at_index = *leaf_node_key(node, c->cell_num);
    if (key_at_index == key_to_insert) {
      return EXECUTE_DUPLICATE_KEY;
    }
  }

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

void do_exit(InputBuffer *b, Table *table) {
  db_close(table);
  close_input_buffer(b);
  exit(EXIT_SUCCESS);
}

MetaCommandResult do_meta_command(InputBuffer *b, Table *table) {
  if (strcmp(b->buf, ".exit") == 0) {
    do_exit(b, table);
  } else if (strcmp(b->buf, ".constants") == 0) {
    printf("Constants:\n");
    print_constants();
    return META_COMMAND_SUCCESS;
  } else if (strcmp(b->buf, ".btree") == 0) {
    printf("Tree:\n");
    print_tree(table->pager, 0, 0);
    return META_COMMAND_SUCCESS;
  }
  return META_COMMAND_UNRECOGNIZED_COMMAND;
}
