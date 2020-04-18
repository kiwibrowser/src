The media-gpu-accelerated prefix sets flags to facilitate hardware accelerated
video decoding on android bots.

TODO(chcunningham): Explore enabling hardware acceleration for desktop bots.
Non-android bots do not currently test with proprietary codecs, which translates
to no hardware acceleration.

Layout tests generally run using Mesa (see http://crrev.com/23868030) to
to produce more stable output for pixel layout tests when running on different
devices. However, Mesa is not compatible with android's SurfaceTexture,
used in in HW accelerated video decoding (H264 in mp4). We specify
use-gpu-in-tests to override the MesaGL default.
