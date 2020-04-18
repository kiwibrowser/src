Name: libpng
URL: http://libpng.org/
Version: 1.6.22
Security Critical: yes
License: libpng license
License Android Compatible: yes

Description:
PNG library.

Local Modifications:

Local changes in Chromium's copy of libpng as of https://crrev.com/404379 - see
README.chromium.

pnglibconf.h: a copy of libpng's scripts/pnglibconf.h.prebuilt.
pngprefix.h: manually-created redefinitions to avoid conflicts with Chromium.
0000-build-config.patch: Local build configuration changes.
0002-static-png-gt.patch: Unconditionally use static png_gt() in png.c to avoid compilation warning.
0003-check-errors-in-set-pcal.patch: Backported github.com/glennrp/libpng/pull/135
0004-invalid-icc.patch: Fix for large allocation for invalid ICC https://github.com/glennrp/libpng/commit/92a7c79db2c962d04006b35e2603ba9d5ce75541
