#ifndef __GST_LPC10_MACROS_H__
#define __GST_LPC10_MACROS_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <glib-object.h>

#ifndef GST_FLOW_NEED_DATA
#define GST_FLOW_NEED_DATA GST_FLOW_CUSTOM_SUCCESS
#endif

/*
 * Macro to define the GObject type and private structure for LPC10 elements.
 * Usage: GST_LPC10_DEFINE_TYPE(Enc, enc, GST_TYPE_BASE_TRANSFORM, struct lpc10_encoder_state);
 *        GST_LPC10_DEFINE_TYPE(Dec, dec, GST_TYPE_BASE_TRANSFORM, struct lpc10_decoder_state);
 */
#define GST_LPC10_DEFINE_TYPE(TypeName, type_name, ParentType, StateStruct)  \
    typedef struct _GstLpc10##TypeName GstLpc10##TypeName;                   \
    typedef struct _GstLpc10##TypeName##Class GstLpc10##TypeName##Class;     \
    typedef struct _GstLpc10##TypeName##Private GstLpc10##TypeName##Private; \
                                                                             \
    struct _GstLpc10##TypeName##Private {                                    \
        StateStruct* lpc10_state;                                            \
    };                                                                       \
                                                                             \
    G_DEFINE_TYPE_WITH_CODE(GstLpc10##TypeName, gst_lpc10_##type_name, ParentType, G_ADD_PRIVATE(GstLpc10##TypeName))

/*
 * Macro to define standard GstBaseTransform function prototypes.
 * Usage: GST_LPC10_BASE_TRANSFORM_PROTOTYPES(Enc, enc);
 *        GST_LPC10_BASE_TRANSFORM_PROTOTYPES(Dec, dec);
 */
#define GST_LPC10_BASE_TRANSFORM_PROTOTYPES(TypeName, type_name)                                                              \
    static void gst_lpc10_##type_name##_init(GstLpc10##TypeName* type_name);                                                  \
    static void gst_lpc10_##type_name##_class_init(GstLpc10##TypeName##Class* klass);                                         \
    static void gst_lpc10_##type_name##_dispose(GObject* object);                                                             \
    static void gst_lpc10_##type_name##_finalize(GObject* object);                                                            \
    static GstCaps* gst_lpc10_##type_name##_transform_caps(GstBaseTransform* trans, GstPadDirection direction, GstCaps* caps, \
                                                           GstCaps* filter);                                                  \
    static gboolean gst_lpc10_##type_name##_set_caps(GstBaseTransform* trans, GstCaps* incaps, GstCaps* outcaps);             \
    static gboolean gst_lpc10_##type_name##_start(GstBaseTransform* trans);                                                   \
    static gboolean gst_lpc10_##type_name##_stop(GstBaseTransform* trans);                                                    \
    static GstFlowReturn gst_lpc10_##type_name##_transform(GstBaseTransform* trans, GstBuffer* inbuf, GstBuffer* outbuf);

/*
 * Macro to assign standard GstBaseTransform function pointers in class_init.
 * Usage: GST_LPC10_BASE_TRANSFORM_ASSIGN(klass, type_name);
 */
#define GST_LPC10_BASE_TRANSFORM_ASSIGN(klass, type_name)                                             \
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);                                              \
    GstBaseTransformClass* base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);                    \
                                                                                                      \
    gobject_class->dispose = gst_lpc10_##type_name##_dispose;                                         \
    gobject_class->finalize = gst_lpc10_##type_name##_finalize;                                       \
                                                                                                      \
    base_transform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_lpc10_##type_name##_transform_caps); \
    base_transform_class->set_caps = GST_DEBUG_FUNCPTR(gst_lpc10_##type_name##_set_caps);             \
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_lpc10_##type_name##_start);                   \
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_lpc10_##type_name##_stop);                     \
    base_transform_class->transform = GST_DEBUG_FUNCPTR(gst_lpc10_##type_name##_transform);

#endif /* __GST_LPC10_MACROS_H__ */
