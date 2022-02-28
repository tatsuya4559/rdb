package backend

type Cursor struct {
	table      *Table
	rowNum     int
	EndOfTable bool
}

func TableStart(t *Table) *Cursor {
	return &Cursor{
		table:      t,
		rowNum:     0,
		EndOfTable: t.NumRows == 0,
	}
}

func TableEnd(t *Table) *Cursor {
	return &Cursor{
		table:      t,
		rowNum:     t.NumRows,
		EndOfTable: true,
	}
}

func (c *Cursor) GetValue() []byte {
	pageNum := c.rowNum / rowsPerPage
	p := c.table.GetPage(pageNum)
	rowOffset := c.rowNum % rowsPerPage
	byteOffset := rowOffset * rowSize
	return p[byteOffset : byteOffset+rowSize]
}

func (c *Cursor) Advance() {
	c.rowNum++
	if c.rowNum >= c.table.NumRows {
		c.EndOfTable = true
	}
}
