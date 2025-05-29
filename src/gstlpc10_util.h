#ifndef __GST_LPC10_UTIL_H__
#define __GST_LPC10_UTIL_H__

#include <gst/gst.h>
#include "lpc10.h"  // For real type and constants (included from lpc10 dir)

G_BEGIN_DECLS

/**
 * @brief Converts an array of S16_LE samples to an array of float samples.
 *
 * @param in Input array of S16_LE samples.
 * @param out Output array of float samples (real type).
 * @param num_samples The number of samples to convert.
 */
void s16le_to_float(const gint16* in, real* out, int num_samples);

/**
 * @brief Converts an array of float samples to an array of S16_LE samples.
 *
 * @param in Input array of float samples (real type).
 * @param out Output array of S16_LE samples.
 * @param num_samples The number of samples to convert.
 */
void float_to_s16le(const real* in, gint16* out, int num_samples);

/**
 * @brief Packs an array of bits (stored as 0 or 1 in gint32) into a byte array.
 *
 * @param bits_in Input array of bits (0 or 1).
 * @param bytes_out Output byte array.
 * @param num_bits The number of bits to pack.
 */
void pack_bits(const gint32* bits_in, guint8* bytes_out, int num_bits);

/**
 * @brief Unpacks a byte array into an array of bits (stored as 0 or 1 in gint32).
 *
 * @param bytes_in Input byte array.
 * @param bits_out Output array of bits (0 or 1).
 * @param num_bits The number of bits to unpack.
 */
void unpack_bits(const guint8* bytes_in, gint32* bits_out, int num_bits);

G_END_DECLS

#endif /* __GST_LPC10_UTIL_H__ */
