// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/search_result_base_view.h"

namespace app_list {

SearchResultBaseView::SearchResultBaseView() : Button(this) {}

SearchResultBaseView::~SearchResultBaseView() = default;

void SearchResultBaseView::SetBackgroundHighlighted(bool enabled) {
  background_highlighted_ = enabled;
  SchedulePaint();
}

}  // namespace app_list
