#ifndef __GST_LPC10_ENC_H__
#define __GST_LPC10_ENC_H__

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include "lpc10.h"  // Corrected include path

G_BEGIN_DECLS

#define GST_TYPE_LPC10_ENC (gst_lpc10_enc_get_type())
#define GST_LPC10_ENC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_LPC10_ENC, GstLpc10Enc))
#define GST_LPC10_ENC_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_LPC10_ENC, GstLpc10EncClass))
#define GST_IS_LPC10_ENC(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_LPC10_ENC))
#define GST_IS_LPC10_ENC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_LPC10_ENC))

typedef struct _GstLpc10Enc GstLpc10Enc;
typedef struct _GstLpc10EncClass GstLpc10EncClass;

struct _GstLpc10Enc {
    GstAudioEncoder parent;  // Corrected parent type

    // Private data for the LPC10 encoder state
    struct lpc10_encoder_state* lpc10_state;

    // Input buffering
    GstBuffer* input_buffer;
    gsize input_buffer_offset;
    gboolean eos_received;  // Flag to indicate if EOS has been received

    gsize processed_samples;  // Keep track of the total number of samples processed

    // Add other instance variables here as needed
};

struct _GstLpc10EncClass {
    GstAudioEncoderClass parent_class;  // Corrected parent class type

    // Add other class variables here as needed
};

GType gst_lpc10_enc_get_type(void);

G_END_DECLS

#endif /* __GST_LPC10_ENC_H__ */
