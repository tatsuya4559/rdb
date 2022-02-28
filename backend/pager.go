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

	return p
}

func (p *pager) flush(pageNum, size int) {
	if p.pages[pageNum] == nil {
		log.Fatalf("Tried to flush null page.")
	}
	if _, err := p.file.Seek(int64(pageNum*pageSize), os.SEEK_SET); err != nil {
		log.Fatalf("Error seeking: %v", err)
	}

	if _, err := p.file.Write(p.pages[pageNum][:size]); err != nil {
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
		numPages := p.fileLength / pageSize
		if p.fileLength%pageSize != 0 {
			numPages++
		}
		if int64(pageNum) <= numPages {
			p.file.Seek(int64(pageNum)*pageSize, os.SEEK_SET)
			if _, err := p.file.Read(page); err != nil && !errors.Is(err, io.EOF) {
				log.Fatalf("Error reading file: %v", err)
			}
		}

		p.pages[pageNum] = page
	}

	return page
}
