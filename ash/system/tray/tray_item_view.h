// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_TRAY_ITEM_VIEW_H_
#define ASH_SYSTEM_TRAY_TRAY_ITEM_VIEW_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/views/view.h"

namespace gfx {
class SlideAnimation;
}

namespace views {
class ImageView;
class Label;
}

namespace ash {
class SystemTrayItem;

// Base-class for items in the tray. It makes sure the widget is updated
// correctly when the visibility/size of the tray item changes. It also adds
// animation when showing/hiding the item in the tray.
class ASH_EXPORT TrayItemView : public views::View,
                                public gfx::AnimationDelegate {
 public:
  explicit TrayItemView(SystemTrayItem* owner);
  ~TrayItemView() override;

  // Convenience function for creating a child Label or ImageView.
  // Only one of the two should be called.
  void CreateLabel();
  void CreateImageView();

  SystemTrayItem* owner() const { return owner_; }
  views::Label* label() const { return label_; }
  views::ImageView* image_view() const { return image_view_; }

  // Overridden from views::View.
  void SetVisible(bool visible) override;
  gfx::Size CalculatePreferredSize() const override;
  int GetHeightForWidth(int width) const override;

 protected:
  // The default animation duration is 200ms. But each view can customize this.
  virtual int GetAnimationDurationMS();

 private:
  // Overridden from views::View.
  void ChildPreferredSizeChanged(View* child) override;

  // Overridden from gfx::AnimationDelegate.
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;
  void AnimationCanceled(const gfx::Animation* animation) override;

  SystemTrayItem* owner_;
  std::unique_ptr<gfx::SlideAnimation> animation_;
  // Only one of |label_| and |image_view_| should be non-null.
  views::Label* label_;
  views::ImageView* image_view_;

  DISALLOW_COPY_AND_ASSIGN(TrayItemView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_TRAY_ITEM_VIEW_H_
