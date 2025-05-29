#!/bin/env bash

GST_PLUGIN_PATH=$(pwd)/build
export GST_PLUGIN_PATH

echo "### Step 1: Encoding sample.wav to output.lpc ###"
gst-launch-1.0 --gst-debug="lpc10enc:4,lpc10dec:4" -v \
filesrc location=sample.wav ! wavparse ! audioconvert ! audioresample ! \
capsfilter caps="audio/x-raw, format=S16LE, rate=8000, channels=1, layout=interleaved" ! \
lpc10enc ! filesink location=output.lpc

echo -e "\n\n### Step 2: Decoding output.lpc to audio sink ###"
gst-launch-1.0 --gst-debug="lpc10enc:4,lpc10dec:4" -v \
filesrc location=output.lpc ! \
capsfilter caps="application/x-lpc10, framerate=8000/180, frame-size=7" ! \
lpc10dec ! audioconvert ! audioresample ! autoaudiosink
