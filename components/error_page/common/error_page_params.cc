// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/error_page/common/error_page_params.h"

#include "base/values.h"

namespace error_page {

ErrorPageParams::ErrorPageParams()
    : suggest_reload(false),
      reload_tracking_id(-1),
      search_tracking_id(-1) {
}

ErrorPageParams::~ErrorPageParams() {
}

}  // namespace error_page
