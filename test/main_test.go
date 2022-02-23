package main

import (
	"fmt"
	"io"
	"os"
	"os/exec"
	"strings"
	"testing"

	"github.com/google/go-cmp/cmp"
)

func TestInsertAndRetrieveRow(t *testing.T) {
	maxUsername := strings.Repeat("u", 32)
	maxEmail := strings.Repeat("e", 255)

	tests := map[string]struct {
		inputs []string
		want   []string
	}{
		"insert and retrieve": {
			[]string{
				"insert 1 foo foo@example.com",
				"select *",
				".exit",
			},
			[]string{
				"db> Executed.",
				"db> (1, foo, foo@example.com)",
				"Executed.",
				"db> ",
			},
		},
		"max varchar length": {
			[]string{
				fmt.Sprintf("insert 1 %s %s", maxUsername, maxEmail),
				"select *",
				".exit",
			},
			[]string{
				"db> Executed.",
				fmt.Sprintf("db> (1, %s, %s)", maxUsername, maxEmail),
				"Executed.",
				"db> ",
			},
		},
		"over varchar length": {
			[]string{
				fmt.Sprintf("insert 1 %s %s", maxUsername+"u", maxEmail+"e"),
				"select *",
				".exit",
			},
			[]string{
				"db> String is too long.",
				"db> Executed.",
				"db> ",
			},
		},
		"id is negative": {
			[]string{
				"insert -1 foo foo@example.com",
				"select *",
				".exit",
			},
			[]string{
				"db> ID must be positive.",
				"db> Executed.",
				"db> ",
			},
		},
		"print contants": {
			[]string{
				".constants",
				".exit",
			},
			[]string{
				"db> Constants:",
				"ROW_SIZE: 293",
				"COMMON_NODE_HEADER_SIZE: 6",
				"LEAF_NODE_HEADER_SIZE: 10",
				"LEAF_NODE_CELL_SIZE: 297",
				"LEAF_NODE_SPACE_FOR_CELLS: 4086",
				"LEAF_NODE_MAX_CELLS: 13",
				"db> ",
			},
		},
		"inserted in sorted order": {
			[]string{
				"insert 3 user3 user3@example.com",
				"insert 1 user1 user1@example.com",
				"insert 2 user2 user2@example.com",
				".btree",
				".exit",
			},
			[]string{
				"db> Executed.",
				"db> Executed.",
				"db> Executed.",
				"db> Tree:",
				"leaf (size 3)",
				"  - 0 : 1",
				"  - 1 : 2",
				"  - 2 : 3",
				"db> ",
			},
		},
		"duplicate id error": {
			[]string{
				"insert 1 user1 user1@example.com",
				"insert 1 user2 user2@example.com",
				"select",
				".exit",
			},
			[]string{
				"db> Executed.",
				"db> Error: Duplicate key.",
				"db> (1, user1, user1@example.com)",
				"Executed.",
				"db> ",
			},
		},
	}

	for name, tt := range tests {
		got := runRDB(t, tt.inputs)
		if diff := cmp.Diff(got, tt.want); diff != "" {
			t.Errorf("failed in case %q (-got +want): %v", name, diff)
		}
		cleanDBFile(t)
	}
}

func TestTableIsFull(t *testing.T) {
	var inputs []string
	for i := 0; i < 1400; i++ {
		inputs = append(inputs, fmt.Sprintf("insert %[1]d user%[1]d user%[1]d@example.com", i))
	}
	inputs = append(inputs, ".exit")

	got := runRDB(t, inputs)

	want := "db> Error: Table full."
	if got[len(got)-2] != want {
		t.Errorf("want %q, but got %q", want, got[len(got)-2])
	}
	cleanDBFile(t)
}

func cleanDBFile(t *testing.T) {
	t.Helper()
	if err := os.Remove("test.db"); err != nil {
		t.Fatalf("failed to remove test.db: %v", err)
	}
}

func runRDB(t *testing.T, inputs []string) []string {
	t.Helper()

	cmd := exec.Command("../rdb", "test.db")
	stdinPipe, _ := cmd.StdinPipe()

	go func() {
		defer stdinPipe.Close()

		for _, input := range inputs {
			io.WriteString(stdinPipe, input+"\n")
		}
	}()

	got, err := cmd.CombinedOutput()
	if err != nil {
		t.Fatalf("error occurred at running command: %v", err)
	}
	return strings.Split(string(got), "\n")
}
