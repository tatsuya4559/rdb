.DEFAULT_GOAL := all

CC := gcc
# CFLAGS := -Wall
# for debug
CFLAGS := -Wall -g -DDEBUG

SRCS := main.c engine.c query.c storage.c util.c
OBJS := $(patsubst %.c,%.o,$(SRCS))
DEPENDS := $(patsubst %.c,%.d,$(SRCS))

BIN := db

-include $(DEPENDS)

all: $(DEPENDS) $(BIN) ## Build all

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) -c $(CFLAGS) $<

%.d: %.c
	$(CC) -MM $< > $@

.PHONY: clean
clean: ## Clean artifacts
	@rm -f $(BIN) $(OBJS) $(DEPENDS)

.PHONY: test
test: ## Run all tests
	python -m unittest

.PHONY: run
run: $(BIN)
	./db mydb.db

.PHONY: help
help: ## Display this help
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'
