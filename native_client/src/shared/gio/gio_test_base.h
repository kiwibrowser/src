/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_SHARED_GIO_GIO_TEST_BASE_H_
#define NATIVE_CLIENT_SRC_SHARED_GIO_GIO_TEST_BASE_H_

/*
 * NaCl Generic I/O test utilities.
 */

#include "native_client/src/shared/gio/gio.h"

const int expected_file_size = 32;

/** Returns character expected to be at file_pos */
char GioExpectedCharAt(char initial_char, int file_pos);

/*
 * Must be given a file with contents:
 * [initial_char+0, initial_char+1, ..., initial_char+expected_file_size-1]. */
void GioReadTestWithOffset(struct Gio* my_file,
                           char initial_char);

/** Should be given a scratch file that can be written to without worry. */
void GioWriteTest(struct Gio* my_file,
                  bool fixed_length);

/*
 * Must be given a file with contents
 * [initial_char+0, initial_char+1, ..., initial_char+expected_file_size-1].
 */
void GioSeekTestWithOffset(struct Gio* my_file,
                           char initial_char,
                           bool wrap_err);

/** Closes and destroys the file. */
void GioCloseTest(struct Gio* my_file);

#endif  /* NATIVE_CLIENT_SRC_SHARED_GIO_GIO_TEST_BASE_H_ */
