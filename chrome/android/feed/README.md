# Feed Host UI

The feed Java package provides support for showing a list of article
suggestions rendered by the third party
[Feed](https://chromium.googlesource.com/feed) library in Chrome UI.

This directory contains two mirrored packages that provide real and dummy
implementations of classes to facilitate compiling out dependencies on
[//third_party/feed/](../../../third_party/feed/) and
[//components/feed/](../../../components/feed/) when the `enable_feed_in_chrome`
build flag is disabled. The public classes and methods in real/dummy used by
[//chrome/android/java/](../java/) must have identical signatures.

Library code and host API definitions can be found under
[//third_party/feed/](../../../third_party/feed/). More information about the
library is available in the [README.md](../../../third_party/feed/src/README.md).
