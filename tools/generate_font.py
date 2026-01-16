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

# Karakter aralığı (ASCII 32-126)
CHAR_START = 32
CHAR_END = 127
CHAR_COUNT = CHAR_END - CHAR_START

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

def render_font(font_file, font_size):
    """Fontu render et"""
    font = ImageFont.truetype(font_file, font_size)

    # Ascent ve descent hesapla
    ascent, descent = font.getmetrics()
    font_height = ascent + descent

    # Her karakteri render et
    glyphs = []

    for code in range(CHAR_START, CHAR_END):
        char = chr(code)

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

        glyphs.append({
            'char': char,
            'code': code,
            'width': advance,
            'height': font_height,
            'pixels': pixels
        })

    return glyphs, font_height

def generate_c_code(glyphs, font_height, output_name, description):
    """C kodu oluştur"""

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
#define {macro_prefix}_CHAR_COUNT {CHAR_COUNT}

/* Karakter bilgisi */
typedef struct {{
    uint8_t width;      /* Karakter genişliği */
    uint32_t offset;    /* Piksel verisinin başlangıç offseti */
}} {output_name}_Glyph;

/* Font verileri */
extern const {output_name}_Glyph {output_name}_glyphs[{CHAR_COUNT}];
extern const uint8_t {output_name}_pixels[];

/* Karakter çizme fonksiyonu */
void {output_name}_draw_char(int x, int y, char c, uint32_t color);
void {output_name}_draw_text(int x, int y, const char* text, uint32_t color);
int {output_name}_text_width(const char* text);

#endif
"""

    # C dosyası
    c_code = f"""/* {output_name}.c - {description} */
#include "{output_name}.h"
#include "graphics.h"

/* Glyph bilgileri (genişlik ve offset) */
const {output_name}_Glyph {output_name}_glyphs[{CHAR_COUNT}] = {{
"""

    # Glyph bilgilerini ekle
    offset = 0
    for i, glyph in enumerate(glyphs):
        char_display = glyph['char'] if glyph['code'] > 32 else ' '
        c_code += f"    {{ {glyph['width']:2d}, {offset:5d} }},  /* {glyph['code']:3d}: '{char_display}' */\n"
        offset += glyph['width'] * font_height

    c_code += "};\n\n"

    # Piksel verilerini ekle (kompakt format)
    c_code += "/* Piksel verileri (8-bit alpha) */\n"
    c_code += f"const uint8_t {output_name}_pixels[] = {{\n"

    all_pixels = []
    for glyph in glyphs:
        all_pixels.extend(glyph['pixels'])

    # 20 piksel per satır
    for i in range(0, len(all_pixels), 20):
        chunk = all_pixels[i:i+20]
        c_code += "    " + ",".join(f"0x{p:02X}" for p in chunk) + ",\n"

    c_code += "};\n\n"

    # Çizim fonksiyonları
    c_code += f"""/* Tek karakter çiz */
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
                    /* Alpha blending (arka plan siyah varsayılarak) */
                    uint8_t nr = (r * alpha) >> 8;
                    uint8_t ng = (g * alpha) >> 8;
                    uint8_t nb = (b * alpha) >> 8;
                    draw_pixel(sx, sy, 0xFF000000 | (nr << 16) | (ng << 8) | nb);
                }}
            }}
        }}
    }}
}}

/* Metin çiz */
void {output_name}_draw_text(int x, int y, const char* text, uint32_t color) {{
    int cx = x;
    while(*text) {{
        char c = *text++;
        if(c < {macro_prefix}_FIRST_CHAR || c > {macro_prefix}_LAST_CHAR) {{
            cx += {output_name}_glyphs[0].width;
            continue;
        }}
        {output_name}_draw_char(cx, y, c, color);
        cx += {output_name}_glyphs[c - {macro_prefix}_FIRST_CHAR].width;
    }}
}}

/* Metin genişliğini hesapla */
int {output_name}_text_width(const char* text) {{
    int width = 0;
    while(*text) {{
        char c = *text++;
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

        glyphs, font_height = render_font(font_file, font_size)
        header, c_code = generate_c_code(glyphs, font_height, output_name, description)

        # Dosyaları yaz (fonts/ klasörüne)
        os.makedirs("../fonts", exist_ok=True)
        with open(f"../fonts/{output_name}.h", 'w') as f:
            f.write(header)

        with open(f"../fonts/{output_name}.c", 'w') as f:
            f.write(c_code)

        print(f"  -> {output_name}.h, {output_name}.c ({font_height}px height)")

    # Unified header oluştur
    print()
    print("Unified header oluşturuluyor...")
    unified = generate_unified_header(FONT_VARIANTS)
    with open("../fonts/fonts.h", 'w') as f:
        f.write(unified)
    print("  -> fonts/fonts.h")

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
