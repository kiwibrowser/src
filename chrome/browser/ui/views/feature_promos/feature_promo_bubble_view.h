// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_FEATURE_PROMO_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_FEATURE_PROMO_BUBBLE_VIEW_H_

#include "base/macros.h"
#include "base/timer/timer.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

namespace gfx {
class Rect;
}

namespace ui {
class MouseEvent;
}

// The FeaturePromoBubbleView is a special BubbleDialogDelegateView for
// in-product help which educates users about certain Chrome features in a
// deferred context.
class FeaturePromoBubbleView : public views::BubbleDialogDelegateView {
 public:
  enum class ActivationAction {
    DO_NOT_ACTIVATE,
    ACTIVATE,
  };

  ~FeaturePromoBubbleView() override;

  // Closes the promo bubble.
  void CloseBubble();

 protected:
  // The |anchor_view| is used to anchor the FeaturePromoBubbleView. The |arrow|
  // sets where the arrow hangs off the bubble and the |string_specifier| is
  // the text that is displayed on the bubble. The |activation_action| sets if
  // the bubble is active or not.
  FeaturePromoBubbleView(views::View* anchor_view,
                         views::BubbleBorder::Arrow arrow,
                         int string_specifier,
                         ActivationAction activation_action);

  // The |anchor_rect| is used to anchor bubble views that have custom
  // positioning requirements. The |arrow| sets where the arrow hangs off the
  // bubble and the |string_specifier| is the text that is displayed on the
  // bubble.
  FeaturePromoBubbleView(const gfx::Rect& anchor_rect,
                         views::BubbleBorder::Arrow arrow,
                         int string_specifier);

 private:
  FeaturePromoBubbleView(views::View* anchor_view,
                         const gfx::Rect& anchor_rect,
                         views::BubbleBorder::Arrow arrow,
                         int string_specifier,
                         ActivationAction activation_action);

  // BubbleDialogDelegateView:
  int GetDialogButtons() const override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  gfx::Rect GetBubbleBounds() override;

  // Starts a timer to close the promo bubble.
  void StartAutoCloseTimer(base::TimeDelta auto_close_duration);

  // Timer used to auto close the bubble.
  base::OneShotTimer timer_;
  const ActivationAction activation_action_;

  DISALLOW_COPY_AND_ASSIGN(FeaturePromoBubbleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_FEATURE_PROMO_BUBBLE_VIEW_H_
