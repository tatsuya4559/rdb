rdb:
	go build -o rdb

test:
	go test -v ./...

.PHONY: rdb test
