# Safe Browsing DMG Test Data

This directory contains scripts to generate test DMG and HFS files for
unit-testing the Safe Browsing archive scanner. The contents of this directory
are primarily used by `//chrome/test:mac_safe_browsing_test_data`.

Most of the data are generated at build-time using the `generate_test_data.sh`
script. However, due to a [macOS issue](https://crbug.com/696529) the outputs
from the `make_hfs.sh` script are generated independently and checked-in.

These independently generated data are stored in `hfs_raw_images.tar.bz2` and
can be regenerated using the `Makefile` in this directory. The build system
will extract the archive into the build output directory.
