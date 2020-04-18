// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SQL_VFS_WRAPPER_H_
#define SQL_VFS_WRAPPER_H_

#include "third_party/sqlite/sqlite3.h"

namespace sql {

// A wrapper around the default VFS.
//
// On OSX, the wrapper propagates Time Machine exclusions from the main database
// file to associated files such as journals. <http://crbug.com/23619> and
// <http://crbug.com/25959> and others.
//
// TODO(shess): On Windows, wrap xFetch() with a structured exception handler.
sqlite3_vfs* VFSWrapper();

}  // namespace sql

#endif  // SQL_VFS_WRAPPER_H_
