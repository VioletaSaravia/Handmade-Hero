package engine

import "core:mem"
import "core:os"
import fp "core:path/filepath"

WHITE :: [4]f32{1, 1, 1, 1}
BLACK :: [4]f32{0, 0, 0, 1}

BMPColor :: struct #packed {
	b, g, r, a: u8,
}

BMPCompression :: enum u32 {
	RGB,
	RLE8,
	RLE4,
	BITFIELDS,
	JPEG,
	PNG,
	ALPHABITFIELDS,
	CMYK,
	CMYKRLE8,
	CMYKRLE4,
}

AseBMP32x32 :: struct #packed {
	// BMFH
	bf_type:        u16,
	bf_size:        u32,
	bf_reserved1:   u16,
	bf_reserved2:   u16,
	bf_off_bits:    u32,

	// BMIH
	size:           u32,
	width:          i32,
	height:         i32,
	planes:         u16,
	bit_count:      u16,
	compression:    BMPCompression,
	size_image:     u32,
	pels_per_meter: [2]i32,
	clr_used:       u32,
	clr_important:  u32,

	// DATA
	rgbq:           [256]BMPColor,
	line_data:      [1024]byte,
}

Image :: []u8

LoadImage :: proc($path: string) -> (result: Image, ok: bool) {
	data := raw_data(#load(DATA + path, []byte))

	switch fp.ext(path) {
	case ".bmp":
		data := transmute(^AseBMP32x32)data
		result = make([]u8, 1024 * 4)
		for color, i in data.rgbq {
			id := i * 4
			result[id + 0] = color.r
			result[id + 1] = color.g
			result[id + 2] = color.b
			result[id + 3] = color.a
		}
		return result, true
	case:
	}

	return result, false
}
