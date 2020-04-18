// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_CONTENT_BROWSER_DISTILLER_UI_HANDLE_H_
#define COMPONENTS_DOM_DISTILLER_CONTENT_BROWSER_DISTILLER_UI_HANDLE_H_

#include "base/macros.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace dom_distiller {

// ExternalFeedbackReporter handles reporting distillation quality through an
// external source.
class DistillerUIHandle {
 public:
  DistillerUIHandle() {}
  virtual ~DistillerUIHandle() {}

  // Open the UI settings for dom distiller.
  virtual void OpenSettings(content::WebContents* web_contents) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(DistillerUIHandle);
};

}  // namespace dom_distiller

#endif  // COMPONENTS_DOM_DISTILLER_CONTENT_BROWSER_DISTILLER_UI_HANDLE_H_
