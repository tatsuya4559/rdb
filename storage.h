#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Row */
#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

typedef struct {
  uint32_t id;
  char username[COLUMN_USERNAME_SIZE + 1];
  char email[COLUMN_EMAIL_SIZE + 1];
} Row;

void print_row(Row *row);
void serialize_row(Row *src, void *dest);
void deserialize_row(void *src, Row *dest);

/* Pager */
typedef struct Pager_tag Pager;
void *get_page(Pager *p, uint32_t page_num);

/* Table */
extern const uint32_t TABLE_MAX_ROWS;

typedef struct {
  uint32_t root_page_num;
  Pager *pager;
} Table;

Table *db_open(const char *filename);
void db_close(Table *table);

/* Cursor */
typedef struct {
  Table *table;
  uint32_t page_num;
  uint32_t cell_num;
  bool end_of_table;
} Cursor;

Cursor *table_start(Table *table);
Cursor *table_end(Table *table);
void *cursor_get_slot(Cursor *c);
void cursor_advance(Cursor *c);

/* Node */
extern const uint32_t LEAF_NODE_MAX_CELLS;
void leaf_node_insert(Cursor *c, uint32_t key, Row *value);
uint32_t *leaf_node_num_cells(void *node);

void print_constants(void);
void print_leaf_node(void *node);
