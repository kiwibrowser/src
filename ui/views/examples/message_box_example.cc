// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/examples/message_box_example.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/message_box_view.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/view.h"

using base::ASCIIToUTF16;

namespace views {
namespace examples {

MessageBoxExample::MessageBoxExample() : ExampleBase("Message Box View") {
}

MessageBoxExample::~MessageBoxExample() {
}

void MessageBoxExample::CreateExampleView(View* container) {
  message_box_view_ = new MessageBoxView(
      MessageBoxView::InitParams(ASCIIToUTF16("Hello, world!")));
  status_ = new LabelButton(this, ASCIIToUTF16("Show Status"));
  toggle_ = new LabelButton(this, ASCIIToUTF16("Toggle Checkbox"));

  GridLayout* layout = container->SetLayoutManager(
      std::make_unique<views::GridLayout>(container));

  message_box_view_->SetCheckBoxLabel(ASCIIToUTF16("Check Box"));

  const int message_box_column = 0;
  ColumnSet* column_set = layout->AddColumnSet(message_box_column);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL, 1,
                        GridLayout::USE_PREF, 0, 0);
  layout->StartRow(1 /* expand */, message_box_column);
  layout->AddView(message_box_view_);

  const int button_column = 1;
  column_set = layout->AddColumnSet(button_column);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL,
                        0.5f, GridLayout::USE_PREF, 0, 0);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL,
                        0.5f, GridLayout::USE_PREF, 0, 0);

  layout->StartRow(0 /* no expand */, button_column);

  layout->AddView(status_);
  layout->AddView(toggle_);
}

void MessageBoxExample::ButtonPressed(Button* sender, const ui::Event& event) {
  if (sender == status_) {
    message_box_view_->SetCheckBoxLabel(
        ASCIIToUTF16(BoolToOnOff(message_box_view_->IsCheckBoxSelected())));
    PrintStatus(message_box_view_->IsCheckBoxSelected() ?
       "Check Box Selected" : "Check Box Not Selected");
  } else if (sender == toggle_) {
    message_box_view_->SetCheckBoxSelected(
        !message_box_view_->IsCheckBoxSelected());
  }
}

}  // namespace examples
}  // namespace views
