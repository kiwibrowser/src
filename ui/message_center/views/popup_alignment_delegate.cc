// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/views/popup_alignment_delegate.h"

#include "ui/message_center/views/message_popup_collection.h"

namespace message_center {

PopupAlignmentDelegate::PopupAlignmentDelegate() : collection_(NULL) {}

PopupAlignmentDelegate::~PopupAlignmentDelegate() {}

void PopupAlignmentDelegate::DoUpdateIfPossible() {
  if (collection_)
    collection_->DoUpdate();
}

}  // namespace message_center
