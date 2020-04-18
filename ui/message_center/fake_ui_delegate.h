// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_FAKE_UI_DELEGATE_H_
#define UI_MESSAGE_CENTER_FAKE_UI_DELEGATE_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "ui/message_center/ui_delegate.h"

namespace message_center {

class UiController;

// A message center UI delegate which does nothing.
class FakeUiDelegate : public UiDelegate {
 public:
  FakeUiDelegate();
  ~FakeUiDelegate() override;

  // Overridden from UiDelegate:
  void OnMessageCenterContentsChanged() override;
  bool ShowPopups() override;
  void HidePopups() override;
  bool ShowMessageCenter(bool show_by_click) override;
  void HideMessageCenter() override;
  bool ShowNotifierSettings() override;

 private:
  std::unique_ptr<UiController> tray_;
  base::Closure quit_closure_;

  DISALLOW_COPY_AND_ASSIGN(FakeUiDelegate);
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_FAKE_UI_DELEGATE_H_
