// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_FILE_CHOOSER_PARAMS_H_
#define CONTENT_PUBLIC_COMMON_FILE_CHOOSER_PARAMS_H_

#include <vector>

#include "base/files/file_path.h"
#include "base/strings/string16.h"
#include "build/build_config.h"
#include "content/common/content_export.h"
#include "url/gurl.h"

namespace content {

// Struct used by WebContentsDelegate.
struct CONTENT_EXPORT FileChooserParams {
  FileChooserParams();
  FileChooserParams(const FileChooserParams& other);
  ~FileChooserParams();

  enum Mode {
    // Requires that the file exists before allowing the user to pick it.
    Open,

    // Like Open, but allows picking multiple files to open.
    OpenMultiple,

    // Like Open, but selects a folder for upload.
    UploadFolder,

    // Allows picking a nonexistent file, and prompts to overwrite if the file
    // already exists.
    Save,
  };

  Mode mode;

  // Title to be used for the dialog. This may be empty for the default title,
  // which will be either "Open" or "Save" depending on the mode.
  base::string16 title;

  // Default file name to select in the dialog.
  base::FilePath default_file_name;

  // A list of valid lower-cased MIME types or file extensions specified in an
  // input element. It is used to restrict selectable files to such types.
  std::vector<base::string16> accept_types;

  // Whether the caller needs native file path or not.
  bool need_local_path;

#if defined(OS_ANDROID)
  // See http://www.w3.org/TR/html-media-capture for more information.
  // If true, the data should be obtained using the device's camera/mic/etc.
  bool capture;
#endif

  // If non-empty, represents the URL of the requestor if the request was
  // initiated by a document. Note that this value should be considered
  // untrustworthy since it is specified by the sandbox and not validated.
  GURL requestor;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_FILE_CHOOSER_PARAMS_H_
