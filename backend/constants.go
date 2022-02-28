package backend

import "unsafe"

const (
	ColumnUsernameSize = 32
	ColumnEmailSize    = 255
)

const (
	pageSize      = 4096
	tableMaxPages = 100
	rowSize       = int(unsafe.Sizeof(Row{}))
	rowsPerPage   = pageSize / rowSize
	TableMaxRows  = rowsPerPage * tableMaxPages
)
