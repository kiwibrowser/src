// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/models/combobox_model.h"

namespace ui {

bool ComboboxModel::IsItemSeparatorAt(int index) {
  return false;
}

int ComboboxModel::GetDefaultIndex() const {
  return 0;
}

bool ComboboxModel::IsItemEnabledAt(int index) const {
  return true;
}

}  // namespace ui
