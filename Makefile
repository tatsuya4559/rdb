CC := gcc
CFLAGS := -Wall -g

SRCS := main.c vm.c compiler.c backend.c util.c
OBJS := $(patsubst %.c,%.o,$(SRCS))

BIN := db

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	@rm -f $(BIN) $(OBJS)

.PHONY: test
test:
	./test.sh
