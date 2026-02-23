------------------------------------
- Desktop ANPR demo app ------------
------------------------------------

The purpose of this application is to demonstrate the usage of the ANPR engine library.
The engine's source code is meant to be very portable, independent of Qt or any other third-party libraries.

Compilation
-----------
1. Open the Qt project in QtCreator (src/DesktopAnpr.pro)
2. Configure and build it. No further preparation needed.

Running
-------
The ANPR engine needs the have access to it's data file (which is included within this pack). The program will look for this as "lpr.lprdat".
This file has to be in the path for the program.

For video playback to work, the program uses the the media back-end of the operating system (see: QMediaPlayer class documentation).
If you experience problems with the opening or playback of video files, you probably have to install some codecs on your system.

To be sure:
  on Linux  : install gstreamer and all codecs
  on Windows: install K-Lite Codec Pack basic (https://www.codecguide.com/download_k-lite_codec_pack_basic.htm)

Notes
-----
Tested with:
* Qt Creator 4.5.2
* Qt 5.10.0 open-source
* g++ 7.2.0
* Ubuntu 17.10 (kernel: 4.13.0-36-generic)
