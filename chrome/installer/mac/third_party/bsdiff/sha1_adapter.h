/* Copyright (c) 2011 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

#ifndef CHROME_INSTALLER_MAC_THIRD_PARTY_BSDIFF_SHA1_ADAPTER_H_
#define CHROME_INSTALLER_MAC_THIRD_PARTY_BSDIFF_SHA1_ADAPTER_H_

/* This file defines a wrapper around Chromium's C++ base::SHA1HashBytes
 * function allowing it to be called from C code. */

#include <sys/types.h>

#define SHA1_DIGEST_LENGTH 20

#if defined(__cplusplus)
extern "C" {
#endif

void SHA1(const unsigned char* data, size_t len, unsigned char* hash);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  /* CHROME_INSTALLER_MAC_THIRD_PARTY_BSDIFF_SHA1_ADAPTER_H_ */
