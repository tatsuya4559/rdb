package main

import (
	"errors"
	"os"
)

var (
	ErrUnrecognizedMetaCommand = errors.New("Unrecognized meta commad")
)

func doMetaCommand(cmd string) error {
	if cmd == ".exit" {
		os.Exit(0)
	}
	return ErrUnrecognizedMetaCommand
}
