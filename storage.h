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

/* Table */
extern const uint32_t TABLE_MAX_ROWS;

typedef struct {
  uint32_t num_rows;
  Pager *pager;
} Table;

Table *Table_new(const char *filename);
void Table_free(Table *table);

/* Cursor */
typedef struct {
  Table *table;
  uint32_t row_num;
  bool end_of_table;
} Cursor;

Cursor *table_start(Table *table);
Cursor *table_end(Table *table);
void *Cursor_get_slot(Cursor *c);
void Cursor_advance(Cursor *c);
