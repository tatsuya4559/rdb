#ifndef _BACKEND_H_
#define _BACKEND_H_

#include <stdint.h>

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

typedef struct {
  uint32_t id;
  char username[COLUMN_USERNAME_SIZE];
  char email[COLUMN_USERNAME_SIZE];
} Row;

void print_row(Row *row);
void serialize_row(Row *src, void *dest);
void deserialize_row(void *src, Row *dest);

#define TABLE_MAX_PAGES 100
extern const uint32_t TABLE_MAX_ROWS;

typedef struct {
  uint32_t num_rows;
  void *pages[TABLE_MAX_PAGES];
} Table;

Table *Table_new();
void *row_slot(Table *table, uint32_t row_num);

#endif /* _BACKEND_H_ */
