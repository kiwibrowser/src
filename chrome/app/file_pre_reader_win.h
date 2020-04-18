// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines a function to pre-read a file in order to avoid touching
// the disk when it is subsequently used.

#ifndef CHROME_APP_FILE_PRE_READER_WIN_H_
#define CHROME_APP_FILE_PRE_READER_WIN_H_

namespace base {
class FilePath;
}

// Pre-reads |file_path| to avoid touching the disk when the file is actually
// used.
void PreReadFile(const base::FilePath& file_path);

#endif  // CHROME_APP_FILE_PRE_READER_WIN_H_
