#ifndef __GST_LPC10_DEC_H__
#define __GST_LPC10_DEC_H__

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiodecoder.h>  // Changed from gstbasetransform.h
#include <gst/gstbuffer.h>              // Include for GstBuffer
#include "lpc10.h"                      // Corrected include path

G_BEGIN_DECLS

#define GST_TYPE_LPC10_DEC (gst_lpc10_dec_get_type())
#define GST_LPC10_DEC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_LPC10_DEC, GstLpc10Dec))
#define GST_LPC10_DEC_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_LPC10_DEC, GstLpc10DecClass))
#define GST_IS_LPC10_DEC(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_LPC10_DEC))
#define GST_IS_LPC10_DEC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_LPC10_DEC))

typedef struct _GstLpc10Dec GstLpc10Dec;
typedef struct _GstLpc10DecClass GstLpc10DecClass;

struct _GstLpc10Dec {
    GstAudioDecoder parent;  // Corrected parent type

    // Private data for the LPC10 decoder state
    struct lpc10_decoder_state* lpc10_state;

    // Input buffer management for submit_input_buffer/generate_output
    GstBuffer* input_buffer;
    gsize input_buffer_offset;
    gboolean eos_received;

    GstClockTime current_input_timestamp;   // Store the timestamp of the current input buffer
    GstClockTime current_output_timestamp;  // Store/calculate the timestamp for the next output buffer

    // Add other instance variables here as needed
};

struct _GstLpc10DecClass {
    GstAudioDecoderClass parent_class;  // Corrected parent class type

    // Add other class variables here as needed
};

GType gst_lpc10_dec_get_type(void);

G_END_DECLS

#endif /* __GST_LPC10_DEC_H__ */
