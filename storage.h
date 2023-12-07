#ifndef _BACKEND_H_
#define _BACKEND_H_

#include <stdint.h>

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

#define TABLE_MAX_PAGES 100
extern const uint32_t TABLE_MAX_ROWS;

typedef struct {
  int fd;
  uint32_t file_len;
  void *pages[TABLE_MAX_PAGES];
} Pager;

typedef struct {
  uint32_t num_rows;
  Pager *pager;
} Table;

Table *Table_new(const char *filename);
void Table_free(Table *table);
void *row_slot(Table *table, uint32_t row_num);

#endif /* _BACKEND_H_ */
