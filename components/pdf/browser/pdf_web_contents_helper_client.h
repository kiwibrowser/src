// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PDF_BROWSER_PDF_WEB_CONTENTS_HELPER_CLIENT_H_
#define COMPONENTS_PDF_BROWSER_PDF_WEB_CONTENTS_HELPER_CLIENT_H_

#include "base/callback.h"
#include "base/strings/string16.h"
#include "ipc/ipc_message.h"

namespace content {
class WebContents;
}

namespace pdf {

class PDFWebContentsHelperClient {
 public:
  virtual ~PDFWebContentsHelperClient() {}

  virtual void UpdateContentRestrictions(content::WebContents* contents,
                                         int content_restrictions) = 0;

  virtual void OnPDFHasUnsupportedFeature(content::WebContents* contents) = 0;

  virtual void OnSaveURL(content::WebContents* contents) = 0;
};

}  // namespace pdf

#endif  // COMPONENTS_PDF_BROWSER_PDF_WEB_CONTENTS_HELPER_CLIENT_H_
