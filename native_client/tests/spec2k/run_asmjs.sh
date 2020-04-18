#!/bin/sh
# Copyright 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# $FIREFOX must point to the Firefox Nightly build (or some other
# build supporting Asm.js).  Alternatively, $PATH can be set such that
# "firefox" executes the Firefox nightly build.

# Some limitations of this framework:
#
# Spec2k uses a lot of file I/O (stdin/stdout, reading files, writing
# files) which doesn't really work in JavaScript. Emscripten deals with
# this by providing the --embed-file and --preload-file options to
# package input files into a virtual in-memory file system. Using
# --embed-file allows input files, stdin, and stdout to work in the
# standalone js shell, except that initialization of the file system is
# extremely slow and ruins any attempt to get reasonable timings. Using
# --preload-file works reasonably fast, but does not work in the
# standalone js shell and must be run in the browser, and stdin is not
# usable.
#
# As such, the following Spec2k components do not work due to their use
# of stdin: 183.equake, 186.crafty, 188.ammp, 197.parser, 253.perlbmk,
# and 254.gap. In addition, 254.gap and 176.gcc do not (yet) work
# because Emscripten does not support setjmp. That leaves 9 out of 16
# components that do work.
#
# The other file I/O related problem is that it is not possible for the
# harness to inspect output files (which are only represented
# in-memory), so validation is only possible by looking at stdout which
# is displayed on the browser page.
#
# As for getting timings, ideally we would want the JavaScript code to
# close the tab/window/browser after it completes. There is
# easily-found JavaScript code to do this, but it isn't reliable on
# Firefox when closing the last tab. As a result, one has to "babysit"
# the tests and manually close the browser as each test completes. This
# means that one needs to use the user+system time, not the wall-clock
# time, in reporting results.
#
# For the "train" versus "ref" runs, the run scripts generally copy or
# symlink files from the appropriate input directory into the current
# directory before running the binary. This approach doesn't work for
# Emscripten, where the files need to be prepackaged at build time. The
# prepackaging is done by concatenating all the files into one big file,
# and embedding offsets and lengths into the JavaScript code. This
# means we have to build separate versions for "train" and "ref". To
# avoid duplication, the input file preparation is refactored into a new
# prepare_input.sh script in each Spec2k component directory.
#
# The emcc command must be part of $PATH. To build Emscripten, get the
# source via "git clone git://github.com/kripken/emscripten.git". There
# is a nice tutorial at
# https://github.com/kripken/emscripten/wiki/Tutorial . Emscripten
# requires Clang+LLVM 3.2, it is convenient to download a prebuilt
# package, e.g.
# http://llvm.org/releases/3.2/clang+llvm-3.2-x86_64-linux-ubuntu-12.04.tar.gz
#
# At this writing, Asm.js is only supported in the Firefox nightly
# builds. It may be necessary to enable javascript.options.asmjs in
# about:config. It is also useful to set dom.max_script_run_time=0 to
# disable the "unresponsive script" popups.

export FIREFOX=${FIREFOX:-firefox}

PORT=8888

python -m SimpleHTTPServer "$PORT" > /dev/null 2>&1 &
PID=$!

# Build a URL of the form http://localhost/argv0?argv1&argv2&argv3
URL=`echo "$*" | sed -e 's/ /?/' -e 's/ /\&/g' -e "s?^?http://localhost:$PORT/?"`
REFTRAIN=`echo $SCRIPTNAME | sed -e 's/.*run\.//' -e 's/\.sh$//'`
URL=`echo $URL | sed "s/.emcc.html/.emcc.html.$REFTRAIN.html/"`

$FIREFOX "$URL"

kill $PID
