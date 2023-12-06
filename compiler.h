#ifndef _COMPILER_H_
#define _COMPILER_H_

#include "util.h"

typedef enum {
  STATEMENT_INSERT,
  STATEMENT_SELECT,
} StatementType;

typedef struct {
  StatementType type;
} Statement;

typedef enum {
  PREPARE_SUCCESS,
  PREPARE_UNRECOGNIZED_STATEMENT,
} PrepareResult;

PrepareResult prepare_statement(InputBuffer *b, Statement *stmt);

#endif /* _COMPILER_H_ */
