package backend

type Cursor struct {
	table      *Table
	pageNum     int
	cellNum int
	EndOfTable bool
}

func TableStart(t *Table) *Cursor {
	c := Cursor{
		table:      t,
		pageNum: t.RootPageNum,
		cellNum: 0,
	}

	rootNode := leafNode(t.GetPage(t.RootPageNum))
	numCells := rootNode.numCells()
	c.EndOfTable = numCells == 0

	return &c
}

func TableEnd(t *Table) *Cursor {
	c := Cursor{
		table:      t,
		pageNum: t.RootPageNum,
		EndOfTable: true,
	}

	rootNode := leafNode(t.GetPage(t.RootPageNum))
	c.cellNum = int(rootNode.numCells())

	return &c
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
