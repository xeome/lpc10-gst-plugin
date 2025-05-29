#include "gstlpc10_util.h"
#include "lpc10.h"  // Include LPC10 header for real type and constants
#include <glib.h>
#include <math.h>  // For roundf

/**
 * @brief Converts an array of S16_LE samples to an array of float samples.
 *
 * @param in Input array of S16_LE samples.
 * @param out Output array of float samples (real type).
 * @param num_samples The number of samples to convert.
 */
void s16le_to_float(const gint16* in, real* out, int num_samples) {
    for (int i = 0; i < num_samples; ++i) {
        // Assuming S16_LE samples are in range [-32768, 32767]
        // Convert to float in range [-1.0, 1.0]
        out[i] = (real)in[i] / 32768.0f;
    }
}

/**
 * @brief Converts an array of float samples to an array of S16_LE samples.
 *
 * @param in Input array of float samples (real type).
 * @param out Output array of S16_LE samples.
 * @param num_samples The number of samples to convert.
 */
void float_to_s16le(const real* in, gint16* out, int num_samples) {
    for (int i = 0; i < num_samples; ++i) {
        // Convert float in range [-1.0, 1.0] to S16_LE in range [-32768, 32767]
        // Clamp and round the result
        real sample = in[i] * 32768.0f;
        if (sample > 32767.0f) {
            out[i] = 32767;
        } else if (sample < -32768.0f) {
            out[i] = -32768;
        } else {
            out[i] = (gint16)roundf(sample);
        }
    }
}

/**
 * @brief Packs an array of bits (stored as 0 or 1 in gint32) into a byte array.
 *
 * Based on analysis of lpc10/chanwr.c. The bits are packed sequentially
 * into bytes, least significant bit first within each byte.
 *
 * @param bits_in Input array of bits (0 or 1).
 * @param bytes_out Output byte array.
 * @param num_bits The number of bits to pack.
 */
void pack_bits(const gint32* bits_in, guint8* bytes_out, int num_bits) {
    g_assert(num_bits % 8 == 6);                         // LPC10_BITS_IN_COMPRESSED_FRAME is 54
    int num_bytes = num_bits / 8 + (num_bits % 8 != 0);  // 54 bits = 7 bytes

    for (int i = 0; i < num_bytes; ++i) {
        bytes_out[i] = 0;
        for (int j = 0; j < 8; ++j) {
            int bit_index = i * 8 + j;
            if (bit_index < num_bits) {
                if (bits_in[bit_index] & 1) {
                    bytes_out[i] |= (1 << j);
                }
            }
        }
    }
}

/**
 * @brief Unpacks a byte array into an array of bits (stored as 0 or 1 in gint32).
 *
 * Based on analysis of lpc10/chanwr.c. The bits are unpacked sequentially
 * from bytes, least significant bit first within each byte.
 *
 * @param bytes_in Input byte array.
 * @param bits_out Output array of bits (0 or 1).
 * @param num_bits The number of bits to unpack.
 */
void unpack_bits(const guint8* bytes_in, gint32* bits_out, int num_bits) {
    g_assert(num_bits % 8 == 6);                         // LPC10_BITS_IN_COMPRESSED_FRAME is 54
    int num_bytes = num_bits / 8 + (num_bits % 8 != 0);  // 54 bits = 7 bytes

    for (int i = 0; i < num_bytes; ++i) {
        for (int j = 0; j < 8; ++j) {
            int bit_index = i * 8 + j;
            if (bit_index < num_bits) {
                bits_out[bit_index] = (bytes_in[i] >> j) & 1;
            }
        }
    }
}
