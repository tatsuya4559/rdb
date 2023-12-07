#include <stdio.h>
#include <string.h>
#include "query.h"
#include "storage.h"

static PrepareResult prepare_insert(InputBuffer *b, Statement *stmt) {
  stmt->type = STATEMENT_INSERT;

  strtok(b->buf, " "); // discard `insert`
  char *id_string = strtok(NULL, " ");
  char *username = strtok(NULL, " ");
  char *email = strtok(NULL, " ");

  if (id_string == NULL || username == NULL || email == NULL) {
    return PREPARE_SYNTAX_ERROR;
  }

  char *endptr;
  long id = strtol(id_string, &endptr, 10);
  if (*endptr != '\0') {
    return PREPARE_SYNTAX_ERROR;
  }
  if (id < 0) {
    return PREPARE_NEGATIVE_ID;
  }
  if (strlen(username) > COLUMN_USERNAME_SIZE) {
    return PREPARE_STRING_TOO_LONG;
  }
  if (strlen(email) > COLUMN_EMAIL_SIZE) {
    return PREPARE_STRING_TOO_LONG;
  }

  stmt->row_to_insert.id = (uint32_t)id;
  strcpy(stmt->row_to_insert.username, username);
  strcpy(stmt->row_to_insert.email, email);

  return PREPARE_SUCCESS;
}

PrepareResult prepare_statement(InputBuffer *b, Statement *stmt) {
  if (strncmp(b->buf, "insert", 6) == 0) {
    return prepare_insert(b, stmt);
  }
  if (strncmp(b->buf, "select", 6) == 0) {
    stmt->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }
  return PREPARE_UNRECOGNIZED_STATEMENT;
}

