// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/menu/menu_runner_impl_adapter.h"

#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner_impl.h"

namespace views {
namespace internal {

MenuRunnerImplAdapter::MenuRunnerImplAdapter(
    ui::MenuModel* menu_model,
    const base::Closure& on_menu_done_callback)
    : menu_model_adapter_(
          new MenuModelAdapter(menu_model, on_menu_done_callback)),
      impl_(new MenuRunnerImpl(menu_model_adapter_->CreateMenu())) {}

bool MenuRunnerImplAdapter::IsRunning() const {
  return impl_->IsRunning();
}

void MenuRunnerImplAdapter::Release() {
  impl_->Release();
  delete this;
}

void MenuRunnerImplAdapter::RunMenuAt(Widget* parent,
                                      MenuButton* button,
                                      const gfx::Rect& bounds,
                                      MenuAnchorPosition anchor,
                                      int32_t types) {
  impl_->RunMenuAt(parent, button, bounds, anchor, types);
}

void MenuRunnerImplAdapter::Cancel() {
  impl_->Cancel();
}

base::TimeTicks MenuRunnerImplAdapter::GetClosingEventTime() const {
  return impl_->GetClosingEventTime();
}

MenuRunnerImplAdapter::~MenuRunnerImplAdapter() {
}

}  // namespace internal
}  // namespace views
