package backend

import (
	"encoding/binary"
	"fmt"
	"log"
)

func assert(cond bool) {
	if !cond {
		panic("assertion error")
	}
}

type buffer struct {
	buf []byte
}

func (b *buffer) Write(p []byte) (n int, err error) {
	// assert(len(p) == len(b.buf))
	copy(b.buf, p)
	return len(p), nil
}
func (b *buffer) Read(p []byte) (n int, err error) {
	// assert(len(p) == len(b.buf))
	copy(p, b.buf)
	return len(b.buf), nil
}

type Row struct {
	ID       uint32
	Username [COLUMN_USERNAME_SIZE]byte
	Email    [COLUMN_EMAIL_SIZE]byte
}

func (r *Row) String() string {
	return fmt.Sprintf("(%d, %s, %s)",
		r.ID,
		cstring(r.Username[:]),
		cstring(r.Email[:]))
}

func (r *Row) SetUsername(username string) {
	if len(username) > COLUMN_USERNAME_SIZE {
		panic("username over")
	}
	copy(r.Username[:], []byte(username))
}

func (r *Row) SetEmail(email string) {
	if len(email) > COLUMN_EMAIL_SIZE {
		panic("email over")
	}
	copy(r.Email[:], []byte(email))
}

func SerializeRow(row *Row, p []byte) {
	log.Printf("row to insert is %v, size = %d\n", row, binary.Size(row))
	log.Printf("slot bytes is %v, size = %d\n", p, len(p))
	buf := &buffer{p}
	binary.Write(buf, binary.BigEndian, row)
	log.Printf("written bytes: %v\n", p)
}

func DeserializeRow(p []byte, row *Row) {
	buf := &buffer{p}
	binary.Read(buf, binary.BigEndian, row)
}

func GetRowSlot(t *Table, rowNum int) []byte {
	pageNum := rowNum / ROWS_PER_PAGE
	p := t.GetPage(pageNum)
	rowOffset := rowNum % ROWS_PER_PAGE
	byteOffset := rowOffset * ROW_SIZE
	return p[byteOffset : byteOffset+ROW_SIZE]
}
