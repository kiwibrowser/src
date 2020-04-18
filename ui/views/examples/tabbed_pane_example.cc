// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/examples/tabbed_pane_example.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/tabbed_pane/tabbed_pane.h"
#include "ui/views/layout/grid_layout.h"

using base::ASCIIToUTF16;

namespace views {
namespace examples {

TabbedPaneExample::TabbedPaneExample() : ExampleBase("Tabbed Pane") {
}

TabbedPaneExample::~TabbedPaneExample() {
}

void TabbedPaneExample::CreateExampleView(View* container) {
  tabbed_pane_ = new TabbedPane();
  tabbed_pane_->set_listener(this);
  add_ = new LabelButton(this, ASCIIToUTF16("Add"));
  add_at_ = new LabelButton(this, ASCIIToUTF16("Add At 1"));
  select_at_ = new LabelButton(this, ASCIIToUTF16("Select At 1"));

  GridLayout* layout = container->SetLayoutManager(
      std::make_unique<views::GridLayout>(container));

  const int tabbed_pane_column = 0;
  ColumnSet* column_set = layout->AddColumnSet(tabbed_pane_column);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL,
                        1.0f, GridLayout::USE_PREF, 0, 0);
  layout->StartRow(1 /* expand */, tabbed_pane_column);
  layout->AddView(tabbed_pane_);

  // Create a few tabs with a button first.
  AddButton("Tab 1");
  AddButton("Tab 2");
  AddButton("Tab 3");

  // Add control buttons horizontally.
  const int button_column = 1;
  column_set = layout->AddColumnSet(button_column);
  for (int i = 0; i < 3; i++) {
    column_set->AddColumn(GridLayout::FILL, GridLayout::FILL,
                          1.0f, GridLayout::USE_PREF, 0, 0);
  }

  layout->StartRow(0 /* no expand */, button_column);
  layout->AddView(add_);
  layout->AddView(add_at_);
  layout->AddView(select_at_);
}

void TabbedPaneExample::ButtonPressed(Button* sender, const ui::Event& event) {
  if (sender == add_) {
    AddButton("Added");
  } else if (sender == add_at_) {
    const base::string16 label = ASCIIToUTF16("Added at 1");
    tabbed_pane_->AddTabAtIndex(1, label, new LabelButton(NULL, label));
  } else if (sender == select_at_) {
    if (tabbed_pane_->GetTabCount() > 1)
      tabbed_pane_->SelectTabAt(1);
  }
  PrintStatus();
}

void TabbedPaneExample::TabSelectedAt(int index) {
  // Just print the status when selection changes.
  PrintStatus();
}

void TabbedPaneExample::PrintStatus() {
  ExampleBase::PrintStatus("Tab Count:%d, Selected Tab:%d",
                           tabbed_pane_->GetTabCount(),
                           tabbed_pane_->GetSelectedTabIndex());
}

void TabbedPaneExample::AddButton(const std::string& label) {
  LabelButton* button = new LabelButton(NULL, ASCIIToUTF16(label));
  tabbed_pane_->AddTab(ASCIIToUTF16(label), button);
}

}  // namespace examples
}  // namespace views
