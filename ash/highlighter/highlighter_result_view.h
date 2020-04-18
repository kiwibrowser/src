// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_HIGHLIGHTER_HIGHLIGHTER_RESULT_VIEW_H_
#define ASH_HIGHLIGHTER_HIGHLIGHTER_RESULT_VIEW_H_

#include <memory>

#include "base/macros.h"
#include "base/time/time.h"
#include "ui/views/view.h"

namespace aura {
class Window;
}

namespace ui {
class Layer;
}

namespace views {
class Widget;
}

namespace ash {

enum class HighlighterGestureType;

// HighlighterResultView displays an animated shape that represents
// the result of the selection.
class HighlighterResultView : public views::View {
 public:
  HighlighterResultView(aura::Window* root_window);

  ~HighlighterResultView() override;

  void Animate(const gfx::RectF& bounds,
               HighlighterGestureType gesture_type,
               const base::Closure& done);

 private:
  void FadeIn(const base::TimeDelta& duration, const base::Closure& done);
  void FadeOut(const base::Closure& done);

  std::unique_ptr<views::Widget> widget_;
  std::unique_ptr<ui::Layer> result_layer_;
  std::unique_ptr<base::OneShotTimer> animation_timer_;

  DISALLOW_COPY_AND_ASSIGN(HighlighterResultView);
};

}  // namespace ash

#endif  // ASH_HIGHLIGHTER_HIGHLIGHTER_RESULT_VIEW_H_
