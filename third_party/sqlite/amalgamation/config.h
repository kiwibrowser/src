// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_SQLITE_AMALGAMATION_CONFIG_H_
#define THIRD_PARTY_SQLITE_AMALGAMATION_CONFIG_H_

// This file is included by sqlite3.c fairly early.

// We prefix chrome_ to SQLite's exported symbols, so that we don't clash with
// other SQLite libraries loaded by the system libraries. This only matters when
// using the component build, where our SQLite's symbols are visible to the
// dynamic library loader.
#include "third_party/sqlite/amalgamation/rename_exports.h"

// Linux-specific configuration fixups.
#if defined(__linux__)

// features.h, included below, indirectly includes sys/mman.h. The latter header
// only defines mremap if _GNU_SOURCE is defined. Depending on the order of the
// files in the amalgamation, removing the define below may result in a build
// error on Linux.
#if defined(__GNUC__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <features.h>

// SQLite wants to track malloc sizes. On OSX it uses malloc_size(), on Windows
// _msize(), elsewhere it handles it manually by enlarging the malloc and
// injecting a field. Enable malloc_usable_size() for Linux.
//
// malloc_usable_size() is not exported by the Android NDK. It is not
// implemented by uclibc.
#if !defined(__UCLIBC__) && !defined(__ANDROID__)
#define HAVE_MALLOC_H 1
#define HAVE_MALLOC_USABLE_SIZE 1
#endif

#endif  // defined(__linux__)

#endif  // THIRD_PARTY_SQLITE_AMALGAMATION_CONFIG_H_
