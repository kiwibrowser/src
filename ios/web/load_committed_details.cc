// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/load_committed_details.h"

namespace web {

LoadCommittedDetails::LoadCommittedDetails()
    : item(nullptr), previous_item_index(-1), is_in_page(false) {
}

}  // namespace web
