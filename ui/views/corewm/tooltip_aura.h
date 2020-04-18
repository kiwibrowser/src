// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_COREWM_TOOLTIP_AURA_H_
#define UI_VIEWS_COREWM_TOOLTIP_AURA_H_

#include <memory>

#include "base/macros.h"
#include "ui/views/corewm/tooltip.h"
#include "ui/views/widget/widget_observer.h"

namespace gfx {
class RenderText;
class Size;
}  // namespace gfx

namespace views {

class Widget;

namespace corewm {
namespace test {
class TooltipAuraTestApi;
}

// Implementation of Tooltip that shows the tooltip using a Widget and Label.
class VIEWS_EXPORT TooltipAura : public Tooltip, public WidgetObserver {
 public:
  TooltipAura();
  ~TooltipAura() override;

 private:
  class TooltipView;

  friend class test::TooltipAuraTestApi;
  gfx::RenderText* GetRenderTextForTest();

  // Adjusts the bounds given by the arguments to fit inside the desktop
  // and applies the adjusted bounds to the label_.
  void SetTooltipBounds(const gfx::Point& mouse_pos,
                        const gfx::Size& tooltip_size);

  // Destroys |widget_|.
  void DestroyWidget();

  // Tooltip:
  int GetMaxWidth(const gfx::Point& location) const override;
  void SetText(aura::Window* window,
               const base::string16& tooltip_text,
               const gfx::Point& location) override;
  void Show() override;
  void Hide() override;
  bool IsVisible() override;

  // WidgetObserver:
  void OnWidgetDestroying(Widget* widget) override;

  // The view showing the tooltip.
  std::unique_ptr<TooltipView> tooltip_view_;

  // The widget containing the tooltip. May be NULL.
  Widget* widget_;

  // The window we're showing the tooltip for. Never NULL and valid while
  // showing.
  aura::Window* tooltip_window_;

  DISALLOW_COPY_AND_ASSIGN(TooltipAura);
};

}  // namespace corewm
}  // namespace views

#endif  // UI_VIEWS_COREWM_TOOLTIP_AURA_H_
