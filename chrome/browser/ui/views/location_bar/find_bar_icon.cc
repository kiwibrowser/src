// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/find_bar_icon.h"

#include "chrome/grit/generated_resources.h"
#include "components/toolbar/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"

FindBarIcon::FindBarIcon(PageActionIconView::Delegate* delegate)
    : PageActionIconView(nullptr, 0, delegate) {}

FindBarIcon::~FindBarIcon() {}

void FindBarIcon::SetActive(bool activate, bool should_animate) {
  if (activate ==
      (GetInkDrop()->GetTargetInkDropState() == views::InkDropState::ACTIVATED))
    return;
  if (activate) {
    if (should_animate) {
      AnimateInkDrop(views::InkDropState::ACTIVATED, nullptr);
    } else {
      GetInkDrop()->SnapToActivated();
    }
  } else {
    AnimateInkDrop(views::InkDropState::HIDDEN, nullptr);
  }
}

base::string16 FindBarIcon::GetTextForTooltipAndAccessibleName() const {
  return l10n_util::GetStringUTF16(IDS_TOOLTIP_FIND);
}

void FindBarIcon::OnExecuting(ExecuteSource execute_source) {}

views::BubbleDialogDelegateView* FindBarIcon::GetBubble() const {
  return nullptr;
}

const gfx::VectorIcon& FindBarIcon::GetVectorIcon() const {
  return toolbar::kFindInPageIcon;
}
