#!/bin/bash
export GST_PLUGIN_PATH=$PWD/build:$GST_PLUGIN_PATH
DEVICE=${1:-"default"}  # Use "default" if not specified
gst-launch-1.0 pulsesrc device="$DEVICE" \
    ! audioconvert \
    ! audioresample \
    ! "audio/x-raw,format=S16LE,rate=8000,channels=1" \
    ! lpc10enc \
    ! lpc10dec \
    ! autoaudiosink
echo "Pipeline finished."
