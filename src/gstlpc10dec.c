#include "gstlpc10dec.h"
#include "gstlpc10_util.h"  // Include utility header
#include "lpc10.h"          // Include LPC10 header
#include <gst/gst.h>
#include <gst/audio/gstaudiodecoder.h>
#include <gst/audio/audio.h>       // General audio utilities
#include <gst/audio/audio-info.h>  // Explicit for GstAudioInfo functions
#include <string.h>                // Required for memset

GST_DEBUG_CATEGORY_STATIC(gst_lpc10_dec_debug_category);
#define GST_CAT_DEFAULT gst_lpc10_dec_debug_category

#include "gstlpc10_macros.h"  // Include macros header

#define LPC10_FRAME_SIZE_BYTES ((LPC10_BITS_IN_COMPRESSED_FRAME + 7) / 8)  // 7 bytes
#define LPC10_SAMPLES_OUT (LPC10_SAMPLES_PER_FRAME)                        // 180 samples

/* Forward declarations for our static functions */
static void gst_lpc10_dec_init(GstLpc10Dec* dec);
static void gst_lpc10_dec_class_init(GstLpc10DecClass* klass);
static void gst_lpc10_dec_dispose(GObject* object);
static void gst_lpc10_dec_finalize(GObject* object);
static gboolean gst_lpc10_dec_start(GstAudioDecoder* dec);
static gboolean gst_lpc10_dec_stop(GstAudioDecoder* dec);
static gboolean gst_lpc10_dec_set_format(GstAudioDecoder* dec, GstCaps* caps);
static GstFlowReturn gst_lpc10_dec_parse(GstAudioDecoder* dec, GstAdapter* adapter, gint* offset, gint* length);
static GstFlowReturn gst_lpc10_dec_handle_frame(GstAudioDecoder* dec, GstBuffer* inbuf);

/* GType registration */
G_DEFINE_TYPE(GstLpc10Dec, gst_lpc10_dec, GST_TYPE_AUDIO_DECODER)

/* Class initialization function */
static void gst_lpc10_dec_class_init(GstLpc10DecClass* klass) {
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass* element_class = GST_ELEMENT_CLASS(klass);
    GstAudioDecoderClass* audio_decoder_class = GST_AUDIO_DECODER_CLASS(klass);

    GST_DEBUG_CATEGORY_INIT(gst_lpc10_dec_debug_category, "lpc10dec", 0, "LPC10 decoder element");

    gobject_class->dispose = gst_lpc10_dec_dispose;
    gobject_class->finalize = gst_lpc10_dec_finalize;

    gst_element_class_set_static_metadata(element_class, "LPC10 Decoder", "Codec/Decoder/Audio", "LPC10 audio decoder",
                                          "Emin xeome@proton.me");

    // Sink pad template: LPC10 bitstream input
    GstCaps* sink_caps = gst_caps_new_simple("application/x-lpc10", "framerate", GST_TYPE_FRACTION, 8000, LPC10_SAMPLES_PER_FRAME,
                                             "frame-size", G_TYPE_INT, LPC10_FRAME_SIZE_BYTES, NULL);
    GstPadTemplate* sink_template = gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, sink_caps);
    gst_element_class_add_pad_template(element_class, sink_template);
    gst_caps_unref(sink_caps);

    // Source pad template: Raw audio output
    GstCaps* src_caps = gst_caps_new_simple("audio/x-raw", "format", G_TYPE_STRING, "S16LE", "layout", G_TYPE_STRING,
                                            "interleaved", "rate", G_TYPE_INT, 8000, "channels", G_TYPE_INT, 1, NULL);
    GstPadTemplate* src_template = gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, src_caps);
    gst_element_class_add_pad_template(element_class, src_template);
    gst_caps_unref(src_caps);

    audio_decoder_class->start = GST_DEBUG_FUNCPTR(gst_lpc10_dec_start);
    audio_decoder_class->stop = GST_DEBUG_FUNCPTR(gst_lpc10_dec_stop);
    audio_decoder_class->set_format = GST_DEBUG_FUNCPTR(gst_lpc10_dec_set_format);
    audio_decoder_class->parse = GST_DEBUG_FUNCPTR(gst_lpc10_dec_parse);
    audio_decoder_class->handle_frame = GST_DEBUG_FUNCPTR(gst_lpc10_dec_handle_frame);
}

/* Instance initialization function */
static void gst_lpc10_dec_init(GstLpc10Dec* dec) {
    dec->lpc10_state = NULL;
    gst_audio_decoder_set_needs_format(GST_AUDIO_DECODER(dec), TRUE);
    gst_audio_decoder_set_use_default_pad_acceptcaps(GST_AUDIO_DECODER(dec), TRUE);
    GST_PAD_SET_ACCEPT_TEMPLATE(GST_AUDIO_DECODER_SINK_PAD(dec));
}

static void gst_lpc10_dec_dispose(GObject* object) {
    GstLpc10Dec* dec = GST_LPC10_DEC(object);
    GST_DEBUG_OBJECT(dec, "dispose");
    if (dec->lpc10_state) {
        g_free(dec->lpc10_state);
        dec->lpc10_state = NULL;
    }
    G_OBJECT_CLASS(gst_lpc10_dec_parent_class)->dispose(object);
}

static void gst_lpc10_dec_finalize(GObject* object) {
    GstLpc10Dec* dec = GST_LPC10_DEC(object);
    GST_DEBUG_OBJECT(dec, "finalize");
    G_OBJECT_CLASS(gst_lpc10_dec_parent_class)->finalize(object);
}

static gboolean gst_lpc10_dec_start(GstAudioDecoder* audio_dec) {
    GstLpc10Dec* dec = GST_LPC10_DEC(audio_dec);
    GST_DEBUG_OBJECT(dec, "start");
    if (dec->lpc10_state) {  // Should be NULL or freed in stop
        g_free(dec->lpc10_state);
    }
    dec->lpc10_state = g_malloc0(sizeof(struct lpc10_decoder_state));
    if (!dec->lpc10_state) {
        GST_ERROR_OBJECT(dec, "Failed to allocate LPC10 decoder state");
        return FALSE;
    }
    init_lpc10_decoder_state(dec->lpc10_state);
    return TRUE;
}

static gboolean gst_lpc10_dec_stop(GstAudioDecoder* audio_dec) {
    GstLpc10Dec* dec = GST_LPC10_DEC(audio_dec);
    GST_DEBUG_OBJECT(dec, "stop");
    if (dec->lpc10_state) {
        g_free(dec->lpc10_state);
        dec->lpc10_state = NULL;
    }
    return TRUE;
}

static gboolean gst_lpc10_dec_set_format(GstAudioDecoder* audio_dec, GstCaps* incaps) {
    GstLpc10Dec* dec = GST_LPC10_DEC(audio_dec);
    GstAudioInfo info;
    GstStructure* s;

    GST_DEBUG_OBJECT(dec, "Setting format from input caps: %" GST_PTR_FORMAT, (void*)incaps);

    s = gst_caps_get_structure(incaps, 0);
    if (s == NULL) {
        GST_ERROR_OBJECT(dec, "Failed to get structure from input caps");
        return FALSE;
    }
    if (!gst_structure_has_name(s, "application/x-lpc10")) {
        GST_ERROR_OBJECT(dec, "Invalid input caps name: %s. Expected application/x-lpc10", gst_structure_get_name(s));
        return FALSE;
    }
    // Further validation of incaps fields (framerate, frame-size) can be added if necessary

    gst_audio_info_init(&info);
    gst_audio_info_set_format(&info, GST_AUDIO_FORMAT_S16LE, 8000, 1, NULL);

    if (!gst_audio_decoder_set_output_format(audio_dec, &info)) {
        GST_ERROR_OBJECT(dec, "Failed to set output audio format");
        return FALSE;
    }
    GST_DEBUG_OBJECT(dec, "Output format set successfully");
    return TRUE;
}

static GstFlowReturn gst_lpc10_dec_parse(GstAudioDecoder* audio_dec, GstAdapter* adapter, gint* offset, gint* length) {
    guint available_data;

    available_data = gst_adapter_available(adapter);

    if (available_data < LPC10_FRAME_SIZE_BYTES) {
        GST_LOG_OBJECT(audio_dec, "Not enough data, available %u, needed %d", available_data, LPC10_FRAME_SIZE_BYTES);
        return GST_FLOW_EOS;  // GstAudioDecoder handles this based on upstream EOS
    }

    *offset = 0;
    *length = LPC10_FRAME_SIZE_BYTES;

    GST_LOG_OBJECT(audio_dec, "Parsed one frame, length %d", *length);
    return GST_FLOW_OK;
}

static GstFlowReturn gst_lpc10_dec_handle_frame(GstAudioDecoder* audio_dec, GstBuffer* inbuf) {
    GstLpc10Dec* dec = GST_LPC10_DEC(audio_dec);
    GstMapInfo in_map, out_map;
    const guint8* in_data;
    gint16* out_data_s16;
    real speech_float[LPC10_SAMPLES_OUT];
    INT32 lpc10_bits[LPC10_BITS_IN_COMPRESSED_FRAME];
    gint i;
    GstBuffer* outbuf;
    GstFlowReturn ret = GST_FLOW_OK;

    if (G_UNLIKELY(inbuf == NULL)) {
        GST_DEBUG_OBJECT(dec, "Received NULL buffer in handle_frame, signaling EOS.");
        return GST_FLOW_EOS;  // Let GstAudioDecoder base class handle EOS finalization
    }

    if (!gst_buffer_map(inbuf, &in_map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(dec, "Failed to map input buffer");
        return GST_FLOW_ERROR;
    }

    if (in_map.size < LPC10_FRAME_SIZE_BYTES) {
        GST_ERROR_OBJECT(dec, "Input buffer too small: %" G_GSIZE_FORMAT " bytes, expected %d", in_map.size,
                         LPC10_FRAME_SIZE_BYTES);
        gst_buffer_unmap(inbuf, &in_map);
        return GST_FLOW_ERROR;
    }
    in_data = (const guint8*)in_map.data;

    // Unpack the 54 bits from the 7-byte input buffer into the INT32 array.
    for (i = 0; i < LPC10_BITS_IN_COMPRESSED_FRAME; ++i) {
        int byte_index = i / 8;
        int bit_index_in_byte = i % 8;
        if ((in_data[byte_index] >> bit_index_in_byte) & 1) {
            lpc10_bits[i] = 1;
        } else {
            lpc10_bits[i] = 0;
        }
    }

    // Call the LPC10 decode function
    lpc10_decode(lpc10_bits, speech_float, dec->lpc10_state);

    // Allocate output buffer
    outbuf = gst_buffer_new_allocate(NULL, LPC10_SAMPLES_OUT * sizeof(gint16), NULL);
    if (!outbuf) {
        GST_ERROR_OBJECT(dec, "Failed to allocate output buffer");
        gst_buffer_unmap(inbuf, &in_map);
        return GST_FLOW_ERROR;
    }

    if (!gst_buffer_map(outbuf, &out_map, GST_MAP_WRITE)) {
        GST_ERROR_OBJECT(dec, "Failed to map output buffer");
        gst_buffer_unmap(inbuf, &in_map);
        gst_buffer_unref(outbuf);
        return GST_FLOW_ERROR;
    }
    out_data_s16 = (gint16*)out_map.data;

    // Convert LPC10 real (float) output to S16_LE format
    for (i = 0; i < LPC10_SAMPLES_OUT; ++i) {
        real val = speech_float[i] * 32768.0f;
        if (val > 32767.0f)
            val = 32767.0f;
        else if (val < -32768.0f)
            val = -32768.0f;
        out_data_s16[i] = (gint16)val;
    }

    gst_buffer_unmap(inbuf, &in_map);
    gst_buffer_unmap(outbuf, &out_map);

    ret = gst_audio_decoder_finish_frame(audio_dec, outbuf, 1);  // 1 frame processed
    return ret;
}
