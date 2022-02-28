package backend

import (
	"testing"

	"github.com/google/go-cmp/cmp"
)

func TestSerializeDeserialize(t *testing.T) {
	var originalRow Row
	originalRow.SetID(100)
	originalRow.SetUsername("Foo")
	originalRow.SetEmail("foo@example.com")

	p := make([]byte, rowSize)
	SerializeRow(&originalRow, p)

	var restoredRow Row
	DeserializeRow(p, &restoredRow)

	if diff := cmp.Diff(originalRow, restoredRow); diff != "" {
		t.Errorf("Serialize failed: %v", diff)
	}
}

func TestSetUsername(t *testing.T) {
	var row Row
	want := "foo"
	row.SetUsername(want)

	if got := cstring(row.Username[:]); got != want {
		t.Errorf("want %s, but got %s", want, got)
	}
}

func TestSetEmail(t *testing.T) {
	var row Row
	want := "foo@example.com"
	row.SetEmail(want)

	if got := cstring(row.Email[:]); got != want {
		t.Errorf("want %s, but got %s", want, got)
	}
}
