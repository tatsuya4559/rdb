package backend

import (
	"bytes"
	"encoding/binary"
	"unsafe"
)

type nodeType uint8
const (
	nodeTypeInternal nodeType = iota + 1
	nodeTypeLeaf
)

// Common node header layout
const nodeTypeSize = unsafe.Sizeof(nodeTypeLeaf)
const nodeTypeOffset = 0
const isRootSize =  unsafe.Sizeof(true)
const isRootOffset = nodeTypeOffset + nodeTypeSize
const parentPointerSize =  unsafe.Sizeof(uint32(0))
const parentPointerOffset = isRootOffset + isRootSize
const commonNodeHeaderSize = nodeTypeSize + isRootSize + parentPointerSize

// Leaf node header layout
const leafNodeNumCellsSize =  unsafe.Sizeof(uint32(0))
const leafNodeNumCellsOffset = commonNodeHeaderSize
const leafNodeHeaderSize = commonNodeHeaderSize + leafNodeNumCellsSize

// Leaf node body layout
const leafNodeKeySize = unsafe.Sizeof(uint32(0))
const leafNodeKeyOffset = 0
const leafNodeValueSize = rowSize
const leafNodeValueOffset = leafNodeKeyOffset + leafNodeKeySize
const leafNodeCellSize = leafNodeKeySize + uintptr(leafNodeValueSize)
const leafNodeSpaceForCells = pageSize - leafNodeHeaderSize
const leafNodeMaxCells = leafNodeSpaceForCells / leafNodeCellSize

// pattern A [pageSize]byte has methods
type node []byte
type leafNode []byte
func (n node) getNodeType() nodeType {
	b := n[:nodeTypeSize]
	numCells, _ := binary.Uvarint(b)
	return nodeType(numCells)
}

func (n leafNode) numCells() uint32 {
	begin := leafNodeNumCellsOffset
	end := begin + leafNodeNumCellsOffset
	numCells, _ := binary.Uvarint(n[begin:end])
	return uint32(numCells)
}

func (n leafNode) setNumCells(num int) {
	begin := leafNodeNumCellsOffset
	end := begin + leafNodeNumCellsOffset
	binary.PutUvarint(n[begin:end], uint64(num))
}

func (n leafNode) cell(cellNum int) []byte {
	begin := leafNodeHeaderSize + uintptr(cellNum) * leafNodeCellSize
	end := begin + leafNodeCellSize
	return n[begin:end]
}

func (n leafNode) key(cellNum int) uint32 {
	cell := n.cell(cellNum)
	key, _ := binary.Uvarint(cell[:leafNodeKeySize])
	return uint32(key)
}

func (n leafNode) value(cellNum int) []byte {
	cell := n.cell(cellNum)
	return cell[leafNodeValueOffset:]
}

func (n leafNode) initialize() {
	n.setNumCells(0)
}

/*
// pattern B struct represents binary structure
// これだとnode(4096byteのpage)全体をreadしないと内部にアクセスできない
type leafNode struct {
	leafNodeHeader
	cells [leafNodeMaxCells]leafNodeCell
	pad [paddingSize]byte
}
*/
