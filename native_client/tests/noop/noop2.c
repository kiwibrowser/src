/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
  Intentionally do nothing.  If even this program won't run, here are
  some debugging tips:

  The last spot in the loader hit before jumping into the NaCl
  executable is NaClSwitch().

  After we're done with our main(), the exit system call will be
  called, which hits NaClSysExitDecoder() in the loader.
*/

#include <stdio.h>

int main(int argc, char* argv[]) {
  return 2;
}
