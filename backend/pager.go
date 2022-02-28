package backend

import (
	"log"
	"os"
)

// TODO: use new type
type Page = []byte

type Pager struct {
	file       *os.File
	fileLength int64
	Pages      [TABLE_MAX_PAGES]Page
}

func openPager(filename string) *Pager {
	file, err := os.Create(filename)
	if err != nil {
		log.Fatalf("Unable to open file.")
	}

	fileinfo, err := file.Stat()
	if err != nil {
		log.Fatalf("Unable to get file info.")
	}

	p := &Pager{
		file:       file,
		fileLength: fileinfo.Size(),
	}

	return p
}

func (p *Pager) flush(pageNum, size int) {
	log.Printf("flush page %d: %v\n", pageNum, p.Pages[pageNum])
	if p.Pages[pageNum] == nil {
		log.Fatalf("Tried to flush null page.")
	}
	if _, err := p.file.Seek(int64(pageNum*PAGE_SIZE), os.SEEK_SET); err != nil {
		log.Fatalf("Error seeking: %v", err)
	}

	if _, err := p.file.Write(p.Pages[pageNum]); err != nil {
		log.Fatalf("Error writing: %v", err)
	}
}
