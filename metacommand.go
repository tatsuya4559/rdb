package main

import (
	"errors"
	"os"

	"github.com/tatsuya4559/build-my-own-x/rdb/backend"
)

var (
	ErrUnrecognizedMetaCommand = errors.New("Unrecognized meta commad")
)

func doMetaCommand(cmd string, table *backend.Table) error {
	if cmd == ".exit" {
		table.Close()
		os.Exit(0)
	}
	return ErrUnrecognizedMetaCommand
}
