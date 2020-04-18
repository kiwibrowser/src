#!/bin/bash
# Copyright 2016 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script assumes the waterfall framework
# (https://github.com/WebAssembly/waterfall) has been downloaded and run, thus
# it is required that all tools necessary for wasm be already installed in the
# system and their relative paths to the waterfall directory follow the
# directory structure of the waterfall framework. Or you can download a prebuilt
# framework for Linux directly from the buildbot at https://wasm-stat.us/.
#
# Environment variable $WASM_INSTALL_DIR must point to the wasm install
# directory in the waterfall, which is WATERFALL_DIR/src/work/wasm-install where
# WATERFALL_DIR is the top waterfall directory.

# Some limitations of this framework:
#
# Spec2k uses a lot of file I/O (stdin/stdout, reading files, writing files)
# which doesn't really work in JavaScript. Emscripten deals with this by
# providing the --embed-file and --preload-file options to package input files
# into a virtual in-memory file system. --embed-file allows input files, stdin,
# and stdout to work in the standalone js shell, except that initialization of
# the file system is extremely slow and ruins any attempt to get reasonable
# timings. --preload-file works reasonably fast, but it only works when running
# in a browser and does not work with d8 shell. d8 shell supports read()
# function which can read a file, so this might be able to be fixed for d8 shell
# if Emscripten hooks up appropriate d8 functions. But it is not supported at
# the time of this writing and --embed-file is the only option.
#
# Currently Wasm does not build and run correctly for many of the SPEC
# benchmarks, which needs further investigation. In addition, 254.gap and
# 176.gcc do not (yet) work because Emscripten does not support setjmp. Also,
# currently wasm build fails if you override some of libc functions, because
# Emscripten does not use archives and has all its libc functions precompiled in
# ~/.emscripten_cache/wasm/libc.bc. 197.parser fails due to this reason
# (strncasecmp is duplicated).
#
# The other file I/O related problem is that it is not possible for the harness
# to inspect output files (which are only represented in-memory), so validation
# is only possible by looking at stdout. To create persistent data out of a
# session, emscripten provides two file system: NODEFS and IDBFS. However,
# NODEFS is only for use when running inside node.js, and IDBFS works only when
# running inside a browser.
#
# For the "train" versus "ref" runs, the run scripts generally copy or symlink
# files from the appropriate input directory into the current directory before
# running the binary. This approach doesn't work for Emscripten, where the files
# need to be prepackaged at build time. The prepackaging is done by
# concatenating all the files into one big file, and embedding offsets and
# lengths into the JavaScript code. This means we have to build separate
# versions for "train" and "ref". To avoid duplication, the input file
# preparation is refactored into a new prepare_input.sh script in each Spec2k
# component directory.

if [ -z $WASM_INSTALL_DIR ]; then
  echo 'error: WASM_INSTALL_DIR is not set'
  exit 1
fi
if [ -z $SCRIPTNAME ]; then
  echo 'error: SCRIPTNAME is not set. Try running using run_all.sh.'
  exit 1
fi
REFTRAIN=`echo $SCRIPTNAME | sed -e 's/.*run\.//' -e 's/\.sh$//'`
$WASM_INSTALL_DIR/bin/d8 --expose-wasm $(basename $1 .js).$REFTRAIN.js -- ${@:2}
