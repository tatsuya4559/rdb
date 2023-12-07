#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "storage.h"
#include "util.h"

#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)0)->Attribute)

/* Row */
static const uint32_t ID_SIZE = size_of_attribute(Row, id);
static const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
static const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
static const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

static const uint32_t ID_OFFSET = 0;
static const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
static const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

void print_row(Row *row) {
  printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

void serialize_row(Row *src, void *dest) {
  memcpy(dest + ID_OFFSET, &(src->id), ID_SIZE);
  memcpy(dest + USERNAME_OFFSET, &(src->username), USERNAME_SIZE);
  memcpy(dest + EMAIL_OFFSET, &(src->email), EMAIL_SIZE);
}

void deserialize_row(void *src, Row *dest) {
  memcpy(&(dest->id), src + ID_OFFSET, ID_SIZE);
  memcpy(&(dest->username), src + USERNAME_OFFSET, USERNAME_SIZE);
  memcpy(&(dest->email), src + EMAIL_OFFSET, EMAIL_SIZE);
}

/* Table */
static const uint32_t PAGE_SIZE = 4096;
static const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = TABLE_MAX_PAGES * ROWS_PER_PAGE;

static Pager *Pager_new(const char *filename) {
  int fd = open(filename, O_RDWR|O_CREAT, S_IWUSR|S_IRUSR);
  if (fd == -1) die("open(2)");

  off_t file_len = lseek(fd, 0, SEEK_END);
  if (file_len == -1) die("lseek");

  Pager *pager = malloc(sizeof(Pager));
  pager->fd = fd;
  pager->file_len = file_len;
  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
    pager->pages[i] = NULL;
  }

  return pager;
}

static void Pager_flush(Pager *p, uint32_t page_num, size_t size) {
  if (p->pages[page_num] == NULL) {
    fprintf(stderr, "Tried to flush null page.\n");
    exit(EXIT_FAILURE);
  }
  if(lseek(p->fd, page_num * PAGE_SIZE, SEEK_SET) == -1) die("lseek");
  if(write(p->fd, p->pages[page_num], size) == -1) die("write(2)");
}

static void Pager_free(Pager *p, uint32_t num_rows) {
  uint32_t num_full_pages = num_rows / ROWS_PER_PAGE;
  for (uint32_t i = 0; i < num_full_pages; i++) {
    if (p->pages[i] == NULL) {
      continue;
    }
    Pager_flush(p, i, PAGE_SIZE);
    free(p->pages[i]);
    p->pages[i] = NULL;
  }

  // There may be a partial page to write to the end of file
  uint32_t num_additional_rows = num_rows % ROWS_PER_PAGE;
  if (num_additional_rows > 0) {
    uint32_t page_num = num_full_pages;
    if (p->pages[page_num] != NULL) {
      Pager_flush(p, page_num, num_additional_rows * ROW_SIZE);
      free(p->pages[page_num]);
      p->pages[page_num] = NULL;
    }
  }

  if (close(p->fd) == -1) die("close(2)");
  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
    void *page = p->pages[i];
    if (page) {
      free(page);
      p->pages[i] = NULL;
    }
  }
  free(p);
}

static void *Pager_get_page(Pager *p, uint32_t page_num) {
  if (page_num > TABLE_MAX_PAGES) {
    fprintf(stderr, "Tried to fetch page number out of bounds. %d > %d\n",
        page_num, TABLE_MAX_PAGES);
    exit(EXIT_FAILURE);
  }

  if (p->pages[page_num] == NULL) {
    // Cache miss. Allocate memory and load from file.
    void *page = malloc(PAGE_SIZE);
    uint32_t num_pages = p->file_len / PAGE_SIZE;

    // We might save a partial page at the end of the file.
    if (p->file_len % PAGE_SIZE) {
      num_pages++;
    }

    if (page_num <= num_pages) {
      if (lseek(p->fd, page_num * PAGE_SIZE, SEEK_SET) == -1) die("lseek");
      if (read(p->fd, page, PAGE_SIZE) == -1) die("read(2)");
    }

    p->pages[page_num] = page;
  }

  return p->pages[page_num];
}

Table *Table_new(const char *filename) {
  Pager *p = Pager_new(filename);

  Table *table = malloc(sizeof(Table));
  table->pager = p;
  table->num_rows = p->file_len / ROW_SIZE;
  return table;
}

void Table_free(Table *table) {
  Pager_free(table->pager, table->num_rows);
  free(table);
}

/* Cursor */
Cursor *table_start(Table *table) {
  Cursor *c = malloc(sizeof(Cursor));
  c->table = table;
  c->row_num = 0;
  c->end_of_table = (table->num_rows == 0);
  return c;
}

Cursor *table_end(Table *table) {
  Cursor *c = malloc(sizeof(Cursor));
  c->table = table;
  c->row_num = table->num_rows;
  c->end_of_table = true;
  return c;
}

void *cursor_value(Cursor *c) {
  uint32_t row_num = c->row_num;
  uint32_t page_num = row_num / ROWS_PER_PAGE;
  void *page = Pager_get_page(c->table->pager, page_num);
  uint32_t row_offset = row_num % ROWS_PER_PAGE;
  uint32_t byte_offset = row_offset * ROW_SIZE;
  return page + byte_offset;
}

void cursor_advance(Cursor *c) {
  assert(!c->end_of_table);

  c->row_num++;
  if (c->row_num >= c->table->num_rows) {
    c->end_of_table = true;
  }
}
