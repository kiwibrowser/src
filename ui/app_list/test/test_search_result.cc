// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/test/test_search_result.h"

namespace app_list {

TestSearchResult::TestSearchResult() {
}

TestSearchResult::~TestSearchResult() {
}

void TestSearchResult::set_result_id(const std::string& id) {
  set_id(id);
}

}  // namespace app_list
