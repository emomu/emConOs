/* png.h - PNG Decoder header */
#ifndef PNG_H
#define PNG_H

#include <stdint.h>

/* PNG decode function
 * data: raw PNG file data
 * data_len: length of PNG data
 * pixels: output buffer for RGBA pixels (must be max_width * max_height * 4 bytes)
 * max_width, max_height: maximum image dimensions
 * out_width, out_height: actual image dimensions (output)
 * Returns: 0 on success, -1 on error
 */
int png_decode(const uint8_t *data, uint32_t data_len,
               uint8_t *pixels, int max_width, int max_height,
               int *out_width, int *out_height);

#endif /* PNG_H */
