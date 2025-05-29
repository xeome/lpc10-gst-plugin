gst-launch-1.0 -v filesrc location=sample.wav ! wavparse ! audioconvert ! audioresample ! audio/x-raw, format=S16LE, rate=8000, channels=1, layout=interleaved ! lpc10enc ! application/x-lpc10, framerate=8000/180, frame-size=7 ! lpc10dec ! audioconvert ! audioresample ! alsasink


gst-launch-1.0 -v filesrc location=sample.wav ! wavparse ! audioconvert ! audioresample ! audio/x-raw, format=S16LE, rate=8000, channels=1, layout=interleaved !  audioconvert ! audioresample ! alsasink