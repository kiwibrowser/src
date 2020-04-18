// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/layout/layout_manager.h"

#include "ui/views/view.h"

namespace views {

LayoutManager::~LayoutManager() {
}

void LayoutManager::Installed(View* host) {
}

int LayoutManager::GetPreferredHeightForWidth(const View* host,
                                              int width) const {
  return GetPreferredSize(host).height();
}

void LayoutManager::ViewAdded(View* host, View* view) {
}

void LayoutManager::ViewRemoved(View* host, View* view) {
}

}  // namespace views
