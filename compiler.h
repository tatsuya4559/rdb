#ifndef _COMPILER_H_
#define _COMPILER_H_

#include "backend.h"
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
} PrepareResult;

PrepareResult prepare_statement(InputBuffer *b, Statement *stmt);

#endif /* _COMPILER_H_ */
