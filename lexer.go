package main

import (
	"bufio"
	"errors"
	"strings"
)

var EOF = errors.New("EOF")

type lexer struct {
	scanner *bufio.Scanner
	src     string
}

func newLexer(line string) *lexer {
	s := bufio.NewScanner(strings.NewReader(line))
	s.Split(bufio.ScanWords)

	return &lexer{
		scanner: s,
		src:     line,
	}
}

func (l *lexer) nextToken() (string, error) {
	if !l.scanner.Scan() {
		return "", EOF
	}
	return l.scanner.Text(), nil
}
