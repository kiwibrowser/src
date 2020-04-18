// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_PAGE_IMPORTANCE_SIGNALS_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_PAGE_IMPORTANCE_SIGNALS_H_

#include "third_party/blink/public/platform/web_common.h"

namespace blink {

class WebViewClient;

// WebPageImportanceSignals indicate the importance of the page state to user.
// This signal is propagated to embedder so that it can prioritize preserving
// state of certain page over the others.
class WebPageImportanceSignals {
 public:
  WebPageImportanceSignals() { Reset(); }

  bool HadFormInteraction() const { return had_form_interaction_; }
  BLINK_EXPORT void SetHadFormInteraction();

  BLINK_EXPORT void Reset();
#if INSIDE_BLINK
  BLINK_EXPORT void OnCommitLoad();
#endif

  void SetObserver(WebViewClient* observer) { observer_ = observer; }

 private:
  bool had_form_interaction_ : 1;

  WebViewClient* observer_ = nullptr;
};

}  // namespace blink

#endif  // WebPageImportancesignals_h
