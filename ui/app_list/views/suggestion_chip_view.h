// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_SUGGESTION_CHIP_VIEW_H_
#define UI_APP_LIST_VIEWS_SUGGESTION_CHIP_VIEW_H_

#include "base/macros.h"
#include "base/optional.h"
#include "ui/app_list/app_list_export.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class ImageView;
class Label;
}  // namespace views

namespace app_list {

class SuggestionChipView;

// Listener which receives notification of suggestion chip events.
class APP_LIST_EXPORT SuggestionChipListener {
 public:
  // Invoked when the specified |sender| is pressed.
  virtual void OnSuggestionChipPressed(SuggestionChipView* sender) = 0;

 protected:
  virtual ~SuggestionChipListener() = default;
};

// View representing a suggestion chip.
class APP_LIST_EXPORT SuggestionChipView : public views::View {
 public:
  static constexpr int kIconSizeDip = 16;

  // Initialization parameters.
  struct Params {
    Params();
    ~Params();

    // Display text.
    base::string16 text;
    // Optional icon.
    base::Optional<gfx::ImageSkia> icon;
  };

  SuggestionChipView(const Params& params,
                     SuggestionChipListener* listener = nullptr);
  ~SuggestionChipView() override;

  // views::View:
  gfx::Size CalculatePreferredSize() const override;
  void ChildVisibilityChanged(views::View* child) override;
  int GetHeightForWidth(int width) const override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnPaintBackground(gfx::Canvas* canvas) override;

  void SetIcon(const gfx::ImageSkia& icon);

  const base::string16& GetText() const;

 private:
  void InitLayout(const Params& params);

  views::ImageView* icon_view_;  // Owned by view hierarchy.
  views::Label* text_view_;  // Owned by view hierarchy.
  SuggestionChipListener* listener_;

  views::BoxLayout* layout_manager_;  // Owned by view hierarchy.

  DISALLOW_COPY_AND_ASSIGN(SuggestionChipView);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_SUGGESTION_CHIP_VIEW_H_
