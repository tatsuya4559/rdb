#include <stdio.h>
#include "vm.h"

void execute_statement(Statement *stmt) {
  switch (stmt->type) {
  case STATEMENT_INSERT:
    printf("This is where we would do an insert.\n");
    break;
  case (STATEMENT_SELECT):
    printf("This is where we would do a select.\n");
    break;
  }
}
