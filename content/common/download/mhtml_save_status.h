// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_DOWNLOAD_MHTML_SAVE_STATUS_H_
#define CONTENT_COMMON_DOWNLOAD_MHTML_SAVE_STATUS_H_

namespace content {

// Status result enum for MHTML generation.
enum class MhtmlSaveStatus {
  SUCCESS = 0,

  // Could not properly close the file where data was written to. Determined by
  // the browser.
  FILE_CLOSING_ERROR,

  // Could not create the file that would be written to. Determined by the
  // browser.
  FILE_CREATION_ERROR,

  // Could not write serialized data to the file. Determined by the renderer.
  FILE_WRITTING_ERROR,

  // The DOM changed and a previously existing frame is no more. Determined by
  // the browser.
  FRAME_NO_LONGER_EXISTS,

  // The main frame was not allowed to be serialized by either policy or cache
  // control. Determined by the renderer.
  FRAME_SERIALIZATION_FORBIDDEN,

  // A render process needed for the serialization of one of the page's frame is
  // no more. Determined by the browser.
  RENDER_PROCESS_EXITED,

  // NOTE: always keep this entry at the end and add new status types only
  // immediately above this line. Set LAST to the new last value and update the
  // implementation of GetMhtmlSaveStatusLabel. Make sure to update the
  // corresponding histogram enum accordingly.
  LAST = RENDER_PROCESS_EXITED,
};

// Gets a textual representation of the provided MhtmlSaveStatus value.
const char* GetMhtmlSaveStatusLabel(MhtmlSaveStatus save_status);

}  // namespace content

#endif  // CONTENT_COMMON_DOWNLOAD_MHTML_SAVE_STATUS_H_
