#ifndef CFNT_H
#define CFNT_H

#include <stdint.h>

enum {
	CFNT_LE = 0xFEFF,
	CFNT_BE = 0xFFFE
};

enum {
	 FORMAT_RGBA8 = 0,
	 FORMAT_RGB8 = 1,
	 FORMAT_RGBA5551 = 2,
	 FORMAT_RGB565 = 3,
	 FORMAT_RGBA4 = 4,
	 FORMAT_LA8 = 5,
	 FORMAT_HILO8 = 6,
	 FORMAT_L8 = 7,
	 FORMAT_A8 = 8,
	 FORMAT_LA4 = 9,
	 FORMAT_L4 = 10,
	 FORMAT_A4 = 11,
	 FORMAT_ETC1 = 12,
	 FORMAT_ETC1A4 = 13
};

enum {
 	MAPPING_DIRECT = 0,
 	MAPPING_TABLE = 1,
 	MAPPING_SCAN = 2
};

typedef struct {
	uint8_t left;
	uint8_t glyph;
	uint8_t character;
} __attribute__((packed)) char_width;

typedef struct {
	uint32_t magic;
	uint32_t section_size;
	uint8_t cell_width;
	uint8_t cell_height;
	uint8_t baseline_position;
	uint8_t max_character_width;
	uint32_t sheet_size;
	uint16_t number_of_sheets;
	uint16_t sheet_image_format;
	uint16_t number_of_columns;
	uint16_t number_of_rows;
	uint16_t sheet_width;
	uint16_t sheet_height;
	uint8_t *sheet_data_offset;
} __attribute__((packed)) tglp_header;

typedef struct cwdh_header_t  {
	uint32_t magic;
	uint32_t section_size;
	uint16_t start_index;
	uint16_t end_index;
	cwdh_header_t *next_cwdh_offset;
	char_width widths[];
} __attribute__((packed)) cwdh_header;

typedef struct cmap_header_t {
	uint32_t magic;
	uint32_t section_size;
	uint16_t code_begin;
	uint16_t code_end;
	uint16_t mapping_method;
	uint16_t reserved;
	cmap_header_t *next_cmap_offset;
	union {
		uint16_t direct_glyph_start;
		uint16_t table_glyphs[];
		struct
		{
			uint16_t scan_pair_count;
			struct
			{
				uint16_t code;
				uint16_t glyph;
			} scan_pairs[];
		};
	};
} __attribute__((packed)) cmap_header;

typedef struct {
	uint32_t magic;
	uint16_t endianness;
	uint16_t header_size;
	uint32_t version;
	uint32_t file_size;
	uint32_t number_of_blocks;
} __attribute__((packed)) cfnt_header;

typedef struct {
	uint32_t magic;
	uint32_t section_size;
	uint8_t font_type;
	uint8_t line_feed;
	uint16_t alter_char_index;
	char_width default_char_width;
	uint8_t encoding;
	tglp_header *tglp_offset;
	cwdh_header *cwdh_offset;
	cmap_header *cmap_offset;
	uint8_t height;
	uint8_t width;
	uint8_t ascent;
	uint8_t reserved;
} __attribute__((packed)) finf_header;

#endif
