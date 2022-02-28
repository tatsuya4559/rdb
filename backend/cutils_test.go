package backend

import "testing"

func TestCString(t *testing.T) {
	tests := []struct {
		input []byte
		want  string
	}{
		{[]byte{}, ""},
		{[]byte{0}, ""},
		{[]byte{97, 0}, "a"},
		{[]byte{97, 97, 0}, "aa"},
		{[]byte{97, 97}, "aa"},
	}

	for _, tt := range tests {
		got := cstring(tt.input)
		if got != tt.want {
			t.Errorf("want %s, but got %s", tt.want, got)
		}
	}
}
