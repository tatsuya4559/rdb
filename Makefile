.DEFAULT_GOAL := all

CC := gcc
CFLAGS := -Wall -g

SRCS := main.c vm.c compiler.c backend.c util.c
OBJS := $(patsubst %.c,%.o,$(SRCS))

BIN := db

all: $(BIN) ## Build all

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $^

.PHONY: clean
clean: ## Clean artifacts
	@rm -f $(BIN) $(OBJS)

.PHONY: test
test: ## Run all tests
	python -m unittest

.PHONY: help
help: ## Display this help
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'
