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
		if t.pager.pages[i] == nil {
			continue
		}
		t.pager.flush(i, pageSize)
	}

	numAdditionalRows := t.NumRows % rowsPerPage
	if numAdditionalRows > 0 {
		pageNum := numFullPages
		if t.pager.pages[pageNum] != nil {
			t.pager.flush(pageNum, numAdditionalRows*rowSize)
		}
	}

	if err := t.pager.file.Close(); err != nil {
		log.Fatalf("Error closing db file.")
	}
}

func (t *Table) GetPage(pageNum int) []byte {
	if pageNum > tableMaxPages {
		log.Fatalf("Tried to fetch page number out of bounds. %d > %d", pageNum, tableMaxPages)
	}
	return t.pager.getPage(pageNum)
}

func (t *Table) GetRowSlot(rowNum int) []byte {
	pageNum := rowNum / rowsPerPage
	p := t.GetPage(pageNum)
	rowOffset := rowNum % rowsPerPage
	byteOffset := rowOffset * rowSize
	return p[byteOffset : byteOffset+rowSize]
}
