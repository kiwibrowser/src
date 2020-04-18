// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_PAGE_IMPORTANCE_SIGNALS_H_
#define CONTENT_PUBLIC_COMMON_PAGE_IMPORTANCE_SIGNALS_H_

namespace content {

// PageImportanceSignals contains useful signals for judging relative importance
// of a tab. This information is provided from renderer to browser.
// This struct is a subset of Blink's WebPageImportanceSignals, containing only
// the signals used in Chromium side.
struct PageImportanceSignals {
  PageImportanceSignals() : had_form_interaction(false) {}

  bool had_form_interaction;
};
};

#endif  // CONTENT_PUBLIC_COMMON_PAGE_IMPORTANCE_SIGNALS_H_
