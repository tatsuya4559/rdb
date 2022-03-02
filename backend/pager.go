package backend

import (
	"errors"
	"io"
	"log"
	"os"
)

type pager struct {
	file       *os.File
	fileLength int64
	numPages   int
	pages      [tableMaxPages][]byte
}

func openPager(filename string) *pager {
	file, err := os.OpenFile(filename, os.O_RDWR|os.O_CREATE, 0600)
	if err != nil {
		log.Fatalf("Unable to open file.")
	}

	fileinfo, err := file.Stat()
	if err != nil {
		log.Fatalf("Unable to get file info.")
	}

	p := &pager{
		file:       file,
		fileLength: fileinfo.Size(),
	}
	p.numPages = int(p.fileLength / pageSize)

	if p.fileLength % pageSize != 0 {
		log.Fatalf("DB file is not a whole number of pages.")
	}

	return p
}

func (p *pager) close() error {
	return p.file.Close()
}

func (p *pager) flush(pageNum int64) {
	if p.pages[pageNum] == nil {
		// Tried to flush null page
		return
	}
	if _, err := p.file.Seek(pageNum*pageSize, os.SEEK_SET); err != nil {
		log.Fatalf("Error seeking: %v", err)
	}

	if _, err := p.file.Write(p.pages[pageNum][:pageSize]); err != nil {
		log.Fatalf("Error writing: %v", err)
	}
}

func (p *pager) getPage(pageNum int) []byte {
	page := p.pages[pageNum]
	// Cache miss.
	if page == nil {
		// allocate memory
		page = make([]byte, pageSize)

		// load from file
		numPages := int(p.fileLength / pageSize)
		if p.fileLength%pageSize != 0 {
			numPages++
		}
		if pageNum <= numPages {
			p.file.Seek(int64(pageNum)*pageSize, os.SEEK_SET)
			if _, err := p.file.Read(page); err != nil && !errors.Is(err, io.EOF) {
				log.Fatalf("Error reading file: %v", err)
			}
		}

		p.pages[pageNum] = page
		if (pageNum >= p.numPages) {
			p.numPages = pageNum + 1
		}
	}

	return page
}
