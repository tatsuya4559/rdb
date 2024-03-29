#pragma once

#include "storage.h"
#include "util.h"

typedef enum {
  STATEMENT_INSERT,
  STATEMENT_SELECT,
} StatementType;

typedef struct {
  StatementType type;
  Row row_to_insert; // only used in insert statement
} Statement;

typedef enum {
  PREPARE_SUCCESS,
  PREPARE_UNRECOGNIZED_STATEMENT,
  PREPARE_SYNTAX_ERROR,
  PREPARE_STRING_TOO_LONG,
  PREPARE_NEGATIVE_ID,
} PrepareResult;

PrepareResult prepare_statement(InputBuffer *b, Statement *stmt);
