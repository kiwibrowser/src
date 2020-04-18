// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/clipboard/clipboard.h"

namespace ui {

const char Clipboard::kMimeTypeText[] = "text/plain";
const char Clipboard::kMimeTypeURIList[] = "text/uri-list";
const char Clipboard::kMimeTypeMozillaURL[] = "text/x-moz-url";
const char Clipboard::kMimeTypeDownloadURL[] = "downloadurl";
const char Clipboard::kMimeTypeHTML[] = "text/html";
const char Clipboard::kMimeTypeRTF[] = "text/rtf";
const char Clipboard::kMimeTypePNG[] = "image/png";
// TODO(dcheng): This name is temporary. See crbug.com/106449.
const char Clipboard::kMimeTypeWebCustomData[] = "chromium/x-web-custom-data";
const char Clipboard::kMimeTypeWebkitSmartPaste[] = "chromium/x-webkit-paste";
const char Clipboard::kMimeTypePepperCustomData[] =
    "chromium/x-pepper-custom-data";

}  // namespace ui
