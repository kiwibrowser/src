// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/examples/radio_button_example.h"

#include <stddef.h>

#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/radio_button.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/view.h"

namespace views {
namespace examples {

RadioButtonExample::RadioButtonExample()
    : ExampleBase("Radio Button"),
      count_(0) {
}

RadioButtonExample::~RadioButtonExample() {
}

void RadioButtonExample::CreateExampleView(View* container) {
  select_ = new LabelButton(this, base::ASCIIToUTF16("Select"));
  status_ = new LabelButton(this, base::ASCIIToUTF16("Show Status"));

  int group = 1;
  for (size_t i = 0; i < arraysize(radio_buttons_); ++i) {
    radio_buttons_[i] = new RadioButton(
        base::UTF8ToUTF16(base::StringPrintf(
            "Radio %d in group %d", static_cast<int>(i) + 1, group)),
        group);
    radio_buttons_[i]->set_listener(this);
  }

  GridLayout* layout = container->SetLayoutManager(
      std::make_unique<views::GridLayout>(container));

  ColumnSet* column_set = layout->AddColumnSet(0);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL,
                        1.0f, GridLayout::USE_PREF, 0, 0);
  for (size_t i = 0; i < arraysize(radio_buttons_); ++i) {
    layout->StartRow(0, 0);
    layout->AddView(radio_buttons_[i]);
  }
  layout->StartRow(0, 0);
  layout->AddView(select_);
  layout->StartRow(0, 0);
  layout->AddView(status_);
}

void RadioButtonExample::ButtonPressed(Button* sender, const ui::Event& event) {
  if (sender == select_) {
    radio_buttons_[2]->SetChecked(true);
  } else if (sender == status_) {
    // Show the state of radio buttons.
    PrintStatus("Group: 1:%s, 2:%s, 3:%s",
                BoolToOnOff(radio_buttons_[0]->checked()),
                BoolToOnOff(radio_buttons_[1]->checked()),
                BoolToOnOff(radio_buttons_[2]->checked()));
  } else {
    PrintStatus("Pressed! count:%d", ++count_);
  }
}

}  // namespace examples
}  // namespace views
