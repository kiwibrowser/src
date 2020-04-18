/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* we need to make sure that _start is the very first function */
/* NOTE: gcc reorders functions in a file arbitrarily */

/* @IGNORE_LINES_FOR_CODE_HYGIENE[1] */
extern int main(void);

void _start(void) {
  main();
}
