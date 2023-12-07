#ifndef _VM_H_
#define _VM_H_

#include "storage.h"
#include "compiler.h"

typedef enum {
  EXECUTE_SUCCESS,
  EXECUTE_TABLE_FULL,
} ExecuteResult;

ExecuteResult execute_statement(Statement *stmt, Table *table);

#endif /* _VM_H_ */
