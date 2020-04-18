// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_FILE_INFO_UTIL_H_
#define CONTENT_RENDERER_FILE_INFO_UTIL_H_

#include "base/files/file.h"

namespace blink {
struct WebFileInfo;
}

namespace content {

// File info conversion
void FileInfoToWebFileInfo(const base::File::Info& file_info,
                           blink::WebFileInfo* web_file_info);

}  // namespace content

#endif  // CONTENT_RENDERER_FILE_INFO_UTIL_H_
