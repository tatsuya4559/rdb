package backend

import (
	"encoding/binary"
	"fmt"
)

type buffer struct {
	buf []byte
}

func (b *buffer) Write(p []byte) (n int, err error) {
	copy(b.buf, p)
	return len(p), nil
}

func (b *buffer) Read(p []byte) (n int, err error) {
	copy(p, b.buf)
	return len(b.buf), nil
}

type Row struct {
	ID       uint32
	Username [ColumnUsernameSize]byte
	Email    [ColumnEmailSize]byte
}

func (r *Row) String() string {
	return fmt.Sprintf("(%d, %s, %s)",
		r.ID,
		cstring(r.Username[:]),
		cstring(r.Email[:]))
}

func (r *Row) SetUsername(username string) {
	if len(username) > ColumnUsernameSize {
		panic("username over")
	}
	copy(r.Username[:], []byte(username))
}

func (r *Row) SetEmail(email string) {
	if len(email) > ColumnEmailSize {
		panic("email over")
	}
	copy(r.Email[:], []byte(email))
}

func SerializeRow(row *Row, p []byte) {
	buf := &buffer{p}
	binary.Write(buf, binary.BigEndian, row)
}

func DeserializeRow(p []byte, row *Row) {
	buf := &buffer{p}
	binary.Read(buf, binary.BigEndian, row)
}
