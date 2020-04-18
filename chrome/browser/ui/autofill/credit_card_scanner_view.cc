// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "chrome/browser/ui/autofill/credit_card_scanner_view.h"

namespace autofill {

// Not implemented on other platforms yet.
#if !defined(OS_ANDROID)
// static
bool CreditCardScannerView::CanShow() {
  return false;
}

// static
std::unique_ptr<CreditCardScannerView> CreditCardScannerView::Create(
    const base::WeakPtr<CreditCardScannerViewDelegate>& delegate,
    content::WebContents* web_contents) {
  return std::unique_ptr<CreditCardScannerView>();
}
#endif  // !defined(OS_ANDROID)

}  // namespace autofill
