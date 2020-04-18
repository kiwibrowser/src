// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/model/tab_model.h"

namespace vr {

TabModel::TabModel(int id, const base::string16& title)
    : id(id), title(title) {}
TabModel::TabModel(const TabModel& other) = default;
TabModel::~TabModel() = default;

}  // namespace vr
