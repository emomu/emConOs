#!/usr/bin/env python3
"""
Inter fontunu C array'e çeviren script
Kullanım: python3 generate_font.py
Gerekli: pip install pillow requests
"""

import os
import requests
from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import zipfile
import io

# Font URL
FONT_URL = "https://github.com/rsms/inter/releases/download/v4.0/Inter-4.0.zip"

# Oluşturulacak font varyantları
FONT_VARIANTS = [
    # (dosya_adı, ttf_adı, boyut, açıklama)
    ("font_inter_16", "Inter-Regular.ttf", 16, "Inter Regular 16px"),
    ("font_inter_16_medium", "Inter-Medium.ttf", 16, "Inter Medium 16px"),
    ("font_inter_16_bold", "Inter-Bold.ttf", 16, "Inter Bold 16px"),
    ("font_inter_20", "Inter-Regular.ttf", 20, "Inter Regular 20px"),
    ("font_inter_20_medium", "Inter-Medium.ttf", 20, "Inter Medium 20px"),
    ("font_inter_20_bold", "Inter-Bold.ttf", 20, "Inter Bold 20px"),
    ("font_inter_24", "Inter-Regular.ttf", 24, "Inter Regular 24px"),
    ("font_inter_24_medium", "Inter-Medium.ttf", 24, "Inter Medium 24px"),
    ("font_inter_24_bold", "Inter-Bold.ttf", 24, "Inter Bold 24px"),
    ("font_inter_32", "Inter-Regular.ttf", 32, "Inter Regular 32px"),
    ("font_inter_32_medium", "Inter-Medium.ttf", 32, "Inter Medium 32px"),
    ("font_inter_32_bold", "Inter-Bold.ttf", 32, "Inter Bold 32px"),
]

# Karakter aralığı (ASCII 32-126 + Türkçe karakterler)
CHAR_START = 32
CHAR_END = 127
ASCII_CHAR_COUNT = CHAR_END - CHAR_START

# Türkçe karakterler (Unicode codepoints)
TURKISH_CHARS = [
    (0x011F, 'ğ'),  # g with breve
    (0x011E, 'Ğ'),  # G with breve
    (0x00FC, 'ü'),  # u with diaeresis
    (0x00DC, 'Ü'),  # U with diaeresis
    (0x015F, 'ş'),  # s with cedilla
    (0x015E, 'Ş'),  # S with cedilla
    (0x0131, 'ı'),  # dotless i
    (0x0130, 'İ'),  # I with dot above
    (0x00F6, 'ö'),  # o with diaeresis
    (0x00D6, 'Ö'),  # O with diaeresis
    (0x00E7, 'ç'),  # c with cedilla
    (0x00C7, 'Ç'),  # C with cedilla
]
TURKISH_CHAR_COUNT = len(TURKISH_CHARS)
CHAR_COUNT = ASCII_CHAR_COUNT + TURKISH_CHAR_COUNT

def download_fonts():
    """Inter fontlarını indir"""
    needed_fonts = set(v[1] for v in FONT_VARIANTS)

    # Zaten var mı kontrol et
    all_exist = all(os.path.exists(f) for f in needed_fonts)
    if all_exist:
        print("Fontlar zaten mevcut")
        return

    print(f"Fontlar indiriliyor...")
    response = requests.get(FONT_URL)

    with zipfile.ZipFile(io.BytesIO(response.content)) as z:
        for name in z.namelist():
            # extras/ttf/ klasöründeki fontları bul
            if 'extras/ttf/' in name and name.endswith('.ttf'):
                font_basename = os.path.basename(name)
                if font_basename in needed_fonts:
                    print(f"  Çıkarılıyor: {font_basename}")
                    with z.open(name) as f:
                        with open(font_basename, 'wb') as out:
                            out.write(f.read())

    print("Fontlar indirildi!")

def render_char(font, char, code, font_size, font_height):
    """Tek bir karakteri render et"""
    # Karakter boyutlarını al
    bbox = font.getbbox(char)
    if bbox:
        left, top, right, bottom = bbox
        char_width = right - left
        advance = font.getlength(char)
    else:
        char_width = font_size // 3
        advance = char_width

    # Advance width (karakter arası boşluk dahil)
    advance = int(advance) + 1
    if advance < 3:
        advance = font_size // 3

    # Görüntü oluştur
    img = Image.new('L', (advance, font_height), 0)
    draw = ImageDraw.Draw(img)

    # Karakteri çiz (baseline'a göre)
    draw.text((0, 0), char, font=font, fill=255)

    # Piksel verilerini al
    pixels = list(img.getdata())

    return {
        'char': char,
        'code': code,
        'width': advance,
        'height': font_height,
        'pixels': pixels
    }

def render_font(font_file, font_size):
    """Fontu render et (ASCII + Türkçe)"""
    font = ImageFont.truetype(font_file, font_size)

    # Ascent ve descent hesapla
    ascent, descent = font.getmetrics()
    font_height = ascent + descent

    # ASCII karakterleri render et
    ascii_glyphs = []
    for code in range(CHAR_START, CHAR_END):
        char = chr(code)
        glyph = render_char(font, char, code, font_size, font_height)
        ascii_glyphs.append(glyph)

    # Türkçe karakterleri render et
    turkish_glyphs = []
    for code, char in TURKISH_CHARS:
        glyph = render_char(font, char, code, font_size, font_height)
        turkish_glyphs.append(glyph)

    return ascii_glyphs, turkish_glyphs, font_height

def generate_c_code(ascii_glyphs, turkish_glyphs, font_height, output_name, description):
    """C kodu oluştur (ASCII + Türkçe)"""

    # Makro isimleri için büyük harf
    macro_prefix = output_name.upper()

    # Header dosyası
    header = f"""/* {output_name}.h - {description} */
#ifndef {macro_prefix}_H
#define {macro_prefix}_H

#include "types.h"

#define {macro_prefix}_HEIGHT {font_height}
#define {macro_prefix}_FIRST_CHAR {CHAR_START}
#define {macro_prefix}_LAST_CHAR {CHAR_END - 1}
#define {macro_prefix}_ASCII_COUNT {ASCII_CHAR_COUNT}
#define {macro_prefix}_TURKISH_COUNT {TURKISH_CHAR_COUNT}
#define {macro_prefix}_CHAR_COUNT ({ASCII_CHAR_COUNT} + {TURKISH_CHAR_COUNT})

/* Türkçe karakter indeksleri */
#define TR_IDX_G_BREVE_LOWER  0   /* ğ */
#define TR_IDX_G_BREVE_UPPER  1   /* Ğ */
#define TR_IDX_U_UMLAUT_LOWER 2   /* ü */
#define TR_IDX_U_UMLAUT_UPPER 3   /* Ü */
#define TR_IDX_S_CEDILLA_LOWER 4  /* ş */
#define TR_IDX_S_CEDILLA_UPPER 5  /* Ş */
#define TR_IDX_I_DOTLESS      6   /* ı */
#define TR_IDX_I_DOTTED       7   /* İ */
#define TR_IDX_O_UMLAUT_LOWER 8   /* ö */
#define TR_IDX_O_UMLAUT_UPPER 9   /* Ö */
#define TR_IDX_C_CEDILLA_LOWER 10 /* ç */
#define TR_IDX_C_CEDILLA_UPPER 11 /* Ç */

/* Karakter bilgisi */
typedef struct {{
    uint8_t width;      /* Karakter genişliği */
    uint32_t offset;    /* Piksel verisinin başlangıç offseti */
}} {output_name}_Glyph;

/* Font verileri */
extern const {output_name}_Glyph {output_name}_glyphs[{ASCII_CHAR_COUNT}];
extern const {output_name}_Glyph {output_name}_turkish_glyphs[{TURKISH_CHAR_COUNT}];
extern const uint8_t {output_name}_pixels[];

/* Karakter çizme fonksiyonu */
void {output_name}_draw_char(int x, int y, char c, uint32_t color);
void {output_name}_draw_turkish_char(int x, int y, int tr_index, uint32_t color);
void {output_name}_draw_text(int x, int y, const char* text, uint32_t color);
int {output_name}_text_width(const char* text);
int {output_name}_turkish_char_width(int tr_index);

#endif
"""

    # C dosyası
    c_code = f"""/* {output_name}.c - {description} */
#include <fonts/{output_name}.h>
#include <graphics.h>

/* ASCII Glyph bilgileri (genişlik ve offset) */
const {output_name}_Glyph {output_name}_glyphs[{ASCII_CHAR_COUNT}] = {{
"""

    # ASCII Glyph bilgilerini ekle
    offset = 0
    for i, glyph in enumerate(ascii_glyphs):
        char_display = glyph['char'] if glyph['code'] > 32 else ' '
        c_code += f"    {{ {glyph['width']:2d}, {offset:5d} }},  /* {glyph['code']:3d}: '{char_display}' */\n"
        offset += glyph['width'] * font_height

    c_code += "};\n\n"

    # Türkçe Glyph bilgilerini ekle
    c_code += f"/* Türkçe karakter Glyph bilgileri */\n"
    c_code += f"const {output_name}_Glyph {output_name}_turkish_glyphs[{TURKISH_CHAR_COUNT}] = {{\n"

    turkish_names = ['ğ', 'Ğ', 'ü', 'Ü', 'ş', 'Ş', 'ı', 'İ', 'ö', 'Ö', 'ç', 'Ç']
    for i, glyph in enumerate(turkish_glyphs):
        c_code += f"    {{ {glyph['width']:2d}, {offset:5d} }},  /* {turkish_names[i]} */\n"
        offset += glyph['width'] * font_height

    c_code += "};\n\n"

    # Piksel verilerini ekle (kompakt format)
    c_code += "/* Piksel verileri (8-bit alpha) */\n"
    c_code += f"const uint8_t {output_name}_pixels[] = {{\n"

    all_pixels = []
    for glyph in ascii_glyphs:
        all_pixels.extend(glyph['pixels'])
    for glyph in turkish_glyphs:
        all_pixels.extend(glyph['pixels'])

    # 20 piksel per satır
    for i in range(0, len(all_pixels), 20):
        chunk = all_pixels[i:i+20]
        c_code += "    " + ",".join(f"0x{p:02X}" for p in chunk) + ",\n"

    c_code += "};\n\n"

    # Çizim fonksiyonları
    c_code += f"""/* UTF-8 karakterini Türkçe indeksine dönüştür */
static int {output_name}_utf8_to_turkish(const char **str) {{
    const unsigned char *s = (const unsigned char *)*str;
    if(s[0] < 0x80) return -1;

    if((s[0] & 0xE0) == 0xC0 && s[1]) {{
        unsigned int cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        (*str) += 1;
        switch(cp) {{
            case 287: return 0;   /* ğ */
            case 286: return 1;   /* Ğ */
            case 252: return 2;   /* ü */
            case 220: return 3;   /* Ü */
            case 351: return 4;   /* ş */
            case 350: return 5;   /* Ş */
            case 305: return 6;   /* ı */
            case 304: return 7;   /* İ */
            case 246: return 8;   /* ö */
            case 214: return 9;   /* Ö */
            case 231: return 10;  /* ç */
            case 199: return 11;  /* Ç */
            default: return -1;
        }}
    }}
    if((s[0] & 0xF0) == 0xE0 && s[1] && s[2]) {{ (*str) += 2; return -1; }}
    if((s[0] & 0xF8) == 0xF0 && s[1] && s[2] && s[3]) {{ (*str) += 3; return -1; }}
    return -1;
}}

/* Tek ASCII karakter çiz */
void {output_name}_draw_char(int x, int y, char c, uint32_t color) {{
    if(c < {macro_prefix}_FIRST_CHAR || c > {macro_prefix}_LAST_CHAR) return;

    int idx = c - {macro_prefix}_FIRST_CHAR;
    const {output_name}_Glyph* glyph = &{output_name}_glyphs[idx];
    const uint8_t* pixels = &{output_name}_pixels[glyph->offset];

    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    for(int py = 0; py < {macro_prefix}_HEIGHT; py++) {{
        for(int px = 0; px < glyph->width; px++) {{
            uint8_t alpha = pixels[py * glyph->width + px];
            if(alpha > 8) {{
                int sx = x + px;
                int sy = y + py;

                if(alpha >= 250) {{
                    draw_pixel(sx, sy, color);
                }} else {{
                    uint8_t nr = (r * alpha) >> 8;
                    uint8_t ng = (g * alpha) >> 8;
                    uint8_t nb = (b * alpha) >> 8;
                    draw_pixel(sx, sy, 0xFF000000 | (nr << 16) | (ng << 8) | nb);
                }}
            }}
        }}
    }}
}}

/* Türkçe karakter çiz */
void {output_name}_draw_turkish_char(int x, int y, int tr_index, uint32_t color) {{
    if(tr_index < 0 || tr_index >= {macro_prefix}_TURKISH_COUNT) return;

    const {output_name}_Glyph* glyph = &{output_name}_turkish_glyphs[tr_index];
    const uint8_t* pixels = &{output_name}_pixels[glyph->offset];

    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    for(int py = 0; py < {macro_prefix}_HEIGHT; py++) {{
        for(int px = 0; px < glyph->width; px++) {{
            uint8_t alpha = pixels[py * glyph->width + px];
            if(alpha > 8) {{
                int sx = x + px;
                int sy = y + py;

                if(alpha >= 250) {{
                    draw_pixel(sx, sy, color);
                }} else {{
                    uint8_t nr = (r * alpha) >> 8;
                    uint8_t ng = (g * alpha) >> 8;
                    uint8_t nb = (b * alpha) >> 8;
                    draw_pixel(sx, sy, 0xFF000000 | (nr << 16) | (ng << 8) | nb);
                }}
            }}
        }}
    }}
}}

/* Türkçe karakter genişliği */
int {output_name}_turkish_char_width(int tr_index) {{
    if(tr_index < 0 || tr_index >= {macro_prefix}_TURKISH_COUNT) return 0;
    return {output_name}_turkish_glyphs[tr_index].width;
}}

/* Metin çiz (UTF-8 ve Türkçe destekli) */
void {output_name}_draw_text(int x, int y, const char* text, uint32_t color) {{
    int cx = x;
    while(*text) {{
        unsigned char c = (unsigned char)*text;

        /* UTF-8 multi-byte kontrolü */
        if(c >= 0x80) {{
            int tr_idx = {output_name}_utf8_to_turkish(&text);
            text++;
            if(tr_idx >= 0) {{
                {output_name}_draw_turkish_char(cx, y, tr_idx, color);
                cx += {output_name}_turkish_glyphs[tr_idx].width;
            }} else {{
                cx += {output_name}_glyphs[0].width;
            }}
            continue;
        }}

        text++;
        if(c < {macro_prefix}_FIRST_CHAR || c > {macro_prefix}_LAST_CHAR) {{
            cx += {output_name}_glyphs[0].width;
            continue;
        }}
        {output_name}_draw_char(cx, y, (char)c, color);
        cx += {output_name}_glyphs[c - {macro_prefix}_FIRST_CHAR].width;
    }}
}}

/* Metin genişliğini hesapla (UTF-8 ve Türkçe destekli) */
int {output_name}_text_width(const char* text) {{
    int width = 0;
    while(*text) {{
        unsigned char c = (unsigned char)*text;

        if(c >= 0x80) {{
            int tr_idx = {output_name}_utf8_to_turkish(&text);
            text++;
            if(tr_idx >= 0) {{
                width += {output_name}_turkish_glyphs[tr_idx].width;
            }} else {{
                width += {output_name}_glyphs[0].width;
            }}
            continue;
        }}

        text++;
        if(c < {macro_prefix}_FIRST_CHAR || c > {macro_prefix}_LAST_CHAR) {{
            width += {output_name}_glyphs[0].width;
            continue;
        }}
        width += {output_name}_glyphs[c - {macro_prefix}_FIRST_CHAR].width;
    }}
    return width;
}}
"""

    return header, c_code

def generate_unified_header(variants_info):
    """Tüm fontları birleştiren header oluştur"""

    header = """/* fonts.h - Unified font system for EmConOs */
#ifndef FONTS_H
#define FONTS_H

#include "types.h"

/* Tüm font varyantlarını dahil et */
"""

    for name, _, size, desc in variants_info:
        header += f'#include "{name}.h"\n'

    header += """
/* Font boyutları */
typedef enum {
    FONT_SIZE_16 = 16,
    FONT_SIZE_20 = 20,
    FONT_SIZE_24 = 24,
    FONT_SIZE_32 = 32
} FontSize;

/* Font kalınlıkları */
typedef enum {
    FONT_WEIGHT_REGULAR,
    FONT_WEIGHT_MEDIUM,
    FONT_WEIGHT_BOLD
} FontWeight;

/* Kolay kullanım makroları - varsayılan olarak medium kullan */

/* 16px fontlar */
#define draw_text_16(x, y, text, color)         font_inter_16_medium_draw_text(x, y, text, color)
#define draw_text_16_regular(x, y, text, color) font_inter_16_draw_text(x, y, text, color)
#define draw_text_16_medium(x, y, text, color)  font_inter_16_medium_draw_text(x, y, text, color)
#define draw_text_16_bold(x, y, text, color)    font_inter_16_bold_draw_text(x, y, text, color)
#define text_width_16(text)                      font_inter_16_medium_text_width(text)

/* 20px fontlar */
#define draw_text_20(x, y, text, color)         font_inter_20_medium_draw_text(x, y, text, color)
#define draw_text_20_regular(x, y, text, color) font_inter_20_draw_text(x, y, text, color)
#define draw_text_20_medium(x, y, text, color)  font_inter_20_medium_draw_text(x, y, text, color)
#define draw_text_20_bold(x, y, text, color)    font_inter_20_bold_draw_text(x, y, text, color)
#define text_width_20(text)                      font_inter_20_medium_text_width(text)

/* 24px fontlar */
#define draw_text_24(x, y, text, color)         font_inter_24_medium_draw_text(x, y, text, color)
#define draw_text_24_regular(x, y, text, color) font_inter_24_draw_text(x, y, text, color)
#define draw_text_24_medium(x, y, text, color)  font_inter_24_medium_draw_text(x, y, text, color)
#define draw_text_24_bold(x, y, text, color)    font_inter_24_bold_draw_text(x, y, text, color)
#define text_width_24(text)                      font_inter_24_medium_text_width(text)

/* 32px fontlar */
#define draw_text_32(x, y, text, color)         font_inter_32_medium_draw_text(x, y, text, color)
#define draw_text_32_regular(x, y, text, color) font_inter_32_draw_text(x, y, text, color)
#define draw_text_32_medium(x, y, text, color)  font_inter_32_medium_draw_text(x, y, text, color)
#define draw_text_32_bold(x, y, text, color)    font_inter_32_bold_draw_text(x, y, text, color)
#define text_width_32(text)                      font_inter_32_medium_text_width(text)

/* Font yükseklikleri */
#define FONT_HEIGHT_16 FONT_INTER_16_MEDIUM_HEIGHT
#define FONT_HEIGHT_20 FONT_INTER_20_MEDIUM_HEIGHT
#define FONT_HEIGHT_24 FONT_INTER_24_MEDIUM_HEIGHT
#define FONT_HEIGHT_32 FONT_INTER_32_MEDIUM_HEIGHT

#endif
"""

    return header

def main():
    script_dir = Path(__file__).parent
    os.chdir(script_dir)

    print("=== Inter Font Generator ===")
    print(f"Varyant sayısı: {len(FONT_VARIANTS)}")
    print()

    # Fontları indir
    download_fonts()
    print()

    # Her varyantı oluştur
    for output_name, font_file, font_size, description in FONT_VARIANTS:
        print(f"Oluşturuluyor: {description}...")

        if not os.path.exists(font_file):
            print(f"  HATA: {font_file} bulunamadı!")
            continue

        ascii_glyphs, turkish_glyphs, font_height = render_font(font_file, font_size)
        header, c_code = generate_c_code(ascii_glyphs, turkish_glyphs, font_height, output_name, description)

        # Dosyaları yaz (include/fonts ve src/fonts klasörlerine)
        os.makedirs("../include/fonts", exist_ok=True)
        os.makedirs("../src/fonts", exist_ok=True)
        with open(f"../include/fonts/{output_name}.h", 'w') as f:
            f.write(header)

        with open(f"../src/fonts/{output_name}.c", 'w') as f:
            f.write(c_code)

        print(f"  -> include/fonts/{output_name}.h, src/fonts/{output_name}.c ({font_height}px height)")

    # Unified header oluştur
    print()
    print("Unified header oluşturuluyor...")
    unified = generate_unified_header(FONT_VARIANTS)
    with open("../include/fonts/fonts.h", 'w') as f:
        f.write(unified)
    print("  -> include/fonts/fonts.h")

    print()
    print("Tamamlandı!")
    print()
    print("Kullanım örnekleri:")
    print('  #include "fonts.h"')
    print('  draw_text_20(10, 10, "Hello", COLOR_WHITE);       // 20px medium')
    print('  draw_text_32_bold(10, 50, "Title", COLOR_WHITE);  // 32px bold')
    print('  int w = text_width_24("Test");                    // Genişlik hesapla')

if __name__ == "__main__":
    main()
