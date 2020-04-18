// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_CORE_FEEDBACK_REPORTER_H_
#define COMPONENTS_DOM_DISTILLER_CORE_FEEDBACK_REPORTER_H_

#include "base/macros.h"

namespace dom_distiller {

// FeedbackReporter handles reporting distillation quality.
class FeedbackReporter {
 public:
  FeedbackReporter();
  virtual ~FeedbackReporter();

  // Reports the quality of the distillation. |good| represents whether the
  // perceived quality of the distillation of a web page was good.
  static void ReportQuality(bool good);

 private:
  DISALLOW_COPY_AND_ASSIGN(FeedbackReporter);
};

}  // namespace dom_distiller

#endif  // COMPONENTS_DOM_DISTILLER_CORE_FEEDBACK_REPORTER_H_
