// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/examples/textfield_example.h"

#include <stddef.h>

#include "base/strings/utf_string_conversions.h"
#include "ui/events/event.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/range/range.h"
#include "ui/gfx/render_text.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/view.h"

using base::ASCIIToUTF16;
using base::UTF16ToUTF8;

namespace views {
namespace examples {

TextfieldExample::TextfieldExample()
    : ExampleBase("Textfield"),
      name_(nullptr),
      password_(nullptr),
      disabled_(nullptr),
      read_only_(nullptr),
      invalid_(nullptr),
      rtl_(nullptr),
      show_password_(nullptr),
      clear_all_(nullptr),
      append_(nullptr),
      set_(nullptr),
      set_style_(nullptr) {}

TextfieldExample::~TextfieldExample() {}

void TextfieldExample::CreateExampleView(View* container) {
  name_ = new Textfield();
  password_ = new Textfield();
  password_->SetTextInputType(ui::TEXT_INPUT_TYPE_PASSWORD);
  password_->set_placeholder_text(ASCIIToUTF16("password"));
  disabled_ = new Textfield();
  disabled_->SetEnabled(false);
  disabled_->SetText(ASCIIToUTF16("disabled"));
  read_only_ = new Textfield();
  read_only_->SetReadOnly(true);
  read_only_->SetText(ASCIIToUTF16("read only"));
  invalid_ = new Textfield();
  invalid_->SetInvalid(true);
  rtl_ = new Textfield();
  rtl_->ChangeTextDirectionAndLayoutAlignment(base::i18n::RIGHT_TO_LEFT);
  show_password_ = new LabelButton(this, ASCIIToUTF16("Show password"));
  set_background_ =
      new LabelButton(this, ASCIIToUTF16("Set non-default background"));
  clear_all_ = new LabelButton(this, ASCIIToUTF16("Clear All"));
  append_ = new LabelButton(this, ASCIIToUTF16("Append"));
  set_ = new LabelButton(this, ASCIIToUTF16("Set"));
  set_style_ = new LabelButton(this, ASCIIToUTF16("Set Styles"));
  name_->set_controller(this);
  password_->set_controller(this);

  GridLayout* layout = container->SetLayoutManager(
      std::make_unique<views::GridLayout>(container));

  ColumnSet* column_set = layout->AddColumnSet(0);
  column_set->AddColumn(GridLayout::LEADING, GridLayout::FILL,
                        0.2f, GridLayout::USE_PREF, 0, 0);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL,
                        0.8f, GridLayout::USE_PREF, 0, 0);

  auto MakeRow = [layout](View* view1, View* view2) {
    layout->StartRowWithPadding(0, 0, 0, 5);
    layout->AddView(view1);
    if (view2)
      layout->AddView(view2);
  };
  MakeRow(new Label(ASCIIToUTF16("Name:")), name_);
  MakeRow(new Label(ASCIIToUTF16("Password:")), password_);
  MakeRow(new Label(ASCIIToUTF16("Disabled:")), disabled_);
  MakeRow(new Label(ASCIIToUTF16("Read Only:")), read_only_);
  MakeRow(new Label(ASCIIToUTF16("Invalid:")), invalid_);
  MakeRow(new Label(ASCIIToUTF16("RTL:")), rtl_);
  MakeRow(new Label(ASCIIToUTF16("Name:")), nullptr);
  MakeRow(show_password_, nullptr);
  MakeRow(set_background_, nullptr);
  MakeRow(clear_all_, nullptr);
  MakeRow(append_, nullptr);
  MakeRow(set_, nullptr);
  MakeRow(set_style_, nullptr);
}

void TextfieldExample::ContentsChanged(Textfield* sender,
                                       const base::string16& new_contents) {
  if (sender == name_) {
    PrintStatus("Name [%s]", UTF16ToUTF8(new_contents).c_str());
  } else if (sender == password_) {
    PrintStatus("Password [%s]", UTF16ToUTF8(new_contents).c_str());
  } else {
    NOTREACHED();
  }
}

bool TextfieldExample::HandleKeyEvent(Textfield* sender,
                                      const ui::KeyEvent& key_event) {
  return false;
}

bool TextfieldExample::HandleMouseEvent(Textfield* sender,
                                        const ui::MouseEvent& mouse_event) {
  PrintStatus("HandleMouseEvent click count=%d", mouse_event.GetClickCount());
  return false;
}

void TextfieldExample::ButtonPressed(Button* sender, const ui::Event& event) {
  if (sender == show_password_) {
    PrintStatus("Password [%s]", UTF16ToUTF8(password_->text()).c_str());
  } else if (sender == set_background_) {
    password_->SetBackgroundColor(gfx::kGoogleRed300);
  } else if (sender == clear_all_) {
    base::string16 empty;
    name_->SetText(empty);
    password_->SetText(empty);
    disabled_->SetText(empty);
    read_only_->SetText(empty);
    invalid_->SetText(empty);
    rtl_->SetText(empty);
  } else if (sender == append_) {
    name_->AppendText(ASCIIToUTF16("[append]"));
    password_->AppendText(ASCIIToUTF16("[append]"));
    disabled_->SetText(ASCIIToUTF16("[append]"));
    read_only_->AppendText(ASCIIToUTF16("[append]"));
    invalid_->AppendText(ASCIIToUTF16("[append]"));
    rtl_->AppendText(ASCIIToUTF16("[append]"));
  } else if (sender == set_) {
    name_->SetText(ASCIIToUTF16("[set]"));
    password_->SetText(ASCIIToUTF16("[set]"));
    disabled_->SetText(ASCIIToUTF16("[set]"));
    read_only_->SetText(ASCIIToUTF16("[set]"));
    invalid_->SetText(ASCIIToUTF16("[set]"));
    rtl_->SetText(ASCIIToUTF16("[set]"));
  } else if (sender == set_style_) {
    if (!name_->text().empty()) {
      name_->SetColor(SK_ColorGREEN);

      if (name_->text().length() >= 5) {
        size_t fifth = name_->text().length() / 5;
        const gfx::Range big_range(1 * fifth, 4 * fifth);
        name_->ApplyStyle(gfx::UNDERLINE, true, big_range);
        name_->ApplyColor(SK_ColorBLUE, big_range);

        const gfx::Range small_range(2 * fifth, 3 * fifth);
        name_->ApplyStyle(gfx::ITALIC, true, small_range);
        name_->ApplyStyle(gfx::UNDERLINE, false, small_range);
        name_->ApplyColor(SK_ColorRED, small_range);
      }
    }
  }
}

}  // namespace examples
}  // namespace views
