# Platform API

The Platform API is designed to allow easy porting of the Open Screen Library
between platforms. There are some default implementations included in the
library.

## Directory structure

 - api/ contains the Platform API which is used by the Open Screen Library.
   Some of the public API may also include adapter code that acts as a small
   shim above the C functions to be implemented by the platform.
 - Other directories contain default implementations provided for the API.
   Currently, this includes:
    - base/ is implemented without any platform-specific features and so may be
      included on any platform.
    - linux/ is specific to Linux.
    - posix/ is specific to Posix (i.e. Linux, BSD, Mac, etc.).
