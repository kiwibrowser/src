// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/payments/validation_delegate.h"

#include "base/logging.h"

namespace payments {

ValidationDelegate::~ValidationDelegate() {}

bool ValidationDelegate::ShouldFormat() {
  return false;
}

base::string16 ValidationDelegate::Format(const base::string16& text) {
  NOTREACHED();
  return text;
}

}  // namespace payments
