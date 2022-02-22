#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* I/O Utility */
#define die(...) do { fprintf(stderr, __VA_ARGS__); exit(EXIT_FAILURE); } while(0)

typedef struct {
  char* buffer;
  size_t buffer_length;
  ssize_t input_length;
} InputBuffer;

InputBuffer* new_input_buffer() {
  InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
  input_buffer->buffer = NULL;
  input_buffer->buffer_length = 0;
  input_buffer->input_length = 0;
  return input_buffer;
}

void print_prompt() {
  printf("db> ");
}

ssize_t getline(char **lineptr, size_t *n, FILE *stream);

void read_input(InputBuffer* input_buffer) {
  ssize_t bytes_read = getline(
      &(input_buffer->buffer),
      &(input_buffer->buffer_length),
      stdin);

  if (bytes_read <= 0) {
    die("Error reading input\n");
  }

  // ignore trailing newline
  input_buffer->input_length = bytes_read - 1;
  input_buffer->buffer[bytes_read - 1] = 0;
}

void close_input_buffer(InputBuffer* input_buffer) {
  free(input_buffer->buffer);
  free(input_buffer);
}

/* Row */
#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
typedef struct {
  uint32_t id;
  char username[COLUMN_USERNAME_SIZE+1]; // +1 for null char
  char email[COLUMN_EMAIL_SIZE+1];
} Row;

void print_row(Row* row) {
  printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)
const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

void serialize_row(Row* src, void* dest) {
  memcpy(dest + ID_OFFSET, &(src->id), ID_SIZE);
  strncpy(dest + USERNAME_OFFSET, src->username, USERNAME_SIZE);
  strncpy(dest + EMAIL_OFFSET, src->email, EMAIL_SIZE);
}

void deserialize_row(void* src, Row* dest) {
  memcpy(&(dest->id), src + ID_OFFSET, ID_SIZE);
  memcpy(&(dest->username), src + USERNAME_OFFSET, USERNAME_SIZE);
  memcpy(&(dest->email), src + EMAIL_OFFSET, EMAIL_SIZE);
}

/* Pager */
const uint32_t PAGE_SIZE = 4096;
#define TABLE_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct {
  int fd; // file descriptor
  uint32_t file_length;
  void* pages[TABLE_MAX_PAGES];
} Pager;

Pager* pager_open(const char* filename) {
  int fd = open(filename, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
  if (fd == -1) {
    die("Unable to open file\n");
  }

  Pager* pager = malloc(sizeof(Pager));
  pager->fd = fd;
  pager->file_length = lseek(fd, 0, SEEK_END);
  for (uint32_t i=0; i<TABLE_MAX_PAGES; i++) {
    pager->pages[i] = NULL;
  }

  return pager;
}

void pager_flush(Pager* pager, uint32_t page_num, uint32_t size) {
  if (pager->pages[page_num] == NULL) {
    die("Tried to flush null page\n");
  }

  if (lseek(pager->fd, page_num * PAGE_SIZE, SEEK_SET) == -1) {
    die("Error seeking: %d\n", errno);
  }

  if (write(pager->fd, pager->pages[page_num], size) == -1) {
    die("Error writing: %d\n", errno);
  }
}

void* get_page(Pager* pager, uint32_t page_num) {
  if (page_num > TABLE_MAX_PAGES) {
    die("Tried to fetch page number out of bounds. %d > %d\n",
        page_num, TABLE_MAX_PAGES);
  }

  if (pager->pages[page_num] == NULL) {
    // cache miss. allocate memory and load from file.
    void* page = malloc(PAGE_SIZE);
    uint32_t num_pages = pager->file_length % PAGE_SIZE == 0 ? // ceil
      pager->file_length / PAGE_SIZE :
      (pager->file_length / PAGE_SIZE) + 1;

    if (page_num <= num_pages) {
      lseek(pager->fd, page_num * PAGE_SIZE, SEEK_SET);
      if (read(pager->fd, page, PAGE_SIZE) == -1) {
        die("error reading file: %d\n", errno);
      }
    }

    pager->pages[page_num] = page;
  }
  return pager->pages[page_num];
}

/* Table */
typedef struct {
  uint32_t num_rows;
  Pager* pager;
} Table;

Table* db_open(const char* filename) {
  Pager* pager = pager_open(filename);
  uint32_t num_rows = pager->file_length / ROW_SIZE;

  Table* table = malloc(sizeof(Table));
  table->pager = pager;
  table->num_rows = num_rows;

  return table;
}

void db_close(Table* table) {
  Pager* pager = table->pager;
  uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;

  for (uint32_t i=0; i<num_full_pages; i++) {
    if (pager->pages[i] == NULL) {
      continue;
    }
    pager_flush(pager, i, PAGE_SIZE);
    free(pager->pages[i]);
    pager->pages[i] = NULL;
  }

  uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
  if (num_additional_rows > 0) {
    uint32_t page_num = num_full_pages;
    if (pager->pages[page_num] != NULL) {
      pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);
      free(pager->pages[page_num]);
      pager->pages[page_num] = NULL;
    }
  }

  if (close(pager->fd) == -1) {
    die("Error closing db file.\n");
  }
  for (uint32_t i=0; i<TABLE_MAX_PAGES; i++) {
    void* page = pager->pages[i];
    if (page) {
      free(page);
      pager->pages[i] = NULL;
    }
  }

  free(pager);
  free(table);
}

typedef struct {
  Table* table;
  uint32_t row_num;
  bool end_of_table;
} Cursor;

Cursor* table_start(Table* table) {
  Cursor* cursor = malloc(sizeof(Cursor));
  cursor->table = table;
  cursor->row_num = 0;
  cursor->end_of_table = (table->num_rows == 0);
  return cursor;
}

Cursor* table_end(Table* table) {
  Cursor* cursor = malloc(sizeof(Cursor));
  cursor->table = table;
  cursor->row_num = table->num_rows;
  cursor->end_of_table = true;
  return cursor;
}

void* cursor_value(Cursor* cursor) {
  uint32_t row_num = cursor->row_num;
  uint32_t page_num = row_num / ROWS_PER_PAGE;
  void* page = get_page(cursor->table->pager, page_num);
  uint32_t row_offset = row_num % ROWS_PER_PAGE;
  uint32_t byte_offset = row_offset * ROW_SIZE;
  return page + byte_offset;
}

void cursor_advance(Cursor* cursor) {
  cursor->row_num++;
  if (cursor->row_num >= cursor->table->num_rows) {
    cursor->end_of_table = true;
  }
}

/* Statement */
typedef enum {
  STATEMENT_INSERT,
  STATEMENT_SELECT,
} StatementType;

typedef struct {
  StatementType type;
  Row row_to_insert;
} Statement;

/* Parse statement */
typedef enum {
  PREPARE_SUCCESS,
  PREPARE_NEGATIVE_ID,
  PREPARE_STRING_TOO_LONG,
  PREPARE_SYNTAX_ERROR,
  PREPARE_UNRECOGNIZED_STATEMENT,
} PrepareResult;

PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* stmt) {
    stmt->type = STATEMENT_INSERT;

    char* keyword = strtok(input_buffer->buffer, " ");
    char* id_string = strtok(NULL, " ");
    char* username = strtok(NULL, " ");
    char* email = strtok(NULL, " ");

    if (id_string == NULL || username == NULL || email == NULL) {
      return PREPARE_SYNTAX_ERROR;
    }

    int id = atoi(id_string);
    if (id < 0) {
      return PREPARE_NEGATIVE_ID;
    }
    if (strlen(username) > COLUMN_USERNAME_SIZE) {
      return PREPARE_STRING_TOO_LONG;
    }
    if (strlen(email) > COLUMN_EMAIL_SIZE) {
      return PREPARE_STRING_TOO_LONG;
    }
    stmt->row_to_insert.id = id;
    strcpy(stmt->row_to_insert.username, username);
    strcpy(stmt->row_to_insert.email, email);

    return PREPARE_SUCCESS;
}

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* stmt) {
  if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
    return prepare_insert(input_buffer, stmt);
  }

  if (strncmp(input_buffer->buffer, "select", 6) == 0) {
    stmt->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }

  return PREPARE_UNRECOGNIZED_STATEMENT;
}

/* Execute statement */
typedef enum {
  EXECUTE_SUCCESS,
  EXECUTE_TABLE_FULL,
} ExecuteResult;

ExecuteResult execute_insert(Statement* stmt, Table* table) {
  if (table->num_rows >= TABLE_MAX_ROWS) {
    return EXECUTE_TABLE_FULL;
  }

  Row* row_to_insert = &(stmt->row_to_insert);
  Cursor* cursor = table_end(table);
  serialize_row(row_to_insert, cursor_value(cursor));
  table->num_rows++;

  free(cursor);

  return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement* stmt, Table* table) {
  Cursor* cursor = table_start(table);
  Row row;
  while(!(cursor->end_of_table)) {
    deserialize_row(cursor_value(cursor), &row);
    print_row(&row);
    cursor_advance(cursor);
  }
  free(cursor);
  return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement* stmt, Table* table) {
  switch (stmt->type) {
  case STATEMENT_INSERT:
    return execute_insert(stmt, table);
  case STATEMENT_SELECT:
    return execute_select(stmt, table);
  }
}

/* Meta command */
typedef enum {
  META_COMMAND_SUCCESS,
  META_COMMAND_UNRECOGNIZED_COMMAND,
} MetaCommandResult;

MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table) {
  if (strcmp(input_buffer->buffer, ".exit") == 0) {
    db_close(table);
    close_input_buffer(input_buffer);
    exit(EXIT_SUCCESS);
  }
  return META_COMMAND_UNRECOGNIZED_COMMAND;
}

/* main */
int main(int argc, char* argv[]) {
  if (argc < 2) {
    die("Must supply a database filename.\n");
  }

  char* filename = argv[1];
  Table* table = db_open(filename);

  InputBuffer* input_buffer = new_input_buffer();

  while (true) {
    print_prompt();
    read_input(input_buffer);

    if (input_buffer->buffer[0] == '.') {
      switch (do_meta_command(input_buffer, table)) {
      case META_COMMAND_SUCCESS:
        continue;
      case META_COMMAND_UNRECOGNIZED_COMMAND:
        printf("Unrecognized command '%s'.\n", input_buffer->buffer);
        continue;
      }
    }

    Statement stmt;
    switch (prepare_statement(input_buffer, &stmt)) {
    case PREPARE_SUCCESS:
      break;
    case PREPARE_NEGATIVE_ID:
      printf("ID must be positive.\n");
      continue;
    case PREPARE_STRING_TOO_LONG:
      printf("String is too long.\n");
      continue;
    case PREPARE_SYNTAX_ERROR:
      printf("Syntax error. Could not parse statement.\n");
      continue;
    case PREPARE_UNRECOGNIZED_STATEMENT:
      printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
      continue;
    }

    switch (execute_statement(&stmt, table)) {
    case EXECUTE_SUCCESS:
      printf("Executed.\n");
      break;
    case EXECUTE_TABLE_FULL:
      printf("Error: Table full.\n");
      break;
    }
  }

  return 0;
}
