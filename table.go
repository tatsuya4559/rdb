package main

const PAGE_SIZE = 4096
const TABLE_MAX_PAGES = 100

var ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE
var TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES

type Table struct {
	numRows int
	pages   [TABLE_MAX_PAGES][]byte
}

func NewTable() *Table {
	var table Table
	for i := 0; i < TABLE_MAX_PAGES; i++ {
		table.pages[i] = make([]byte, PAGE_SIZE)
	}
	return &table
}

func (t *Table) GetRowSlot(rowNum int) []byte {
	pageNum := rowNum / ROWS_PER_PAGE
	p := t.pages[pageNum]
	rowOffset := rowNum % ROWS_PER_PAGE
	byteOffset := rowOffset * ROW_SIZE
	return p[byteOffset : byteOffset+ROW_SIZE]
}
