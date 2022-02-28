package main

import (
	"bufio"
	"encoding/binary"
	"errors"
	"fmt"
	"log"
	"os"
	"strconv"
	"strings"

	"github.com/tatsuya4559/build-my-own-x/rdb/backend"
)

func main() {
	if len(os.Args) < 2 {
		log.Fatalf("Must supply a database filename.")
	}
	filename := os.Args[1]
	table := backend.OpenDB(filename)

	scanner := bufio.NewScanner(os.Stdin)
	for {
		fmt.Print("db> ")
		if !scanner.Scan() {
			os.Exit(0)
		}
		line := scanner.Text()

		if line == ".debug" {
			fmt.Println(binary.Size(backend.Row{}))
			fmt.Println(table.Pager.Pages[0])
			var row backend.Row
			backend.DeserializeRow(table.Pager.Pages[0][:backend.ROW_SIZE], &row)
			fmt.Println(row.String())
			fmt.Println("TABLE_MAX_ROWS: ", backend.TABLE_MAX_ROWS)
			continue
		}
		if strings.HasPrefix(line, ".") {
			if err := doMetaCommand(line, table); err != nil {
				fmt.Println(err)
			}
			continue
		}
		stmt, err := prepareStatement(line)
		if err != nil {
			fmt.Println(err)
			continue
		}
		if err := executeStatement(stmt, table); err != nil {
			fmt.Println(err)
			continue
		}
		fmt.Println("Executed.")
	}
}

type StatementType int

const (
	StatementTypeInsert StatementType = iota + 1
	StatementTypeSelect
)

type Statement struct {
	Type        StatementType
	rowToInsert *backend.Row
}

var (
	ErrPrepareUnrecognizedStatement = errors.New("Unrecognized statement")
	ErrPrepareSyntax                = errors.New("Syntax error.")
	ErrPrepareStringTooLong         = errors.New("String is too long.")
	ErrPrepareNegativeID            = errors.New("ID must be positive.")
)

func prepareInsert(line string) (*Statement, error) {
	var stmt Statement
	stmt.Type = StatementTypeInsert

	l := newLexer(line)
	// keyword `insert`
	if _, err := l.nextToken(); err != nil {
		return nil, ErrPrepareSyntax
	}
	idString, err := l.nextToken()
	if err != nil {
		return nil, ErrPrepareSyntax
	}
	username, err := l.nextToken()
	if err != nil {
		return nil, ErrPrepareSyntax
	}
	email, err := l.nextToken()
	if err != nil {
		return nil, ErrPrepareSyntax
	}

	id, err := strconv.Atoi(idString)
	if err != nil {
		return nil, ErrPrepareSyntax
	}
	if id < 0 {
		return nil, ErrPrepareNegativeID
	}
	if len(username) > backend.COLUMN_USERNAME_SIZE {
		return nil, ErrPrepareStringTooLong
	}
	if len(email) > backend.COLUMN_EMAIL_SIZE {
		return nil, ErrPrepareStringTooLong
	}

	var row backend.Row
	row.ID = uint32(id)
	row.SetUsername(username)
	row.SetEmail(email)

	stmt.rowToInsert = &row

	return &stmt, nil
}

func prepareStatement(line string) (*Statement, error) {
	if strings.HasPrefix(line, "insert") {
		return prepareInsert(line)
	}
	if strings.HasPrefix(line, "select") {
		var stmt Statement
		stmt.Type = StatementTypeSelect
		return &stmt, nil
	}
	return nil, ErrPrepareUnrecognizedStatement
}

var (
	ErrExecuteTableFull = errors.New("Error: Table full.")
)

func executeInsert(stmt *Statement, table *backend.Table) error {
	if table.NumRows >= backend.TABLE_MAX_ROWS {
		return ErrExecuteTableFull
	}

	backend.SerializeRow(stmt.rowToInsert, backend.GetRowSlot(table, table.NumRows))
	table.NumRows++
	return nil
}

func executeSelect(stmt *Statement, table *backend.Table) error {
	var row backend.Row
	for i := 0; i < table.NumRows; i++ {
		backend.DeserializeRow(backend.GetRowSlot(table, i), &row)
		fmt.Println(row.String())
	}
	return nil
}

func executeStatement(stmt *Statement, table *backend.Table) error {
	switch stmt.Type {
	case StatementTypeInsert:
		return executeInsert(stmt, table)
	case StatementTypeSelect:
		return executeSelect(stmt, table)
	}
	return nil
}
