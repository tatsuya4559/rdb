package backend

func clen(n []byte) int {
	for i := 0; i < len(n); i++ {
		if n[i] == 0 {
			return i
		}
	}
	return len(n)
}

func cstring(n []byte) string {
	return string(n[:clen(n)])
}
