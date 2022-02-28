package backend

import (
	"encoding/binary"
	"errors"
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

var (
	ErrRowNegativeID    = errors.New("ID must be positive.")
	ErrRowStringTooLong = errors.New("String is too long.")
)

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

func (r *Row) SetID(id int) error {
	if id < 0 {
		return ErrRowNegativeID
	}
	r.ID = uint32(id)
	return nil
}

func (r *Row) SetUsername(username string) error {
	if len(username) > ColumnUsernameSize {
		return ErrRowStringTooLong
	}
	copy(r.Username[:], []byte(username))
	return nil
}

func (r *Row) SetEmail(email string) error {
	if len(email) > ColumnEmailSize {
		return ErrRowStringTooLong
	}
	copy(r.Email[:], []byte(email))
	return nil
}

func SerializeRow(row *Row, p []byte) {
	buf := &buffer{p}
	binary.Write(buf, binary.BigEndian, row)
}

func DeserializeRow(p []byte, row *Row) {
	buf := &buffer{p}
	binary.Read(buf, binary.BigEndian, row)
}
