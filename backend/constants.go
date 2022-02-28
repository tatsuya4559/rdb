package backend

import "unsafe"

const (
	COLUMN_USERNAME_SIZE = 32
	COLUMN_EMAIL_SIZE    = 255
)

const (
	PAGE_SIZE       = 4096
	TABLE_MAX_PAGES = 100
	ROW_SIZE        = int(unsafe.Sizeof(Row{}))
	ROWS_PER_PAGE   = PAGE_SIZE / ROW_SIZE
	TABLE_MAX_ROWS  = ROWS_PER_PAGE * TABLE_MAX_PAGES
)
