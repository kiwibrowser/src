// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GESTURES_FILE_UTIL_H_
#define GESTURES_FILE_UTIL_H_

#include <string>

namespace gestures {

// Reads the file at |path| into |contents| and returns true on success.
// |contents| may be NULL, in which case this function is useful for its
// side effect of priming the disk cache (could be used for unit tests).
// The function returns false and the string pointed to by |contents| is
// cleared when |path| does not exist or if it contains path traversal
// components ('..').
bool ReadFileToString(const char* path, std::string* contents);

// Writes the given buffer into the file, overwriting any data that was
// previously there.  Returns the number of bytes written, or -1 on error.
int WriteFile(const char* filename, const char* data, int size);

}  // namespace gestures

#endif  // GESTURES_UTIL_H_
