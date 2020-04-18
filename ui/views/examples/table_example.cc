// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/examples/table_example.h"

#include <vector>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/layout/grid_layout.h"

using base::ASCIIToUTF16;

namespace views {
namespace examples {

namespace {

ui::TableColumn TestTableColumn(int id, const std::string& title) {
  ui::TableColumn column;
  column.id = id;
  column.title = ASCIIToUTF16(title.c_str());
  column.sortable = true;
  return column;
}

}  // namespace

TableExample::TableExample() : ExampleBase("Table") , table_(NULL) {
}

TableExample::~TableExample() {
  // Delete the view before the model.
  delete table_;
  table_ = NULL;
}

void TableExample::CreateExampleView(View* container) {
  column1_visible_checkbox_ = new Checkbox(
      ASCIIToUTF16("Fruit column visible"));
  column1_visible_checkbox_->SetChecked(true);
  column1_visible_checkbox_->set_listener(this);
  column2_visible_checkbox_ = new Checkbox(
      ASCIIToUTF16("Color column visible"));
  column2_visible_checkbox_->SetChecked(true);
  column2_visible_checkbox_->set_listener(this);
  column3_visible_checkbox_ = new Checkbox(
      ASCIIToUTF16("Origin column visible"));
  column3_visible_checkbox_->SetChecked(true);
  column3_visible_checkbox_->set_listener(this);
  column4_visible_checkbox_ = new Checkbox(
      ASCIIToUTF16("Price column visible"));
  column4_visible_checkbox_->SetChecked(true);
  column4_visible_checkbox_->set_listener(this);

  GridLayout* layout = container->SetLayoutManager(
      std::make_unique<views::GridLayout>(container));

  std::vector<ui::TableColumn> columns;
  columns.push_back(TestTableColumn(0, "Fruit"));
  columns[0].percent = 1;
  columns.push_back(TestTableColumn(1, "Color"));
  columns.push_back(TestTableColumn(2, "Origin"));
  columns.push_back(TestTableColumn(3, "Price"));
  columns.back().alignment = ui::TableColumn::RIGHT;
  table_ = new TableView(this, columns, ICON_AND_TEXT, true);
  table_->SetGrouper(this);
  table_->set_observer(this);
  icon1_.allocN32Pixels(16, 16);
  SkCanvas canvas1(icon1_);
  canvas1.drawColor(SK_ColorRED);

  icon2_.allocN32Pixels(16, 16);
  SkCanvas canvas2(icon2_);
  canvas2.drawColor(SK_ColorBLUE);

  ColumnSet* column_set = layout->AddColumnSet(0);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL, 1,
                        GridLayout::USE_PREF, 0, 0);
  layout->StartRow(1 /* expand */, 0);
  layout->AddView(table_->CreateParentIfNecessary());

  column_set = layout->AddColumnSet(1);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL,
                        0.5f, GridLayout::USE_PREF, 0, 0);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL,
                        0.5f, GridLayout::USE_PREF, 0, 0);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL,
                        0.5f, GridLayout::USE_PREF, 0, 0);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL,
                        0.5f, GridLayout::USE_PREF, 0, 0);

  layout->StartRow(0 /* no expand */, 1);

  layout->AddView(column1_visible_checkbox_);
  layout->AddView(column2_visible_checkbox_);
  layout->AddView(column3_visible_checkbox_);
  layout->AddView(column4_visible_checkbox_);
}

int TableExample::RowCount() {
  return 10;
}

base::string16 TableExample::GetText(int row, int column_id) {
  if (row == -1)
    return base::string16();

  const char* const cells[5][4] = {
    { "Orange", "Orange", "South america", "$5" },
    { "Apple", "Green", "Canada", "$3" },
    { "Blue berries", "Blue", "Mexico", "$10.3" },
    { "Strawberries", "Red", "California", "$7" },
    { "Cantaloupe", "Orange", "South america", "$5" },
  };
  return ASCIIToUTF16(cells[row % 5][column_id]);
}

gfx::ImageSkia TableExample::GetIcon(int row) {
  SkBitmap row_icon = row % 2 ? icon1_ : icon2_;
  return gfx::ImageSkia::CreateFrom1xBitmap(row_icon);
}

void TableExample::SetObserver(ui::TableModelObserver* observer) {}

void TableExample::GetGroupRange(int model_index, GroupRange* range) {
  if (model_index < 2) {
    range->start = 0;
    range->length = 2;
  } else if (model_index > 6) {
    range->start = 7;
    range->length = 3;
  } else {
    range->start = model_index;
    range->length = 1;
  }
}

void TableExample::OnSelectionChanged() {
  PrintStatus("Selected: %s",
              base::UTF16ToASCII(GetText(table_->selection_model().active(),
                                         0)).c_str());
}

void TableExample::OnDoubleClick() {
  PrintStatus("Double Click: %s",
              base::UTF16ToASCII(GetText(table_->selection_model().active(),
                                         0)).c_str());
}

void TableExample::OnMiddleClick() {}

void TableExample::OnKeyDown(ui::KeyboardCode virtual_keycode) {}

void TableExample::ButtonPressed(Button* sender, const ui::Event& event) {
  int index = 0;
  bool show = true;
  if (sender == column1_visible_checkbox_) {
    index = 0;
    show = column1_visible_checkbox_->checked();
  } else if (sender == column2_visible_checkbox_) {
    index = 1;
    show = column2_visible_checkbox_->checked();
  } else if (sender == column3_visible_checkbox_) {
    index = 2;
    show = column3_visible_checkbox_->checked();
  } else if (sender == column4_visible_checkbox_) {
    index = 3;
    show = column4_visible_checkbox_->checked();
  }
  table_->SetColumnVisibility(index, show);
}

}  // namespace examples
}  // namespace views
