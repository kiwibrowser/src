// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/confirm_quit_bubble.h"

#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/subtle_notification_view.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/border.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace {

constexpr base::TimeDelta kSlideDuration =
    base::TimeDelta::FromMilliseconds(200);

}  // namespace

ConfirmQuitBubble::ConfirmQuitBubble()
    : animation_(std::make_unique<gfx::SlideAnimation>(this)) {
  animation_->SetSlideDuration(kSlideDuration.InMilliseconds());
}

ConfirmQuitBubble::~ConfirmQuitBubble() {}

void ConfirmQuitBubble::Show() {
  animation_->Show();
}

void ConfirmQuitBubble::Hide() {
  animation_->Hide();
}

void ConfirmQuitBubble::AnimationProgressed(const gfx::Animation* animation) {
  float opacity = static_cast<float>(animation->CurrentValueBetween(0.0, 1.0));
  if (opacity == 0) {
    popup_.reset();
  } else {
    if (!popup_) {
      SubtleNotificationView* view = new SubtleNotificationView();

      popup_ = std::make_unique<views::Widget>();
      views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
      params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
      params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
      params.accept_events = false;
      params.keep_on_top = true;
      popup_->Init(params);
      popup_->SetContentsView(view);

      // TODO(thomasanderson): Localize this string.
      view->UpdateContent(
          base::WideToUTF16(L"Hold |Ctrl|+|Shift|+|Q| to quit"));

      gfx::Size size = view->GetPreferredSize();
      view->SetSize(size);
      popup_->CenterWindow(size);

      popup_->ShowInactive();
    }
    popup_->SetOpacity(opacity);
  }
}

void ConfirmQuitBubble::AnimationEnded(const gfx::Animation* animation) {
  AnimationProgressed(animation);
}
