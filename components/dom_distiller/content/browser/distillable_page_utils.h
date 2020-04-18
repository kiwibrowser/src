// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_CONTENT_BROWSER_DISTILLABLE_PAGE_UTILS_H_
#define COMPONENTS_DOM_DISTILLER_CONTENT_BROWSER_DISTILLABLE_PAGE_UTILS_H_

#include "base/callback.h"
#include "content/public/browser/web_contents.h"

namespace dom_distiller {

class DistillablePageDetector;

// Uses the provided DistillablePageDetector to detect if the page is
// distillable. The passed detector must be alive until after the callback is
// called.
void IsDistillablePageForDetector(content::WebContents* web_contents,
                                  const DistillablePageDetector* detector,
                                  base::Callback<void(bool)> callback);

typedef base::RepeatingCallback<void(bool, bool, bool)> DistillabilityDelegate;

// Set the delegate to receive the result of whether the page is distillable.
void setDelegate(content::WebContents* web_contents,
                 DistillabilityDelegate delegate);
}

#endif  // COMPONENTS_DOM_DISTILLER_CONTENT_BROWSER_DISTILLABLE_PAGE_UTILS_H_
