package backend

import (
	"log"
)

type Table struct {
	NumRows int
	pager   *pager
}

func OpenDB(filename string) *Table {
	pager := openPager(filename)
	return &Table{
		NumRows: int(pager.fileLength) / rowSize,
		pager:   pager,
	}
}

func (t *Table) Close() {
	numFullPages := t.NumRows / rowsPerPage

	for i := 0; i < numFullPages; i++ {
		t.pager.flush(int64(i), pageSize)
	}

	numAdditionalRows := t.NumRows % rowsPerPage
	if numAdditionalRows > 0 {
		pageNum := numFullPages
		t.pager.flush(int64(pageNum), numAdditionalRows*rowSize)
	}

	if err := t.pager.close(); err != nil {
		log.Fatalf("Error closing db file.")
	}
}

func (t *Table) IsFull() bool {
	return t.NumRows >= TableMaxRows
}

func (t *Table) GetPage(pageNum int) []byte {
	if pageNum > tableMaxPages {
		log.Fatalf("Tried to fetch page number out of bounds. %d > %d", pageNum, tableMaxPages)
	}
	return t.pager.getPage(int64(pageNum))
}
