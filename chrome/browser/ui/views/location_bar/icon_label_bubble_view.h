// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_ICON_LABEL_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_ICON_LABEL_BUBBLE_VIEW_H_

#include <string>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/animation/ink_drop_host_view.h"
#include "ui/views/animation/ink_drop_observer.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/label.h"
#include "ui/views/widget/widget_observer.h"

namespace gfx {
class FontList;
class ImageSkia;
}

namespace views {
class ImageView;
class InkDropContainerView;
}

// View used to draw a bubble, containing an icon and a label. We use this as a
// base for the classes that handle the location icon (including the EV bubble),
// tab-to-search UI, and content settings.
class IconLabelBubbleView : public views::InkDropObserver,
                            public views::Button,
                            public views::WidgetObserver {
 public:
  static constexpr int kTrailingPaddingPreMd = 2;

  // A view that draws the separator.
  class SeparatorView : public views::View,
                        public ui::ImplicitAnimationObserver {
   public:
    explicit SeparatorView(IconLabelBubbleView* owner);

    // views::View:
    void OnPaint(gfx::Canvas* canvas) override;

    // ui::ImplicitAnimationObserver:
    void OnImplicitAnimationsCompleted() override;

    // Updates the opacity based on the ink drop's state.
    void UpdateOpacity();

    void set_disable_animation_for_test(bool disable_animation_for_test) {
      disable_animation_for_test_ = disable_animation_for_test;
    }

   private:
    // Weak.
    IconLabelBubbleView* owner_;

    bool disable_animation_for_test_ = false;

    DISALLOW_COPY_AND_ASSIGN(SeparatorView);
  };

  explicit IconLabelBubbleView(const gfx::FontList& font_list);
  ~IconLabelBubbleView() override;

  // views::InkDropObserver:
  void InkDropAnimationStarted() override;
  void InkDropRippleAnimationEnded(views::InkDropState state) override;

  void SetLabel(const base::string16& label);
  void SetImage(const gfx::ImageSkia& image);

  const views::ImageView* GetImageView() const { return image_; }
  views::ImageView* GetImageView() { return image_; }

  // Exposed for testing.
  SeparatorView* separator_view() const { return separator_view_; }

  void set_next_element_interior_padding(int padding) {
    next_element_interior_padding_ = padding;
  }

  void OnBubbleCreated(views::Widget* bubble_widget);

 protected:
  static constexpr int kOpenTimeMS = 150;

  views::ImageView* image() { return image_; }
  const views::ImageView* image() const { return image_; }
  views::Label* label() { return label_; }
  const views::Label* label() const { return label_; }
  const views::InkDropContainerView* ink_drop_container() const {
    return ink_drop_container_;
  }

  // Gets the color for displaying text.
  virtual SkColor GetTextColor() const = 0;

  // Returns true when the label should be visible.
  virtual bool ShouldShowLabel() const;

  // Returns true when the separator should be visible.
  virtual bool ShouldShowSeparator() const;

  // Returns a multiplier used to calculate the actual width of the view based
  // on its desired width.  This ranges from 0 for a zero-width view to 1 for a
  // full-width view and can be used to animate the width of the view.
  virtual double WidthMultiplier() const;

  // Returns true when animation is in progress and is shrinking.
  virtual bool IsShrinking() const;

  // Returns true if a bubble was shown.
  virtual bool ShowBubble(const ui::Event& event);

  // Returns true if the bubble anchored to the icon is shown. This is to
  // prevent the bubble from reshowing on a mouse release.
  virtual bool IsBubbleShowing() const;

  // views::InkDropHostView:
  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;
  void OnNativeThemeChanged(const ui::NativeTheme* native_theme) override;
  void AddInkDropLayer(ui::Layer* ink_drop_layer) override;
  void RemoveInkDropLayer(ui::Layer* ink_drop_layer) override;
  std::unique_ptr<views::InkDrop> CreateInkDrop() override;
  std::unique_ptr<views::InkDropRipple> CreateInkDropRipple() const override;
  std::unique_ptr<views::InkDropHighlight> CreateInkDropHighlight()
      const override;
  SkColor GetInkDropBaseColor() const override;
  std::unique_ptr<views::InkDropMask> CreateInkDropMask() const override;

  // views::Button:
  bool IsTriggerableEvent(const ui::Event& event) override;
  bool ShouldUpdateInkDropOnClickCanceled() const override;
  void NotifyClick(const ui::Event& event) override;

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;
  void OnWidgetVisibilityChanged(views::Widget* widget, bool visible) override;

  const gfx::FontList& font_list() const { return label_->font_list(); }

  SkColor GetParentBackgroundColor() const;

  gfx::Size GetSizeForLabelWidth(int label_width) const;

  // Returns the maximum size for the label width. The value ignores
  // WidthMultiplier().
  gfx::Size GetMaxSizeForLabelWidth(int label_width) const;

 private:
  // Amount of padding from the leading edge of the view to the leading edge of
  // the image, and from the trailing edge of the label (or image, if the label
  // is invisible) to the trailing edge of the view.
  int GetOuterPadding() const;

  // Spacing between the image and the label.
  int GetInternalSpacing() const;

  // Returns the amount of space reserved for the separator in DIP.
  int GetSeparatorLayoutWidth() const;

  // Retrieves the width taken the separator including padding before the
  // separator stroke, taking into account whether it is shown or not.
  int GetPrefixedSeparatorWidth() const;

  // Padding after the separator.
  int GetEndPadding() const;

  // Gets the minimum size to use when the label is not shown.
  gfx::Size GetNonLabelSize() const;

  float GetScaleFactor() const;

  // The view has been activated by a user gesture such as spacebar.
  // Returns true if some handling was performed.
  bool OnActivate(const ui::Event& event);

  // views::View:
  const char* GetClassName() const override;

  // The contents of the bubble.
  views::ImageView* image_;
  views::Label* label_;
  views::InkDropContainerView* ink_drop_container_;
  SeparatorView* separator_view_;

  // The padding of the element that will be displayed after |this|. This value
  // is relevant for calculating the amount of space to reserve after the
  // separator.
  int next_element_interior_padding_ = 0;

  // This is used to check if the bubble was showing in the last mouse press
  // event. If this is true then IsTriggerableEvent() will return false to
  // prevent the bubble from reshowing. This flag is necessary because the
  // bubble gets dismissed before the button handles the mouse release event.
  bool suppress_button_release_;

  DISALLOW_COPY_AND_ASSIGN(IconLabelBubbleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_ICON_LABEL_BUBBLE_VIEW_H_
