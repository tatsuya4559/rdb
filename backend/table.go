package backend

import (
	"errors"
	"io"
	"log"
	"os"
)

type Table struct {
	NumRows int
	Pager   *Pager
}

func OpenDB(filename string) *Table {
	pager := openPager(filename)
	return &Table{
		NumRows: int(pager.fileLength) / ROW_SIZE,
		Pager:   pager,
	}
}

func (t *Table) Close() {
	pager := t.Pager
	num_full_pages := t.NumRows / ROWS_PER_PAGE

	for i := 0; i < num_full_pages+1; i++ {
		if pager.Pages[i] == nil {
			continue
		}
		pager.flush(i, PAGE_SIZE)
	}

	if err := pager.file.Close(); err != nil {
		log.Fatalf("Error closing db file.")
	}
}

func (t *Table) GetPage(pageNum int) Page {
	if pageNum > TABLE_MAX_PAGES {
		log.Fatalf("Tried to fetch page number out of bounds. %d > %d", pageNum, TABLE_MAX_PAGES)
	}

	page := t.Pager.Pages[pageNum]
	// Cache miss.
	if page == nil {
		// allocate memory
		page = make([]byte, PAGE_SIZE)

		// load from file
		numPages := t.Pager.fileLength / PAGE_SIZE
		if t.Pager.fileLength%PAGE_SIZE != 0 {
			numPages++
		}
		if int64(pageNum) <= numPages {
			t.Pager.file.Seek(int64(pageNum)*PAGE_SIZE, os.SEEK_SET)
			if _, err := t.Pager.file.Read(page); err != nil && !errors.Is(err, io.EOF) {
				log.Fatalf("Error reading file: %v", err)
			}
		}

		t.Pager.Pages[pageNum] = page
	}

	return page
}
