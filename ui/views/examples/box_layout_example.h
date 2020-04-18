// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_EXAMPLES_BOX_LAYOUT_EXAMPLE_H_
#define UI_VIEWS_EXAMPLES_BOX_LAYOUT_EXAMPLE_H_

#include "base/macros.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/combobox/combobox_listener.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/examples/example_base.h"
#include "ui/views/layout/box_layout.h"

namespace views {

class Checkbox;
class Combobox;
class Textfield;

namespace examples {

namespace {

class ChildPanel;
}

class VIEWS_EXAMPLES_EXPORT BoxLayoutExample : public ExampleBase,
                                               public ButtonListener,
                                               public ComboboxListener,
                                               public TextfieldController {
 public:
  BoxLayoutExample();
  ~BoxLayoutExample() override;

  // ExampleBase
  void CreateExampleView(View* container) override;

 private:
  friend views::examples::ChildPanel;
  // ButtonListener
  void ButtonPressed(Button* sender, const ui::Event& event) override;

  // ComboboxListener
  void OnPerformAction(Combobox* combobox) override;

  // TextfieldController
  void ContentsChanged(Textfield* sender,
                       const base::string16& new_contents) override;

  // Force the box_layout_panel_ to layout and repaint.
  void RefreshLayoutPanel();

  // Set the border insets on the current BoxLayout instance.
  void UpdateBorderInsets();

  // Create a new BoxLayout and ensure all settings match the current state of
  // the various control_panel_ controls.
  void UpdateLayoutManager();

  // Create a Combobox with a label with |label_text| to the left. Adjust
  // |vertical_pos| to |vertical_pos| + combo_box->height() + kSpacing.
  Combobox* CreateCombobox(const base::string16& label_text,
                           const char** items,
                           int count,
                           int& vertical_pos);

  // Create just a Textfield at the current position of |horizontal_pos| and
  // |vertical_pos|. Update |horizontal_pos| to |horizontal_pos| +
  // text_field->width() + kSpacing.
  Textfield* CreateRawTextfield(int& horizontal_pos,
                                int vertical_pos,
                                bool add);

  // Create a Textfield with a label with |label_text| to the left. Adjust
  // |vertical_pos| to |vertical_pos| + combo_box->height() + kSpacing.
  Textfield* CreateTextfield(const base::string16& label_text,
                             int& vertical_pos);

  // Returns the current values contained in the child_panel_size_[] Textfields
  // as a gfx::Size. If either value is negative or not a valid integer,
  // default values are returned, 180 x 90 for width and height, respectively.
  gfx::Size GetChildPanelSize() const;

  BoxLayout* layout_ = nullptr;
  View* full_panel_ = nullptr;
  View* box_layout_panel_ = nullptr;
  View* control_panel_ = nullptr;
  LabelButton* add_button_ = nullptr;
  Combobox* orientation_ = nullptr;
  Combobox* main_axis_alignment_ = nullptr;
  Combobox* cross_axis_alignment_ = nullptr;
  Textfield* between_child_spacing_ = nullptr;
  Textfield* default_flex_ = nullptr;
  Textfield* min_cross_axis_size_ = nullptr;
  Textfield* border_insets_[4] = {nullptr, nullptr, nullptr, nullptr};
  Textfield* child_panel_size_[2] = {nullptr, nullptr};
  Checkbox* collapse_margins_ = nullptr;
  int panel_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(BoxLayoutExample);
};

}  // namespace examples
}  // namespace views
#endif  // UI_VIEWS_EXAMPLES_BOX_LAYOUT_EXAMPLE_H_
