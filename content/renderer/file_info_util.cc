// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/file_info_util.h"

#include "base/logging.h"
#include "third_party/blink/public/platform/web_file_info.h"

namespace content {

void FileInfoToWebFileInfo(const base::File::Info& file_info,
                           blink::WebFileInfo* web_file_info) {
  DCHECK(web_file_info);
  // Blink now expects NaN as uninitialized/null Date.
  if (file_info.last_modified.is_null())
    web_file_info->modification_time = std::numeric_limits<double>::quiet_NaN();
  else
    web_file_info->modification_time = file_info.last_modified.ToJsTime();
  web_file_info->length = file_info.size;
  if (file_info.is_directory)
    web_file_info->type = blink::WebFileInfo::kTypeDirectory;
  else
    web_file_info->type = blink::WebFileInfo::kTypeFile;
}

static_assert(std::numeric_limits<double>::has_quiet_NaN,
              "should have quiet NaN");

}  // namespace content
