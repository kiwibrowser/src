// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/utf_string_conversions.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace shell {

struct BubbleConfig {
  base::string16 label;
  views::View* anchor_view;
  views::BubbleBorder::Arrow arrow;
};

class ExampleBubbleDialogDelegateView : public views::BubbleDialogDelegateView {
 public:
  explicit ExampleBubbleDialogDelegateView(const BubbleConfig& config);
  ~ExampleBubbleDialogDelegateView() override;

  void Init() override {
    SetLayoutManager(std::make_unique<views::FillLayout>());
    views::Label* label = new views::Label(label_);
    AddChildView(label);
  }

 private:
  base::string16 label_;
};

ExampleBubbleDialogDelegateView::ExampleBubbleDialogDelegateView(
    const BubbleConfig& config)
    : BubbleDialogDelegateView(config.anchor_view, config.arrow),
      label_(config.label) {}

ExampleBubbleDialogDelegateView::~ExampleBubbleDialogDelegateView() = default;

void CreatePointyBubble(views::View* anchor_view) {
  BubbleConfig config;
  config.label = base::ASCIIToUTF16("Pointy Dialog Bubble");
  config.anchor_view = anchor_view;
  config.arrow = views::BubbleBorder::TOP_LEFT;
  ExampleBubbleDialogDelegateView* bubble =
      new ExampleBubbleDialogDelegateView(config);
  views::BubbleDialogDelegateView::CreateBubble(bubble)->Show();
}

}  // namespace shell
}  // namespace ash
