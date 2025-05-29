#include "gstlpc10enc.h"
#include "gstlpc10_util.h"  // Include utility header
#include "lpc10.h"          // Include LPC10 header
#include <gst/gst.h>
#include <gst/audio/gstaudioencoder.h>
#include <gst/audio/audio.h>       // General audio utilities
#include <gst/audio/audio-info.h>  // Explicit for GstAudioInfo functions
#include <string.h>                // Required for memcpy, memset

GST_DEBUG_CATEGORY_STATIC(gst_lpc10_enc_debug_category);
#define GST_CAT_DEFAULT gst_lpc10_enc_debug_category

#include "gstlpc10_macros.h"  // Include macros header

/* Define GstLpc10Enc private structure if G_ADD_PRIVATE is used,
 * or ensure GstLpc10Enc itself in gstlpc10enc.h has the members.
 * For simplicity here, we assume members are directly in GstLpc10Enc.
 * gstlpc10enc.h should have:
 * struct _GstLpc10Enc {
 *   GstAudioEncoder parent_instance;
 *   struct lpc10_encoder_state *lpc10_state;
 * };
 */

/* Forward declarations for our static functions */
static void gst_lpc10_enc_init(GstLpc10Enc* enc);
static void gst_lpc10_enc_class_init(GstLpc10EncClass* klass);
static void gst_lpc10_enc_dispose(GObject* object);
static void gst_lpc10_enc_finalize(GObject* object);  // Added for completeness
static gboolean gst_lpc10_enc_start(GstAudioEncoder* enc);
static gboolean gst_lpc10_enc_stop(GstAudioEncoder* enc);
static gboolean gst_lpc10_enc_set_format(GstAudioEncoder* enc, GstAudioInfo* info);
static GstFlowReturn gst_lpc10_enc_handle_frame(GstAudioEncoder* enc, GstBuffer* buffer);

/* GType registration */
G_DEFINE_TYPE(GstLpc10Enc, gst_lpc10_enc, GST_TYPE_AUDIO_ENCODER)

/* Class initialization function */
static void gst_lpc10_enc_class_init(GstLpc10EncClass* klass) {
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass* element_class = GST_ELEMENT_CLASS(klass);
    GstAudioEncoderClass* audio_encoder_class = GST_AUDIO_ENCODER_CLASS(klass);

    GST_DEBUG_CATEGORY_INIT(gst_lpc10_enc_debug_category, "lpc10enc", 0, "LPC10 encoder element");

    gobject_class->dispose = gst_lpc10_enc_dispose;
    gobject_class->finalize = gst_lpc10_enc_finalize;

    gst_element_class_set_static_metadata(element_class, "LPC10 Encoder", "Codec/Encoder/Audio", "LPC10 audio encoder",
                                          "Emin xeome@proton.me");

    // Sink pad template: Raw audio input
    GstCaps* sink_caps = gst_caps_new_simple("audio/x-raw", "format", G_TYPE_STRING, "S16LE", "layout", G_TYPE_STRING,
                                             "interleaved", "rate", G_TYPE_INT, 8000, "channels", G_TYPE_INT, 1, NULL);
    GstPadTemplate* sink_template = gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, sink_caps);
    gst_element_class_add_pad_template(element_class, sink_template);
    gst_caps_unref(sink_caps);

    // Source pad template: LPC10 bitstream output
    GstCaps* src_caps = gst_caps_new_simple("application/x-lpc10", "framerate", GST_TYPE_FRACTION, 8000, LPC10_SAMPLES_PER_FRAME,
                                            "frame-size", G_TYPE_INT, (LPC10_BITS_IN_COMPRESSED_FRAME + 7) / 8, NULL);
    GstPadTemplate* src_template = gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, src_caps);
    gst_element_class_add_pad_template(element_class, src_template);
    gst_caps_unref(src_caps);

    audio_encoder_class->start = GST_DEBUG_FUNCPTR(gst_lpc10_enc_start);
    audio_encoder_class->stop = GST_DEBUG_FUNCPTR(gst_lpc10_enc_stop);
    audio_encoder_class->set_format = GST_DEBUG_FUNCPTR(gst_lpc10_enc_set_format);
    audio_encoder_class->handle_frame = GST_DEBUG_FUNCPTR(gst_lpc10_enc_handle_frame);

    // Set latency based on one frame
    // gst_audio_encoder_class_set_max_latency(audio_encoder_class,
    //                                         gst_util_uint64_scale_int(LPC10_SAMPLES_PER_FRAME, GST_SECOND, 8000));
}

/* Instance initialization function */
static void gst_lpc10_enc_init(GstLpc10Enc* enc) {
    // GstLpc10Enc *enc = GST_LPC10_ENC (self); // Not needed with G_DEFINE_TYPE direct init
    enc->lpc10_state = NULL;
    // Set sink pad to accept template caps by default
    GST_PAD_SET_ACCEPT_TEMPLATE(GST_AUDIO_ENCODER_SINK_PAD(enc));
}

static void gst_lpc10_enc_dispose(GObject* object) {
    GstLpc10Enc* enc = GST_LPC10_ENC(object);

    GST_DEBUG_OBJECT(enc, "dispose");

    if (enc->lpc10_state) {
        g_free(enc->lpc10_state);
        enc->lpc10_state = NULL;
    }
    G_OBJECT_CLASS(gst_lpc10_enc_parent_class)->dispose(object);
}

// Finalize function
static void gst_lpc10_enc_finalize(GObject* object) {
    GstLpc10Enc* enc = GST_LPC10_ENC(object);
    GST_DEBUG_OBJECT(enc, "finalize");
    // Free any other resources if necessary, though lpc10_state is handled in dispose
    G_OBJECT_CLASS(gst_lpc10_enc_parent_class)->finalize(object);
}

static gboolean gst_lpc10_enc_start(GstAudioEncoder* audio_enc) {
    GstLpc10Enc* enc = GST_LPC10_ENC(audio_enc);

    GST_DEBUG_OBJECT(enc, "start");

    // Ensure state is clean if start is called multiple times (e.g. after a stop)
    if (enc->lpc10_state) {
        g_free(enc->lpc10_state);
        enc->lpc10_state = NULL;
    }

    enc->lpc10_state = g_malloc0(sizeof(struct lpc10_encoder_state));
    if (!enc->lpc10_state) {
        GST_ERROR_OBJECT(enc, "Failed to allocate LPC10 encoder state");
        return FALSE;
    }
    init_lpc10_encoder_state(enc->lpc10_state);
    return TRUE;
}

static gboolean gst_lpc10_enc_stop(GstAudioEncoder* audio_enc) {
    GstLpc10Enc* enc = GST_LPC10_ENC(audio_enc);

    GST_DEBUG_OBJECT(enc, "stop");

    if (enc->lpc10_state) {
        g_free(enc->lpc10_state);
        enc->lpc10_state = NULL;
    }
    return TRUE;
}

static gboolean gst_lpc10_enc_set_format(GstAudioEncoder* audio_enc, GstAudioInfo* info) {
    GstLpc10Enc* enc = GST_LPC10_ENC(audio_enc);
    GstCaps* outcaps;

    GST_DEBUG_OBJECT(enc, "set_format: rate %d, channels %d, format %s", GST_AUDIO_INFO_RATE(info), GST_AUDIO_INFO_CHANNELS(info),
                     gst_audio_format_to_string(GST_AUDIO_INFO_FORMAT(info)));

    // Validate input format
    if (GST_AUDIO_INFO_FORMAT(info) != GST_AUDIO_FORMAT_S16LE &&
        GST_AUDIO_INFO_FORMAT(info) != GST_AUDIO_FORMAT_S16) {  // S16 for native endian
        GST_ERROR_OBJECT(enc, "Unsupported audio format: %s. Expected S16LE or S16.",
                         gst_audio_format_to_string(GST_AUDIO_INFO_FORMAT(info)));
        return FALSE;
    }
    if (GST_AUDIO_INFO_RATE(info) != 8000) {
        GST_ERROR_OBJECT(enc, "Unsupported sample rate: %d. Expected 8000 Hz.", GST_AUDIO_INFO_RATE(info));
        return FALSE;
    }
    if (GST_AUDIO_INFO_CHANNELS(info) != 1) {
        GST_ERROR_OBJECT(enc, "Unsupported channel count: %d. Expected 1 (mono).", GST_AUDIO_INFO_CHANNELS(info));
        return FALSE;
    }

    // Define output capabilities
    outcaps = gst_caps_new_simple("application/x-lpc10", "framerate", GST_TYPE_FRACTION, 8000, LPC10_SAMPLES_PER_FRAME,
                                  "frame-size", G_TYPE_INT, (LPC10_BITS_IN_COMPRESSED_FRAME + 7) / 8, NULL);
    if (!gst_audio_encoder_set_output_format(audio_enc, outcaps)) {
        gst_caps_unref(outcaps);
        GST_ERROR_OBJECT(enc, "Failed to set output format");
        return FALSE;
    }
    gst_caps_unref(outcaps);

    // Inform base class about framing
    gst_audio_encoder_set_frame_samples_min(audio_enc, LPC10_SAMPLES_PER_FRAME);
    gst_audio_encoder_set_frame_samples_max(audio_enc, LPC10_SAMPLES_PER_FRAME);
    gst_audio_encoder_set_frame_max(audio_enc, 1);  // Each input frame produces one output buffer

    return TRUE;
}

static GstFlowReturn gst_lpc10_enc_handle_frame(GstAudioEncoder* audio_enc, GstBuffer* inbuf) {
    GstLpc10Enc* enc = GST_LPC10_ENC(audio_enc);
    GstMapInfo in_map, out_map;
    gint16* in_data;
    guint8* out_data;
    real speech_float[LPC10_SAMPLES_PER_FRAME];
    INT32 lpc10_bits[LPC10_BITS_IN_COMPRESSED_FRAME];
    gint i;
    GstBuffer* outbuf;
    GstFlowReturn ret = GST_FLOW_OK;

    if (G_UNLIKELY(inbuf == NULL)) {
        // This typically means drain/EOS handling by the base class,
        // but GstAudioEncoder usually provides a valid buffer or calls finish().
        // If we get NULL here, it's unusual for handle_frame.
        GST_DEBUG_OBJECT(enc, "Received NULL buffer in handle_frame, finishing.");
        return gst_audio_encoder_finish_frame(audio_enc, NULL, 0);
    }

    if (!gst_buffer_map(inbuf, &in_map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(enc, "Failed to map input buffer");
        return GST_FLOW_ERROR;
    }

    if (in_map.size < (LPC10_SAMPLES_PER_FRAME * sizeof(gint16))) {
        GST_ERROR_OBJECT(enc, "Input buffer too small: %" G_GSIZE_FORMAT " bytes, expected %d", in_map.size,
                         (int)(LPC10_SAMPLES_PER_FRAME * sizeof(gint16)));
        gst_buffer_unmap(inbuf, &in_map);
        return GST_FLOW_ERROR;  // Or perhaps just drop and ask for more?
    }

    in_data = (gint16*)in_map.data;

    // Convert S16_LE input to LPC10 real (float) format
    for (i = 0; i < LPC10_SAMPLES_PER_FRAME; ++i) {
        speech_float[i] = (real)in_data[i] / 32768.0f;
    }

    // Call the LPC10 encode function
    lpc10_encode(speech_float, lpc10_bits, enc->lpc10_state);

    // Allocate output buffer for 54 bits (7 bytes)
    outbuf = gst_buffer_new_allocate(NULL, (LPC10_BITS_IN_COMPRESSED_FRAME + 7) / 8, NULL);
    if (!outbuf) {
        GST_ERROR_OBJECT(enc, "Failed to allocate output buffer");
        gst_buffer_unmap(inbuf, &in_map);
        return GST_FLOW_ERROR;
    }

    if (!gst_buffer_map(outbuf, &out_map, GST_MAP_WRITE)) {
        GST_ERROR_OBJECT(enc, "Failed to map output buffer");
        gst_buffer_unmap(inbuf, &in_map);
        gst_buffer_unref(outbuf);
        return GST_FLOW_ERROR;
    }
    out_data = (guint8*)out_map.data;
    memset(out_data, 0, out_map.size);

    // Pack the 54 bits from the INT32 array into the 7-byte output buffer.
    for (i = 0; i < LPC10_BITS_IN_COMPRESSED_FRAME; ++i) {
        if (lpc10_bits[i] != 0) {
            int byte_index = i / 8;
            int bit_index_in_byte = i % 8;
            out_data[byte_index] |= (1 << bit_index_in_byte);
        }
    }

    gst_buffer_unmap(inbuf, &in_map);
    gst_buffer_unmap(outbuf, &out_map);

    // Timestamps and duration are typically handled by GstAudioEncoder based on
    // input buffer and samples consumed.
    ret = gst_audio_encoder_finish_frame(audio_enc, outbuf, LPC10_SAMPLES_PER_FRAME);

    return ret;
}
