package backend

const (
	PAGE_SIZE       = 4096
	TABLE_MAX_PAGES = 100
)

type Table struct {
	NumRows int
	Pages   [TABLE_MAX_PAGES][]byte
}

func NewTable() *Table {
	var table Table
	for i := 0; i < TABLE_MAX_PAGES; i++ {
		table.Pages[i] = make([]byte, PAGE_SIZE)
	}
	return &table
}
