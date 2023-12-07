#pragma once

#include "storage.h"
#include "query.h"

typedef enum {
  EXECUTE_SUCCESS,
  EXECUTE_TABLE_FULL,
} ExecuteResult;

ExecuteResult execute_statement(Statement *stmt, Table *table);
