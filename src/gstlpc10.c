#include "gstlpc10enc.h"
#include "gstlpc10dec.h"
#include <gst/gst.h>
#include "version.h"

GST_DEBUG_CATEGORY_STATIC(gst_lpc10_debug_category);
#define GST_CAT_DEFAULT gst_lpc10_debug_debug_category

/* Define PACKAGE and VERSION for GST_PLUGIN_DEFINE */
#define PACKAGE "gstlpc10"  // Corresponds to the plugin name
#define VERSION THISBUILD   // Use the combined version string from version.h

/* prototypes */

static gboolean plugin_init(GstPlugin* plugin) {
    GST_DEBUG_CATEGORY_INIT(gst_lpc10_debug_category, "lpc10", 0, "LPC10 plugin");

    if (!gst_element_register(plugin, "lpc10enc", GST_RANK_NONE, GST_TYPE_LPC10_ENC))
        return FALSE;

    if (!gst_element_register(plugin, "lpc10dec", GST_RANK_NONE, GST_TYPE_LPC10_DEC))
        return FALSE;

    return TRUE;
}

/* PACKAGE: this is usually the same as the plugin name */
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  lpc10,
                  "LPC10 encoder and decoder",
                  plugin_init,
                  VERSION,
                  "LGPL",
                  "GStreamer",
                  "https://gstreamer.freedesktop.org/")
