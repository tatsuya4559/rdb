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

static bool is_root_node(void *node) {
  return *(bool *)(node + IS_ROOT_OFFSET);
}

static void set_node_root(void *node, bool is_root) {
  *(bool *)(node + IS_ROOT_OFFSET) = is_root;
}

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

static const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
static const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;

static NodeType get_node_type(void *node) {
  return *(NodeType *)(node + NODE_TYPE_OFFSET);
}

static void set_node_type(void *node, NodeType type) {
  *(NodeType *)(node + NODE_TYPE_OFFSET) = type;
}

uint32_t *leaf_node_num_cells(void *node) {
  return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

static void *leaf_node_cell(void *node, uint32_t cell_num) {
  return node + LEAF_NODE_HEADER_SIZE + LEAF_NODE_CELL_SIZE * cell_num;
}

uint32_t *leaf_node_key(void *node, uint32_t cell_num) {
  return leaf_node_cell(node, cell_num);
}

static void *leaf_node_value(void *node, uint32_t cell_num) {
  return leaf_node_cell(node, cell_num) + LEAF_NODE_VALUE_OFFSET;
}

static void initialize_leaf_node(void *node) {
  *leaf_node_num_cells(node) = 0;
  set_node_type(node, NODE_LEAF);
  set_node_root(node, false);
}

/* Internal Node Header Layout */
static const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
static const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
static const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
static const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET =
  INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
static const uint32_t INTERNAL_NODE_HEADER_SIZE =
  COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE;

/* Internal Node Body Layout */
static const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
static const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
static const uint32_t INTERNAL_NODE_CELL_SIZE =
  INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;

static uint32_t *internal_node_num_keys(void *node) {
  return node + INTERNAL_NODE_NUM_KEYS_OFFSET;
}

static uint32_t *internal_node_right_child(void *node) {
  return node + INTERNAL_NODE_RIGHT_CHILD_OFFSET;
}

static uint32_t *internal_node_cell(void *node, uint32_t cell_num) {
  return node + INTERNAL_NODE_HEADER_SIZE + INTERNAL_NODE_CELL_SIZE * cell_num;
}

static uint32_t *internal_node_child(void *node, uint32_t child_num) {
  uint32_t num_keys = *internal_node_num_keys(node);
  if (child_num > num_keys) {
    fprintf(stderr, "Tried to access child_num %d > num_keys %d\n",
        child_num, num_keys);
    exit(EXIT_FAILURE);
  } else if (child_num == num_keys) {
    return internal_node_right_child(node);
  } else {
    return internal_node_cell(node, child_num);
  }
}

static uint32_t *internal_node_key(void *node, uint32_t key_num) {
  return internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE;
}

static void initialize_internal_node(void *node) {
  set_node_type(node, NODE_INTERNAL);
  set_node_root(node, false);
  *internal_node_num_keys(node) = 0;
}

static uint32_t get_node_max_key(void *node) {
  switch (get_node_type(node)) {
  case NODE_INTERNAL:
    return *internal_node_key(node, *internal_node_num_keys(node) - 1);
  case NODE_LEAF:
    return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
  }
  assert(false);
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

uint32_t get_unused_page_num(Pager *p) {
  return p->num_pages;
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
    set_node_root(root_node, true);
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

Cursor *leaf_node_find(Table *table, uint32_t page_num, uint32_t key) {
  void *node = get_page(table->pager, page_num);
  uint32_t num_cells = *leaf_node_num_cells(node);

  Cursor *c = malloc(sizeof(Cursor));
  c->table = table;
  c->page_num = page_num;

  // Binary search
  uint32_t min_idx = 0;
  uint32_t one_past_max_idx = num_cells;
  while (min_idx < one_past_max_idx) {
    uint32_t idx = (min_idx + one_past_max_idx) / 2;
    uint32_t key_at_idx = *leaf_node_key(node, idx);
    if (key == key_at_idx) {
      c->cell_num = idx;
      c->end_of_table = idx == num_cells;
      return c;
    }
    if (key < key_at_idx) {
      one_past_max_idx = idx;
    } else {
      min_idx = idx + 1;
    }
  }

  c->end_of_table = num_cells <= min_idx;
  c->cell_num = min_idx;
  return c;
}

Cursor *internal_node_find(Table *table, uint32_t page_num, uint32_t key) {
  void *node = get_page(table->pager, page_num);
  uint32_t num_keys = *internal_node_num_keys(node);

  // Binary search
  uint32_t min_idx = 0;
  uint32_t max_idx = num_keys;
  printf("key = %d\n", key);
  while (min_idx < max_idx) {
    uint32_t idx = (min_idx + max_idx) / 2;
    uint32_t key_to_right = *internal_node_key(node, idx);
    printf("min=%d idx=%d max=%d\n", min_idx, idx, max_idx);
    printf("key=%d key_to_right=%d\n", key, key_to_right);
    if (key == key_to_right) {
      min_idx = idx;
      break;
    } else if (key < key_to_right) {
      max_idx = idx;
    } else {
      min_idx = idx == min_idx ? idx+1 : idx;
    }
  }

  uint32_t child_num = *internal_node_child(node, min_idx);
  void *child = get_page(table->pager, child_num);
  switch (get_node_type(child)) {
  case NODE_LEAF:
    return leaf_node_find(table, child_num, key);
  case NODE_INTERNAL:
    return internal_node_find(table, child_num, key);
  }
  assert(false);
}

Cursor *table_find(Table *table, uint32_t key) {
  uint32_t root_page_num = table->root_page_num;
  void *root_node = get_page(table->pager, root_page_num);

  if (get_node_type(root_node) == NODE_LEAF) {
    return leaf_node_find(table, root_page_num, key);
  } else {
    return internal_node_find(table, root_page_num, key);
  }
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

static void create_new_root(Table *table, uint32_t right_child_page_num) {
  void *root = get_page(table->pager, table->root_page_num);
  uint32_t left_child_page_num = get_unused_page_num(table->pager);
  void *left_child = get_page(table->pager, left_child_page_num);

  memcpy(left_child, root, PAGE_SIZE);
  set_node_root(left_child, false);

  initialize_internal_node(root);
  set_node_root(root, true);
  *internal_node_num_keys(root) = 1;
  *internal_node_child(root, 0) = left_child_page_num;
  uint32_t left_child_max_key = get_node_max_key(left_child);
  *internal_node_key(root, 0) = left_child_max_key;
  *internal_node_right_child(root) = right_child_page_num;
}

void leaf_node_split_and_insert(Cursor *c, uint32_t key, Row *value) {
  /* Create a new node */
  void *old_node = get_page(c->table->pager, c->page_num);
  uint32_t new_page_num = get_unused_page_num(c->table->pager);
  void *new_node = get_page(c->table->pager, new_page_num);
  initialize_leaf_node(new_node);

  /* copy every cell into its new location */
  for (int32_t i=LEAF_NODE_MAX_CELLS; i>=0; i--) {
    void *dest_node = i >= LEAF_NODE_LEFT_SPLIT_COUNT ?  new_node : old_node;
    uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
    void *dest = leaf_node_cell(dest_node, index_within_node);

    if (i == c->cell_num) {
      serialize_row(value, dest);
    } else if (i > c->cell_num) {
      memcpy(dest, leaf_node_cell(old_node, i-1), LEAF_NODE_CELL_SIZE);
    } else {
      memcpy(dest, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
    }
  }

  *(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
  *(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;

  if (is_root_node(old_node)) {
    create_new_root(c->table, new_page_num);
  } else {
    fprintf(stderr, "Need to implement updating parent after split\n");
    exit(EXIT_FAILURE);
  }
}

void leaf_node_insert(Cursor *c, uint32_t key, Row *value) {
  void *node = get_page(c->table->pager, c->page_num);

  uint32_t num_cells = *leaf_node_num_cells(node);
  if (num_cells >= LEAF_NODE_MAX_CELLS) {
    // Node is full
    leaf_node_split_and_insert(c, key, value);
    return;
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

void print_tree(Pager *p, uint32_t page_num, uint32_t depth) {
  void *node = get_page(p, page_num);
  uint32_t num_keys, child;

  switch (get_node_type(node)) {
  case NODE_LEAF:
    num_keys = *leaf_node_num_cells(node);
    printf("%*s leaf (size %d)\n", depth*2, "-", num_keys);
    for (uint32_t i=0; i<num_keys; i++) {
      printf("%*s %d\n", (depth+1)*2, "-", *leaf_node_key(node, i));
    }
    break;
  case NODE_INTERNAL:
    num_keys = *internal_node_num_keys(node);
    printf("%*s internal (size %d)\n", depth*2, "-", num_keys);
    for (uint32_t i=0; i<num_keys; i++) {
      child = *internal_node_child(node, i);
      print_tree(p, child, depth+1);
      printf("%*s key %d\n", depth*2, "-", *internal_node_key(node, i));
    }
    child = *internal_node_right_child(node);
    print_tree(p, child, depth+1);
    break;
  }
}
