// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/examples/progress_bar_example.h"

#include <algorithm>

#include "base/strings/utf_string_conversions.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/progress_bar.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/view.h"

namespace {

const double kStepSize = 0.1;

double SetToMax(double percent) {
  return std::min(std::max(percent, 0.0), 1.0);
}

}  // namespace

namespace views {
namespace examples {

ProgressBarExample::ProgressBarExample()
    : ExampleBase("Progress Bar"),
      minus_button_(nullptr),
      plus_button_(nullptr),
      progress_bar_(nullptr),
      current_percent_(0.0) {}

ProgressBarExample::~ProgressBarExample() {
}

void ProgressBarExample::CreateExampleView(View* container) {
  GridLayout* layout = container->SetLayoutManager(
      std::make_unique<views::GridLayout>(container));

  ColumnSet* column_set = layout->AddColumnSet(0);
  column_set->AddColumn(GridLayout::TRAILING, GridLayout::CENTER, 0,
                        GridLayout::USE_PREF, 0, 0);
  column_set->AddPaddingColumn(0, 8);
  column_set->AddColumn(GridLayout::FILL, GridLayout::CENTER, 1,
                        GridLayout::FIXED, 200, 0);
  column_set->AddPaddingColumn(0, 8);
  column_set->AddColumn(GridLayout::LEADING, GridLayout::CENTER, 0,
                        GridLayout::USE_PREF, 0, 0);

  layout->StartRow(0, 0);
  minus_button_ = MdTextButton::Create(this, base::ASCIIToUTF16("-"));
  layout->AddView(minus_button_);
  progress_bar_ = new ProgressBar();
  layout->AddView(progress_bar_);
  plus_button_ = MdTextButton::Create(this, base::ASCIIToUTF16("+"));
  layout->AddView(plus_button_);

  layout->StartRowWithPadding(0, 0, 0, 10);
  layout->AddView(new Label(base::ASCIIToUTF16("Infinite loader:")));
  ProgressBar* infinite_bar = new ProgressBar();
  infinite_bar->SetValue(-1);
  layout->AddView(infinite_bar);

  layout->StartRowWithPadding(0, 0, 0, 10);
  layout->AddView(
      new Label(base::ASCIIToUTF16("Infinite loader (very short):")));
  ProgressBar* shorter_bar = new ProgressBar(2);
  shorter_bar->SetValue(-1);
  layout->AddView(shorter_bar);
}

void ProgressBarExample::ButtonPressed(Button* sender, const ui::Event& event) {
  if (sender == minus_button_)
    current_percent_ = SetToMax(current_percent_ - kStepSize);
  else if (sender == plus_button_)
    current_percent_ = SetToMax(current_percent_ + kStepSize);

  progress_bar_->SetValue(current_percent_);
}

}  // namespace examples
}  // namespace views
