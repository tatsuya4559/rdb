CC := gcc
CFLAGS := -Wall -g

SRCS := main.c
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
