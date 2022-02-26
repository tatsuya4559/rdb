rdb: src/main.c
	gcc -Wall -o rdb src/main.c

test: rdb
	cd test; go test main_test.go

.PHONY: test
