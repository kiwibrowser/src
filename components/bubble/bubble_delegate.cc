// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/bubble/bubble_delegate.h"

BubbleDelegate::BubbleDelegate() {}

BubbleDelegate::~BubbleDelegate() {}

bool BubbleDelegate::ShouldClose(BubbleCloseReason reason) const {
  return true;
}

void BubbleDelegate::DidClose(BubbleCloseReason reason) {}

bool BubbleDelegate::UpdateBubbleUi(BubbleUi* bubble_ui) {
  return false;
}
