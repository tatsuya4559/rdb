rdb: main.c
	gcc -Wall -o rdb main.c

test: rdb
	go test main_test.go

.PHONY: test
