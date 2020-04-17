# Chromium's QUIC

This directory contains a copy of Chromium's QUIC, along with what is
(hopefully) a minimum set of dependencies from Chromium.  All of the build files
were rewritten to avoid also porting Chromium's build infrastructure (gn flags,
configs, and absolute-path imports).

## Build files

The QUIC code is all covered by the BUILD.gn in this directory.  The remaining
Chromium code is covered by a BUILD.gn for each top-level directory under src/
(e.g. net, url, etc.).  src/base/ is handled slightly differently, because it is
a submodule where we still want to rewrite BUILD.gn, as well as some generated
files.  In this case, we use build/base/, which contains our new BUILD.gn and
generated files from an existing Chromium checkout.

## Cloning process

At a basic level, the original cloning process was as follows:
  1. Copy net/third_party/quic and add its files to BUILD.gn.
  2. Try to compile.
  3. Fix errors by copying evidently necessary files from Chromium and adding
     them to the appropriate BUILD.gn
  4. Repeat from 2 until everything works.

The process is mostly encapsulated in the script `clone_helper.sh`.  The only
caveat is that the original cloning process was done as described, but now
src/base/ is included as a submodule with its BUILD.gn and generated files in
build/base/.

The original clone is from Chromium commit
8b90885e60134c34176df2dd1834a94dbd6b73b9.
