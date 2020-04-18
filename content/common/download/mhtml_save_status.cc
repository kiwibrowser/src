// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/download/mhtml_save_status.h"

#include "base/logging.h"

namespace content {

const char* GetMhtmlSaveStatusLabel(MhtmlSaveStatus save_status) {
  switch (save_status) {
    case MhtmlSaveStatus::SUCCESS:
      return "Success";
    case MhtmlSaveStatus::FILE_CLOSING_ERROR:
      return "File closing error";
    case MhtmlSaveStatus::FILE_CREATION_ERROR:
      return "File creation error";
    case MhtmlSaveStatus::FILE_WRITTING_ERROR:
      return "File writing error";
    case MhtmlSaveStatus::FRAME_NO_LONGER_EXISTS:
      return "Frame no longer exists";
    case MhtmlSaveStatus::FRAME_SERIALIZATION_FORBIDDEN:
      return "Main frame serialization forbidden";
    case MhtmlSaveStatus::RENDER_PROCESS_EXITED:
      return "Render process no longer exists";
  }
  NOTREACHED();
  return "<Invalid status>";
}

}  // namespace content
