#!/bin/env bash
export GST_PLUGIN_PATH=$(pwd)/build 
gst-launch-1.0 filesrc location=sample.wav ! wavparse ! audioconvert ! audioresample ! capsfilter caps="audio/x-raw, format=S16LE, rate=8000, channels=1, layout=interleaved" ! lpc10enc ! capsfilter caps="application/x-lpc10, framerate=8000/180, frame-size=7" ! lpc10dec ! audioconvert ! audioresample ! autoaudiosink