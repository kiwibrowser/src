// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_MESSAGE_CENTER_POPUPS_ONLY_UI_DELEGATE_H_
#define CHROME_BROWSER_UI_VIEWS_MESSAGE_CENTER_POPUPS_ONLY_UI_DELEGATE_H_

#include <memory>

#include "base/macros.h"
#include "ui/message_center/ui_delegate.h"

namespace message_center {
class DesktopPopupAlignmentDelegate;
class MessageCenter;
class UiController;
class MessagePopupCollection;
}  // namespace message_center

// A message center view implementation that shows notification popups (toasts)
// in the corner of the screen, but has no dedicated message center (widget with
// multiple notifications inside). This is used on Windows and Linux for
// non-native notifications.
class PopupsOnlyUiDelegate : public message_center::UiDelegate {
 public:
  PopupsOnlyUiDelegate();
  ~PopupsOnlyUiDelegate() override;

  message_center::MessageCenter* message_center();

  // UiDelegate implementation.
  bool ShowPopups() override;
  void HidePopups() override;
  bool ShowMessageCenter(bool show_by_click) override;
  void HideMessageCenter() override;
  void OnMessageCenterContentsChanged() override;
  bool ShowNotifierSettings() override;

  message_center::UiController* GetUiControllerForTesting();

 private:
  std::unique_ptr<message_center::MessagePopupCollection> popup_collection_;
  std::unique_ptr<message_center::DesktopPopupAlignmentDelegate>
      alignment_delegate_;
  std::unique_ptr<message_center::UiController> ui_controller_;

  DISALLOW_COPY_AND_ASSIGN(PopupsOnlyUiDelegate);
};

#endif  // CHROME_BROWSER_UI_VIEWS_MESSAGE_CENTER_POPUPS_ONLY_UI_DELEGATE_H_
