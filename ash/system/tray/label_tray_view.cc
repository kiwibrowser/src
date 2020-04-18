// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray/label_tray_view.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/system/tray/hover_highlight_view.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/view_click_listener.h"
#include "ui/gfx/font.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"

namespace ash {

LabelTrayView::LabelTrayView(ViewClickListener* click_listener,
                             const gfx::VectorIcon& icon)
    : click_listener_(click_listener), icon_(icon) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  SetVisible(false);
}

LabelTrayView::~LabelTrayView() = default;

void LabelTrayView::SetMessage(const base::string16& message) {
  if (message_ == message)
    return;

  message_ = message;
  RemoveAllChildViews(true);

  if (!message_.empty()) {
    AddChildView(CreateChildView(message_));
    SetVisible(true);
  } else {
    SetVisible(false);
  }
}

views::View* LabelTrayView::CreateChildView(
    const base::string16& message) const {
  HoverHighlightView* child = new HoverHighlightView(click_listener_);
  gfx::ImageSkia icon_image = gfx::CreateVectorIcon(icon_, kMenuIconColor);
  child->AddIconAndLabelForDefaultView(icon_image, message);
  child->text_label()->SetMultiLine(true);
  child->text_label()->SetAllowCharacterBreak(true);
  child->SetExpandable(true);
  child->SetVisible(true);
  return child;
}

}  // namespace ash
