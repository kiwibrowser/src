// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/examples/combobox_example.h"

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/layout/box_layout.h"

namespace views {
namespace examples {

ComboboxModelExample::ComboboxModelExample() {
}

ComboboxModelExample::~ComboboxModelExample() {
}

int ComboboxModelExample::GetItemCount() const {
  return 10;
}

base::string16 ComboboxModelExample::GetItemAt(int index) {
  return base::UTF8ToUTF16(base::StringPrintf("%c item", 'A' + index));
}

ComboboxExample::ComboboxExample() : ExampleBase("Combo Box") {
}

ComboboxExample::~ComboboxExample() {
  // Delete |combobox_| first as it references |combobox_model_|.
  delete combobox_;
  delete disabled_combobox_;
  delete action_combobox_;
  combobox_ = nullptr;
  disabled_combobox_ = nullptr;
  action_combobox_ = nullptr;
}

void ComboboxExample::CreateExampleView(View* container) {
  combobox_ = new Combobox(&combobox_model_);
  combobox_->set_listener(this);
  combobox_->SetSelectedIndex(3);

  disabled_combobox_ = new Combobox(&combobox_model_);
  disabled_combobox_->set_listener(this);
  disabled_combobox_->SetSelectedIndex(4);
  disabled_combobox_->SetEnabled(false);

  action_combobox_ = new Combobox(&combobox_model_, Combobox::STYLE_ACTION);
  action_combobox_->set_listener(this);
  // Note: STYLE_ACTION comboboxes always have the first item selected by
  // default.

  container->SetLayoutManager(
      std::make_unique<BoxLayout>(BoxLayout::kVertical, gfx::Insets(10, 0), 5));
  container->AddChildView(combobox_);
  container->AddChildView(disabled_combobox_);
  container->AddChildView(action_combobox_);
}

void ComboboxExample::OnPerformAction(Combobox* combobox) {
  if (combobox == combobox_) {
    PrintStatus("Selected: %s", base::UTF16ToUTF8(combobox_model_.GetItemAt(
        combobox->selected_index())).c_str());
  } else if (combobox == action_combobox_) {
    PrintStatus("Action: %s", base::UTF16ToUTF8(combobox_model_.GetItemAt(
        combobox->selected_index())).c_str());
  } else {
    NOTREACHED() << "Surprising combobox.";
  }
}

}  // namespace examples
}  // namespace views
