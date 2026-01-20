/* png.c - PNG Decoder for EmConOs */
/* Supports: PNG with deflate compression, RGB/RGBA, 8-bit depth */

#include <stdint.h>
#include <hw.h>

/* PNG Signature */
static const uint8_t PNG_SIG[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

/* Chunk types */
#define CHUNK_IHDR 0x49484452
#define CHUNK_IDAT 0x49444154
#define CHUNK_IEND 0x49454E44
#define CHUNK_PLTE 0x504C5445

/* Filter types */
#define FILTER_NONE    0
#define FILTER_SUB     1
#define FILTER_UP      2
#define FILTER_AVERAGE 3
#define FILTER_PAETH   4

/* PNG Color types */
#define PNG_COLOR_GRAY       0
#define PNG_COLOR_RGB        2
#define PNG_COLOR_INDEXED    3
#define PNG_COLOR_GRAY_ALPHA 4
#define PNG_COLOR_RGBA       6

/* ============== DEFLATE DECOMPRESSOR ============== */

/* Huffman code limits */
#define MAX_BITS 15
#define MAX_LIT_CODES 286
#define MAX_DIST_CODES 30
#define MAX_CODE_LEN_CODES 19

/* Huffman table */
typedef struct {
    uint16_t counts[MAX_BITS + 1];
    uint16_t symbols[MAX_LIT_CODES];
} HuffmanTable;

/* Deflate state */
typedef struct {
    const uint8_t *src;
    uint32_t src_len;
    uint32_t src_pos;

    uint8_t *dst;
    uint32_t dst_len;
    uint32_t dst_pos;

    uint32_t bit_buf;
    uint32_t bit_cnt;
} DeflateState;

/* Fixed Huffman tables (pre-computed) */
static HuffmanTable fixed_lit_table;
static HuffmanTable fixed_dist_table;
static int tables_initialized = 0;

/* Code length order for dynamic Huffman */
static const uint8_t code_len_order[19] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

/* Base lengths for length codes 257-285 */
static const uint16_t len_base[29] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};

/* Extra bits for length codes */
static const uint8_t len_extra[29] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};

/* Base distances for distance codes */
static const uint16_t dist_base[30] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};

/* Extra bits for distance codes */
static const uint8_t dist_extra[30] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

/* Read bits from stream (LSB first) */
static uint32_t read_bits(DeflateState *s, int n) {
    while(s->bit_cnt < (uint32_t)n) {
        if(s->src_pos >= s->src_len) return 0;
        s->bit_buf |= (uint32_t)s->src[s->src_pos++] << s->bit_cnt;
        s->bit_cnt += 8;
    }
    uint32_t val = s->bit_buf & ((1u << n) - 1);
    s->bit_buf >>= n;
    s->bit_cnt -= n;
    return val;
}

/* Build Huffman table from code lengths */
static int build_huffman(HuffmanTable *tbl, const uint8_t *lens, int num_codes) {
    int i;

    /* Count code lengths */
    for(i = 0; i <= MAX_BITS; i++) {
        tbl->counts[i] = 0;
    }
    for(i = 0; i < num_codes; i++) {
        if(lens[i] <= MAX_BITS) {
            tbl->counts[lens[i]]++;
        }
    }
    tbl->counts[0] = 0;

    /* Compute offsets */
    uint16_t offsets[MAX_BITS + 1];
    offsets[0] = 0;
    for(i = 1; i <= MAX_BITS; i++) {
        offsets[i] = offsets[i-1] + tbl->counts[i-1];
    }

    /* Build symbol table */
    for(i = 0; i < num_codes; i++) {
        if(lens[i] > 0 && lens[i] <= MAX_BITS) {
            tbl->symbols[offsets[lens[i]]++] = i;
        }
    }

    return 0;
}

/* Decode one symbol using Huffman table */
static int decode_symbol(DeflateState *s, HuffmanTable *tbl) {
    int code = 0;
    int first = 0;
    int index = 0;

    for(int len = 1; len <= MAX_BITS; len++) {
        code |= read_bits(s, 1);
        int count = tbl->counts[len];
        if(code < first + count) {
            return tbl->symbols[index + (code - first)];
        }
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }

    return -1; /* Error */
}

/* Static buffer for fixed table init */
static uint8_t fixed_lens[288];

/* Initialize fixed Huffman tables */
static void init_fixed_tables(void) {
    if(tables_initialized) return;

    uart_puts("[INFLATE] building fixed lit table\n");

    int i;
    /* Fixed literal/length table */
    for(i = 0; i < 144; i++) fixed_lens[i] = 8;
    for(i = 144; i < 256; i++) fixed_lens[i] = 9;
    for(i = 256; i < 280; i++) fixed_lens[i] = 7;
    for(i = 280; i < 288; i++) fixed_lens[i] = 8;
    build_huffman(&fixed_lit_table, fixed_lens, 288);

    uart_puts("[INFLATE] building fixed dist table\n");

    /* Fixed distance table */
    for(i = 0; i < 32; i++) fixed_lens[i] = 5;
    build_huffman(&fixed_dist_table, fixed_lens, 32);

    tables_initialized = 1;
    uart_puts("[INFLATE] fixed tables ready\n");
}

/* Decode dynamic Huffman tables */
static int decode_dynamic_tables(DeflateState *s, HuffmanTable *lit_tbl, HuffmanTable *dist_tbl) {
    int hlit = read_bits(s, 5) + 257;
    int hdist = read_bits(s, 5) + 1;
    int hclen = read_bits(s, 4) + 4;

    /* Read code length code lengths */
    uint8_t code_lens[19] = {0};
    for(int i = 0; i < hclen; i++) {
        code_lens[code_len_order[i]] = read_bits(s, 3);
    }

    /* Build code length table */
    HuffmanTable code_len_tbl;
    build_huffman(&code_len_tbl, code_lens, 19);

    /* Decode literal/length and distance code lengths */
    uint8_t lens[MAX_LIT_CODES + MAX_DIST_CODES];
    int num = 0;
    int total = hlit + hdist;

    while(num < total) {
        int sym = decode_symbol(s, &code_len_tbl);
        if(sym < 0) return -1;

        if(sym < 16) {
            lens[num++] = sym;
        } else if(sym == 16) {
            int repeat = read_bits(s, 2) + 3;
            uint8_t prev = (num > 0) ? lens[num - 1] : 0;
            while(repeat-- > 0 && num < total) {
                lens[num++] = prev;
            }
        } else if(sym == 17) {
            int repeat = read_bits(s, 3) + 3;
            while(repeat-- > 0 && num < total) {
                lens[num++] = 0;
            }
        } else if(sym == 18) {
            int repeat = read_bits(s, 7) + 11;
            while(repeat-- > 0 && num < total) {
                lens[num++] = 0;
            }
        }
    }

    /* Build tables */
    build_huffman(lit_tbl, lens, hlit);
    build_huffman(dist_tbl, lens + hlit, hdist);

    return 0;
}

/* Decode a deflate block */
static int decode_block(DeflateState *s, HuffmanTable *lit_tbl, HuffmanTable *dist_tbl) {
    while(1) {
        int sym = decode_symbol(s, lit_tbl);
        if(sym < 0) return -1;

        if(sym < 256) {
            /* Literal byte */
            if(s->dst_pos >= s->dst_len) return -1;
            s->dst[s->dst_pos++] = (uint8_t)sym;
        } else if(sym == 256) {
            /* End of block */
            return 0;
        } else {
            /* Length/distance pair */
            sym -= 257;
            if(sym >= 29) return -1;

            int length = len_base[sym] + read_bits(s, len_extra[sym]);

            int dist_sym = decode_symbol(s, dist_tbl);
            if(dist_sym < 0 || dist_sym >= 30) return -1;

            int distance = dist_base[dist_sym] + read_bits(s, dist_extra[dist_sym]);

            /* Copy from history */
            if(s->dst_pos < (uint32_t)distance) return -1;

            uint32_t src_pos = s->dst_pos - distance;
            for(int i = 0; i < length; i++) {
                if(s->dst_pos >= s->dst_len) return -1;
                s->dst[s->dst_pos++] = s->dst[src_pos++];
            }
        }
    }
}

/* Main deflate decompression */
static int inflate(const uint8_t *src, uint32_t src_len, uint8_t *dst, uint32_t dst_len) {
    uart_puts("[INFLATE] src_len=");
    uart_hex(src_len);
    uart_puts(" dst_len=");
    uart_hex(dst_len);
    uart_puts("\n");

    uart_puts("[INFLATE] init tables\n");
    init_fixed_tables();
    uart_puts("[INFLATE] tables done\n");

    DeflateState state = {
        .src = src,
        .src_len = src_len,
        .src_pos = 0,
        .dst = dst,
        .dst_len = dst_len,
        .dst_pos = 0,
        .bit_buf = 0,
        .bit_cnt = 0
    };

    int bfinal;
    do {
        bfinal = read_bits(&state, 1);
        int btype = read_bits(&state, 2);

        uart_puts("[INFLATE] bfinal=");
        uart_hex(bfinal);
        uart_puts(" btype=");
        uart_hex(btype);
        uart_puts("\n");

        if(btype == 0) {
            /* Stored block */
            state.bit_buf = 0;
            state.bit_cnt = 0;

            if(state.src_pos + 4 > state.src_len) return -1;
            uint16_t len = state.src[state.src_pos] | (state.src[state.src_pos + 1] << 8);
            state.src_pos += 4; /* Skip len and nlen */

            if(state.src_pos + len > state.src_len) return -1;
            if(state.dst_pos + len > state.dst_len) return -1;

            for(int i = 0; i < len; i++) {
                state.dst[state.dst_pos++] = state.src[state.src_pos++];
            }
        } else if(btype == 1) {
            /* Fixed Huffman */
            if(decode_block(&state, &fixed_lit_table, &fixed_dist_table) < 0) {
                return -1;
            }
        } else if(btype == 2) {
            /* Dynamic Huffman */
            HuffmanTable lit_tbl, dist_tbl;
            if(decode_dynamic_tables(&state, &lit_tbl, &dist_tbl) < 0) {
                return -1;
            }
            if(decode_block(&state, &lit_tbl, &dist_tbl) < 0) {
                return -1;
            }
        } else {
            return -1; /* Invalid block type */
        }
    } while(!bfinal);

    return state.dst_pos;
}

/* ============== PNG DECODER ============== */

/* CRC32 for chunk validation (optional, can skip) */
static uint32_t crc32_table[256];
static int crc_initialized = 0;

static void init_crc32(void) {
    if(crc_initialized) return;
    for(uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for(int j = 0; j < 8; j++) {
            c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        }
        crc32_table[i] = c;
    }
    crc_initialized = 1;
}

/* Read big-endian 32-bit value */
static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

/* Paeth predictor for PNG filtering */
static uint8_t paeth_predictor(int a, int b, int c) {
    int p = a + b - c;
    int pa = p > a ? p - a : a - p;
    int pb = p > b ? p - b : b - p;
    int pc = p > c ? p - c : c - p;

    if(pa <= pb && pa <= pc) return (uint8_t)a;
    if(pb <= pc) return (uint8_t)b;
    return (uint8_t)c;
}

/* Apply PNG filter to a scanline - noinline to prevent optimization issues */
__attribute__((noinline))
static void unfilter_row(volatile uint8_t *row, volatile const uint8_t *prev, int filter, int bpp, int len) {
    int i;

    if(filter == FILTER_NONE) {
        /* No filter */
        return;
    }

    if(filter == FILTER_SUB) {
        for(i = bpp; i < len; i++) {
            row[i] = row[i] + row[i - bpp];
        }
        return;
    }

    if(filter == FILTER_UP) {
        for(i = 0; i < len; i++) {
            row[i] = row[i] + prev[i];
        }
        return;
    }

    if(filter == FILTER_AVERAGE) {
        for(i = 0; i < bpp; i++) {
            row[i] = row[i] + (prev[i] >> 1);
        }
        for(i = bpp; i < len; i++) {
            row[i] = row[i] + ((row[i - bpp] + prev[i]) >> 1);
        }
        return;
    }

    if(filter == FILTER_PAETH) {
        for(i = 0; i < bpp; i++) {
            row[i] = row[i] + paeth_predictor(0, prev[i], 0);
        }
        for(i = bpp; i < len; i++) {
            row[i] = row[i] + paeth_predictor(row[i - bpp], prev[i], prev[i - bpp]);
        }
        return;
    }
}

/* PNG decode buffers - static to avoid stack overflow */
#define PNG_MAX_WIDTH 640
#define PNG_MAX_HEIGHT 480
#define PNG_MAX_IDAT (256 * 1024)  /* 256KB for compressed data */
#define PNG_MAX_RAW  (PNG_MAX_WIDTH * PNG_MAX_HEIGHT * 4 + PNG_MAX_HEIGHT)  /* RGBA + filter bytes */

static uint8_t png_idat_buf[PNG_MAX_IDAT];
static uint8_t png_raw_buf[PNG_MAX_RAW];
static uint8_t png_prev_row[PNG_MAX_WIDTH * 4];
static uint8_t png_palette[256 * 3];  /* Max 256 colors, RGB */

/* PNG decode function */
int png_decode(const uint8_t *data, uint32_t data_len,
               uint8_t *pixels, int max_width, int max_height,
               int *out_width, int *out_height) {

    init_crc32();

    /* Check PNG signature */
    if(data_len < 8) return -1;
    for(int i = 0; i < 8; i++) {
        if(data[i] != PNG_SIG[i]) return -1;
    }

    uint32_t pos = 8;
    uint32_t width = 0, height = 0;
    uint8_t bit_depth = 0, color_type = 0;
    uint32_t idat_len = 0;
    int bpp = 0; /* bytes per pixel */
    int palette_size = 0;

    /* Parse chunks */
    uart_puts("[PNG] Parsing chunks... data=");
    uart_hex((uint32_t)(uintptr_t)data);
    uart_puts(" len=");
    uart_hex(data_len);
    uart_puts("\n");

    while(pos + 12 <= data_len) {
        uart_puts("[PNG] loop: pos=");
        uart_hex(pos);
        uart_puts(" reading at ");
        uart_hex((uint32_t)(uintptr_t)(data + pos));
        uart_puts("\n");

        volatile uint8_t b0 = data[pos];
        volatile uint8_t b1 = data[pos + 1];
        volatile uint8_t b2 = data[pos + 2];
        volatile uint8_t b3 = data[pos + 3];
        uint32_t chunk_len = ((uint32_t)b0 << 24) | ((uint32_t)b1 << 16) | ((uint32_t)b2 << 8) | (uint32_t)b3;

        volatile uint8_t t0 = data[pos + 4];
        volatile uint8_t t1 = data[pos + 5];
        volatile uint8_t t2 = data[pos + 6];
        volatile uint8_t t3 = data[pos + 7];
        uint32_t chunk_type = ((uint32_t)t0 << 24) | ((uint32_t)t1 << 16) | ((uint32_t)t2 << 8) | (uint32_t)t3;

        uart_puts("[PNG] chunk pos=");
        uart_hex(pos);
        uart_puts(" type=");
        uart_hex(chunk_type);
        uart_puts(" len=");
        uart_hex(chunk_len);
        uart_puts("\n");

        if(pos + 12 + chunk_len > data_len) {
            uart_puts("[PNG] chunk overflow, breaking\n");
            break;
        }

        const uint8_t *chunk_data = data + pos + 8;

        if(chunk_type == CHUNK_IHDR) {
            /* Image header */
            if(chunk_len < 13) return -1;

            width = read_be32(chunk_data);
            height = read_be32(chunk_data + 4);
            bit_depth = chunk_data[8];
            color_type = chunk_data[9];

            uart_puts("[PNG] W=");
            uart_hex(width);
            uart_puts(" H=");
            uart_hex(height);
            uart_puts(" depth=");
            uart_hex(bit_depth);
            uart_puts(" color=");
            uart_hex(color_type);
            uart_puts("\n");

            /* Only support 8-bit RGB, RGBA, and indexed */
            if(bit_depth != 8) {
                uart_puts("[PNG] Unsupported bit depth\n");
                return -1;
            }
            if(color_type != PNG_COLOR_RGB && color_type != PNG_COLOR_RGBA && color_type != PNG_COLOR_INDEXED) {
                uart_puts("[PNG] Unsupported color type\n");
                return -1;
            }

            if(width > (uint32_t)max_width || height > (uint32_t)max_height) {
                uart_puts("[PNG] Image too large\n");
                return -1;
            }

            if(color_type == PNG_COLOR_RGBA) {
                bpp = 4;
            } else if(color_type == PNG_COLOR_RGB) {
                bpp = 3;
            } else {
                bpp = 1;  /* Indexed = 1 byte per pixel */
            }
            uart_puts("[PNG] bpp=");
            uart_hex(bpp);
            uart_puts(" IHDR done\n");

        } else if(chunk_type == CHUNK_PLTE) {
            /* Palette chunk */
            uart_puts("[PNG] PLTE chunk len=");
            uart_hex(chunk_len);
            uart_puts("\n");
            palette_size = chunk_len / 3;
            if(palette_size > 256) palette_size = 256;
            uart_puts("[PNG] reading palette...\n");
            volatile const uint8_t *pal_src = chunk_data;
            for(int i = 0; i < palette_size * 3; i++) {
                png_palette[i] = pal_src[i];
            }
            uart_puts("[PNG] palette_size=");
            uart_hex(palette_size);
            uart_puts("\n");

        } else if(chunk_type == CHUNK_IDAT) {
            /* Image data - collect all IDAT chunks */
            uart_puts("[PNG] IDAT chunk len=");
            uart_hex(chunk_len);
            uart_puts("\n");
            if(idat_len + chunk_len > PNG_MAX_IDAT) {
                uart_puts("[PNG] IDAT too large\n");
                return -1;
            }
            for(uint32_t i = 0; i < chunk_len; i++) {
                png_idat_buf[idat_len++] = chunk_data[i];
            }

        } else if(chunk_type == CHUNK_IEND) {
            /* End of image */
            uart_puts("[PNG] IEND found\n");
            break;
        }

        pos += 12 + chunk_len;
        uart_puts("[PNG] next pos=");
        uart_hex(pos);
        uart_puts("\n");
    }

    uart_puts("[PNG] Loop done. Total IDAT: ");
    uart_hex(idat_len);
    uart_puts("\n");

    if(width == 0 || height == 0 || idat_len == 0) {
        uart_puts("[PNG] Invalid PNG\n");
        return -1;
    }

    /* Check palette for indexed images */
    if(color_type == PNG_COLOR_INDEXED && palette_size == 0) {
        uart_puts("[PNG] Indexed PNG but no palette!\n");
        return -1;
    }

    /* Skip zlib header (2 bytes) */
    if(idat_len < 6) return -1;  /* Need at least header + adler32 */

    uint8_t cmf = png_idat_buf[0];
    uint8_t flg = png_idat_buf[1];

    if((cmf & 0x0F) != 8) {
        uart_puts("[PNG] Not deflate\n");
        return -1;
    }
    if(flg & 0x20) {
        uart_puts("[PNG] FDICT not supported\n");
        return -1;
    }

    /* Decompress */
    uart_puts("[PNG] Decompressing...\n");

    uint32_t raw_size = (width * bpp + 1) * height;  /* +1 for filter byte per row */
    if(raw_size > PNG_MAX_RAW) {
        uart_puts("[PNG] Image too large for buffer\n");
        return -1;
    }

    int decompressed = inflate(png_idat_buf + 2, idat_len - 6, png_raw_buf, raw_size);
    if(decompressed < 0) {
        uart_puts("[PNG] Decompression failed\n");
        return -1;
    }

    uart_puts("[PNG] Decompressed ");
    uart_hex(decompressed);
    uart_puts(" bytes\n");

    /* Unfilter and convert to RGBA */
    uart_puts("[PNG] Unfiltering...\n");

    int row_bytes = (int)(width * bpp);
    uart_puts("[PNG] row_bytes=");
    uart_hex(row_bytes);
    uart_puts("\n");

    uint32_t raw_pos = 0;

    uart_puts("[PNG] Clearing prev_row\n");
    /* Clear previous row */
    for(int i = 0; i < row_bytes; i++) {
        png_prev_row[i] = 0;
    }
    uart_puts("[PNG] prev_row cleared\n");

    uart_puts("[PNG] Starting row loop, height=");
    uart_hex(height);
    uart_puts("\n");

    for(uint32_t y = 0; y < height; y++) {
        /* Progress her 20 satırda bir */
        if(y % 20 == 0) {
            uart_puts("[PNG] row ");
            uart_hex(y);
            uart_puts("\n");
        }

        /* Get filter type */
        uint8_t filter = png_raw_buf[raw_pos++];

        /* Debug: filter type kontrol */
        if(filter > 4) {
            uart_puts("[PNG] BAD filter=");
            uart_hex(filter);
            uart_puts(" at y=");
            uart_hex(y);
            uart_puts(" raw_pos=");
            uart_hex(raw_pos - 1);
            uart_puts("\n");
            return -1;
        }

        /* Get row data pointer */
        volatile uint8_t *row = png_raw_buf + raw_pos;
        raw_pos += row_bytes;

        /* Unfilter */
        unfilter_row(row, png_prev_row, filter, bpp, row_bytes);

        /* Copy to previous row for next iteration */
        for(int i = 0; i < row_bytes; i++) {
            png_prev_row[i] = row[i];
        }

        /* Convert to output format (RGBA) */
        volatile uint8_t *out = pixels;
        for(uint32_t x = 0; x < width; x++) {
            uint32_t dst_idx = (y * (uint32_t)max_width + x) * 4;

            if(color_type == PNG_COLOR_INDEXED) {
                /* Indexed color - lookup in palette */
                /* PNG: RGB, Framebuffer: BGR */
                uint8_t idx = row[x];
                out[dst_idx + 0] = png_palette[idx * 3 + 2];  /* B */
                out[dst_idx + 1] = png_palette[idx * 3 + 1];  /* G */
                out[dst_idx + 2] = png_palette[idx * 3 + 0];  /* R */
                out[dst_idx + 3] = 255;  /* A */
            } else {
                /* PNG: RGB(A), Framebuffer: BGR(A) */
                uint32_t src_idx = x * bpp;
                out[dst_idx + 0] = row[src_idx + 2];  /* B */
                out[dst_idx + 1] = row[src_idx + 1];  /* G */
                out[dst_idx + 2] = row[src_idx + 0];  /* R */
                out[dst_idx + 3] = (bpp == 4) ? row[src_idx + 3] : 255;  /* A */
            }
        }
    }

    uart_puts("[PNG] All rows done\n");

    *out_width = width;
    *out_height = height;

    uart_puts("[PNG] Decode complete\n");
    return 0;
}
