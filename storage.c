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

/* B-Tree */
#define TABLE_MAX_PAGES 100
static const uint32_t PAGE_SIZE = 4096;

typedef enum {
  NODE_INTERNAL,
  NODE_LEAF,
} NodeType;

/* Common Node Header Layout */
static const uint32_t NODE_TYPE_SIZE = sizeof(NodeType);
static const uint32_t NODE_TYPE_OFFSET = 0;
static const uint32_t IS_ROOT_SIZE = sizeof(bool);
static const uint32_t IS_ROOT_OFFSET = NODE_TYPE_OFFSET + NODE_TYPE_SIZE;
static const uint32_t PARENT_POINTER_SIZE = sizeof(uintptr_t);
static const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
static const uint32_t COMMON_NODE_HEADER_SIZE =
  NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

/* Leaf Node Header Layout */
static const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
static const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
static const uint32_t LEAF_NODE_HEADER_SIZE =
  COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

/* Leaf Node Body Layout */
static const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
static const uint32_t LEAF_NODE_KEY_OFFSET = 0;
static const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
static const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
static const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
static const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

uint32_t *leaf_node_num_cells(void *node) {
  return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

static void *leaf_node_cell(void *node, uint32_t cell_num) {
  return node + LEAF_NODE_HEADER_SIZE + LEAF_NODE_CELL_SIZE * cell_num;
}

static uint32_t *leaf_node_key(void *node, uint32_t cell_num) {
  return leaf_node_cell(node, cell_num);
}

static void *leaf_node_value(void *node, uint32_t cell_num) {
  return leaf_node_cell(node, cell_num) + LEAF_NODE_VALUE_OFFSET;
}

static void initialize_leaf_node(void *node) {
  *leaf_node_num_cells(node) = 0;
}

/* Pager */
struct Pager_tag {
  int fd;
  uint32_t file_len;
  uint32_t num_pages;
  void *pages[TABLE_MAX_PAGES];
};

static Pager *pager_open(const char *filename) {
  int fd = open(filename, O_RDWR|O_CREAT, S_IWUSR|S_IRUSR);
  if (fd == -1) die("open(2)");

  off_t file_len = lseek(fd, 0, SEEK_END);
  if (file_len == -1) die("lseek");

  Pager *pager = malloc(sizeof(Pager));
  pager->fd = fd;
  pager->file_len = file_len;
  pager->num_pages = (file_len / PAGE_SIZE);

  if (file_len % PAGE_SIZE != 0) {
    fprintf(stderr, "Db file is not a whole number of pages. Corrupt file.\n");
    exit(EXIT_FAILURE);
  }

  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
    pager->pages[i] = NULL;
  }

  return pager;
}

static void pager_flush(Pager *p, uint32_t page_num) {
  if (p->pages[page_num] == NULL) {
    return;
  }
  if(lseek(p->fd, page_num * PAGE_SIZE, SEEK_SET) == -1) die("lseek");
  if(write(p->fd, p->pages[page_num], PAGE_SIZE) == -1) die("write(2)");
}

static void pager_free(Pager *p) {
  for (uint32_t i = 0; i < p->num_pages; i++) {
    pager_flush(p, i);
    free(p->pages[i]);
    p->pages[i] = NULL;
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

void *get_page(Pager *p, uint32_t page_num) {
  if (page_num > TABLE_MAX_PAGES) {
    fprintf(stderr, "Tried to fetch page number out of bounds. %d > %d\n",
        page_num, TABLE_MAX_PAGES);
    exit(EXIT_FAILURE);
  }

  if (p->pages[page_num] == NULL) {
    // Cache miss. Allocate memory and load from file.
    void *page = malloc(PAGE_SIZE);
    p->pages[page_num] = page;

    if (page_num < p->num_pages) { /* page is in file */
      if (lseek(p->fd, page_num * PAGE_SIZE, SEEK_SET) == -1) die("lseek");
      if (read(p->fd, page, PAGE_SIZE) == -1) die("read(2)");
    } else { /* page is not in file */
      p->num_pages = page_num + 1;
    }
  }

  return p->pages[page_num];
}

/* Table */
Table *db_open(const char *filename) {
  Pager *p = pager_open(filename);

  Table *table = malloc(sizeof(Table));
  table->pager = p;
  table->root_page_num = 0;
  if (p->num_pages == 0) {
    // New database file. Initialize page 0 as leaf node.
    void *root_node = get_page(p, 0);
    initialize_leaf_node(root_node);
  }
  return table;
}

void db_close(Table *table) {
  pager_free(table->pager);
  free(table);
}

/* Cursor */
Cursor *table_start(Table *table) {
  Cursor *c = malloc(sizeof(Cursor));
  c->table = table;
  c->page_num = table->root_page_num;
  c->cell_num = 0;

  void *root_node = get_page(table->pager, table->root_page_num);
  uint32_t num_cells = *leaf_node_num_cells(root_node);
  c->end_of_table = (num_cells == 0);
  return c;
}

Cursor *table_end(Table *table) {
  Cursor *c = malloc(sizeof(Cursor));
  c->table = table;
  c->page_num = table->root_page_num;

  void *root_node = get_page(table->pager, table->root_page_num);
  uint32_t num_cells = *leaf_node_num_cells(root_node);
  c->cell_num = num_cells;
  c->end_of_table = true;
  return c;
}

void *cursor_get_slot(Cursor *c) {
  void *page = get_page(c->table->pager, c->page_num);
  return leaf_node_value(page, c->cell_num);
}

void cursor_advance(Cursor *c) {
  void *page = get_page(c->table->pager, c->page_num);
  c->cell_num++;
  if (c->cell_num >= (*leaf_node_num_cells(page))) {
    c->end_of_table = true;
  }
}

void leaf_node_insert(Cursor *c, uint32_t key, Row *value) {
  void *node = get_page(c->table->pager, c->page_num);

  uint32_t num_cells = *leaf_node_num_cells(node);
  if (num_cells >= LEAF_NODE_MAX_CELLS) {
    // Node is full
    fprintf(stderr, "Need to implement splitting a leaf node.\n");
    exit(EXIT_FAILURE);
  }

  if (c->cell_num < num_cells) {
    // Make room for new cell
    for (uint32_t i = num_cells; i > c->cell_num; i--) {
      memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i-1),
          LEAF_NODE_CELL_SIZE);
    }
  }

  *(leaf_node_num_cells(node)) += 1;
  *(leaf_node_key(node, c->cell_num)) = key;
  serialize_row(value, leaf_node_value(node, c->cell_num));
}

void print_constants(void) {
  printf("ROW_SIZE: %d\n", ROW_SIZE);
  printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
  printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
  printf("LEAF_NODE_CELL_SIZE: %d\n", LEAF_NODE_CELL_SIZE);
  printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
  printf("LEAF_NODE_MAX_CELLS: %d\n", LEAF_NODE_MAX_CELLS);
}

void print_leaf_node(void *node) {
  uint32_t num_cells = *leaf_node_num_cells(node);
  printf("leaf (size %d)\n", num_cells);
  for (uint32_t i=0; i<num_cells; i++) {
    uint32_t key = *leaf_node_key(node, i);
    printf("  - %d : %d\n", i, key);
  }
}
