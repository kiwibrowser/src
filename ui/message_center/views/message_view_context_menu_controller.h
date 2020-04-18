// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_VIEWS_MESSAGE_VIEW_CONTEXT_MENU_CONTROLLER_H_
#define UI_MESSAGE_CENTER_VIEWS_MESSAGE_VIEW_CONTEXT_MENU_CONTROLLER_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/message_center/message_center_export.h"
#include "ui/views/context_menu_controller.h"

namespace ui {
class MenuModel;
}  // namespace ui

namespace views {
class MenuRunner;
}  // namespace views

namespace message_center {

class MESSAGE_CENTER_EXPORT MessageViewContextMenuController
    : public views::ContextMenuController {
 public:
  explicit MessageViewContextMenuController();
  ~MessageViewContextMenuController() override;

 private:
  // Overridden from views::ContextMenuController:
  void ShowContextMenuForView(views::View* source,
                              const gfx::Point& point,
                              ui::MenuSourceType source_type) override;

  // Callback for MenuRunner
  void OnMenuClosed();

  std::unique_ptr<ui::MenuModel> menu_model_;
  std::unique_ptr<views::MenuRunner> menu_runner_;

  DISALLOW_COPY_AND_ASSIGN(MessageViewContextMenuController);
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_VIEWS_MESSAGE_VIEW_CONTEXT_MENU_CONTROLLER_H_
