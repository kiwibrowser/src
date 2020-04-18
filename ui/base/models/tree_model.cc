// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/models/tree_model.h"

#include "base/logging.h"

namespace ui {

void TreeModel::SetTitle(TreeModelNode* node,
                         const base::string16& title) {
  NOTREACHED();
}

int TreeModel::GetIconIndex(TreeModelNode* node) {
  return -1;
}

}  // namespace ui
