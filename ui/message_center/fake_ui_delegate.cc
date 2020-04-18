// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/fake_ui_delegate.h"

#include "ui/message_center/ui_controller.h"

namespace message_center {

FakeUiDelegate::FakeUiDelegate() : tray_(new UiController(this)) {}

FakeUiDelegate::~FakeUiDelegate() {}

void FakeUiDelegate::OnMessageCenterContentsChanged() {}

bool FakeUiDelegate::ShowPopups() {
  return false;
}

void FakeUiDelegate::HidePopups() {}

bool FakeUiDelegate::ShowMessageCenter(bool show_by_click) {
  return false;
}

void FakeUiDelegate::HideMessageCenter() {}

bool FakeUiDelegate::ShowNotifierSettings() {
  return false;
}

}  // namespace message_center
