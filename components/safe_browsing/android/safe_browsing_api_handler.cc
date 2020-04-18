// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/android/safe_browsing_api_handler.h"
#include "base/bind.h"

namespace safe_browsing {

SafeBrowsingApiHandler* SafeBrowsingApiHandler::instance_ = nullptr;

// static
void SafeBrowsingApiHandler::SetInstance(SafeBrowsingApiHandler* instance) {
  instance_ = instance;
}

// static
SafeBrowsingApiHandler* SafeBrowsingApiHandler::GetInstance() {
  return instance_;
}

}  // namespace safe_browsing
