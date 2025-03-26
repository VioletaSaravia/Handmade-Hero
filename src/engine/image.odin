package engine

import "core:mem"

BMPColor :: struct {
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

AseBMP32x32 :: struct {
	// BMFH
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

LoadImage :: proc(path: string) -> (result: Image) {
	return
}

ImageFromAseBMP32x32 :: proc(bmp: AseBMP32x32) -> (result: Image) {
	// no
	result = []u8{}

	for i := 0; i < len(bmp.line_data); i += 4 {
		color := bmp.rgbq[i]
		result[i] = color.r
		result[i + 1] = color.g
		result[i + 2] = color.b
		result[i + 3] = color.a
	}

	return
}
