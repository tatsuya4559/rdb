#pragma once

#include "storage.h"
#include "query.h"
#include "util.h"

typedef enum {
  EXECUTE_SUCCESS,
  EXECUTE_TABLE_FULL,
  EXECUTE_DUPLICATE_KEY,
} ExecuteResult;

ExecuteResult execute_statement(Statement *stmt, Table *table);

typedef enum {
  META_COMMAND_SUCCESS,
  META_COMMAND_UNRECOGNIZED_COMMAND,
} MetaCommandResult;

MetaCommandResult do_meta_command(InputBuffer *b, Table *table);
