// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "box_layout_example.h"

#include <vector>

#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/models/combobox_model.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/examples/example_combobox_model.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/view_properties.h"

namespace views {
namespace examples {

namespace {

// This View holds two other views which consists of a view on the left onto
// which the BoxLayout is attached for demonstrating its features. The view
// on the right contains all the various controls which allow the user to
// interactively control the various features/properties of BoxLayout. Layout()
// will ensure the left view takes 75% and the right view fills the remaining
// 25%.
class FullPanel : public View {
 public:
  FullPanel() {}
  ~FullPanel() override {}

  // View
  void Layout() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FullPanel);
};

// This view is created and added to the left-side view in the FullPanel each
// time the "Add" button is pressed. It also will display Textfield controls
// when the mouse is pressed over the view. These Textfields allow the user to
// interactively set each margin and the "flex" for the given view.
class ChildPanel : public View, public TextfieldController {
 public:
  explicit ChildPanel(BoxLayoutExample* example, gfx::Size preferred_size);
  ~ChildPanel() override {}

  // View
  gfx::Size CalculatePreferredSize() const override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void Layout() override;

  void SetSelected(bool value);
  bool selected() const { return selected_; };

  int GetFlex();

 private:
  // TextfieldController
  void ContentsChanged(Textfield* sender,
                       const base::string16& new_contents) override;

  Textfield* CreateTextfield();

  BoxLayoutExample* example_;
  bool selected_ = false;
  Textfield* flex_;
  Textfield* margin_[4];
  gfx::Size preferred_size_;

  DISALLOW_COPY_AND_ASSIGN(ChildPanel);
};

void FullPanel::Layout() {
  DCHECK_EQ(child_count(), 2);
  View* left_panel = child_at(0);
  View* right_panel = child_at(1);
  gfx::Rect bounds = GetContentsBounds();
  left_panel->SetBounds(bounds.x(), bounds.y(), (bounds.width() * 75) / 100,
                        bounds.height());
  right_panel->SetBounds(left_panel->width(), bounds.y(),
                         bounds.width() - left_panel->width(), bounds.height());
}

ChildPanel::ChildPanel(BoxLayoutExample* example, gfx::Size preferred_size)
    : View(), example_(example), preferred_size_(preferred_size) {
  SetBorder(CreateSolidBorder(1, SK_ColorGRAY));
  for (unsigned i = 0; i < sizeof(margin_) / sizeof(margin_[0]); ++i)
    margin_[i] = CreateTextfield();
  flex_ = CreateTextfield();
  flex_->SetText(base::ASCIIToUTF16(""));
}

gfx::Size ChildPanel::CalculatePreferredSize() const {
  return preferred_size_;
}

bool ChildPanel::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton())
    SetSelected(true);
  return true;
}

void ChildPanel::Layout() {
  const int kSpacing = 2;
  if (selected_) {
    gfx::Rect client = GetContentsBounds();
    for (unsigned i = 0; i < sizeof(margin_) / sizeof(margin_[0]); ++i) {
      gfx::Point pos;
      Textfield* textfield = margin_[i];
      switch (i) {
        case 0:
          pos = gfx::Point((client.width() - textfield->width()) / 2, kSpacing);
          break;
        case 1:
          pos =
              gfx::Point(kSpacing, (client.height() - textfield->height()) / 2);
          break;
        case 2:
          pos = gfx::Point((client.width() - textfield->width()) / 2,
                           client.height() - textfield->height() - kSpacing);
          break;
        case 3:
          pos = gfx::Point(client.width() - textfield->width() - kSpacing,
                           (client.height() - textfield->height()) / 2);
          break;
        default:
          NOTREACHED();
      }
      textfield->SetPosition(pos);
    }
    flex_->SetPosition(gfx::Point((client.width() - flex_->width()) / 2,
                                  (client.height() - flex_->height()) / 2));
  }
}

void ChildPanel::SetSelected(bool value) {
  if (value != selected_) {
    selected_ = value;
    SetBorder(CreateSolidBorder(1, selected_ ? SK_ColorBLACK : SK_ColorGRAY));
    if (selected_ && parent()) {
      for (int i = 0; i < parent()->child_count(); ++i) {
        View* child = parent()->child_at(i);
        if (child != this && child->GetGroup() == GetGroup()) {
          ChildPanel* child_panel = static_cast<ChildPanel*>(child);
          child_panel->SetSelected(false);
        }
      }
    }
    for (Textfield* textfield : margin_)
      textfield->SetVisible(selected_);
    flex_->SetVisible(selected_);
    InvalidateLayout();
    example_->RefreshLayoutPanel();
  }
}

int ChildPanel::GetFlex() {
  int flex;
  if (base::StringToInt(flex_->text(), &flex))
    return flex;
  return -1;
}

void ChildPanel::ContentsChanged(Textfield* sender,
                                 const base::string16& new_contents) {
  int edges[4];
  for (unsigned i = 0; i < sizeof(margin_) / sizeof(margin_[0]); ++i) {
    base::StringToInt(margin_[i]->text(), &edges[i]);
  }
  gfx::Insets margins = gfx::Insets(edges[0], edges[1], edges[2], edges[3]);
  if (!margins.IsEmpty())
    this->SetProperty(kMarginsKey, new gfx::Insets(margins));
  else
    this->ClearProperty(kMarginsKey);
  if (sender == flex_)
    example_->UpdateLayoutManager();
  example_->RefreshLayoutPanel();
}

Textfield* ChildPanel::CreateTextfield() {
  Textfield* textfield = new Textfield();
  textfield->SetDefaultWidthInChars(3);
  textfield->SizeToPreferredSize();
  textfield->SetText(base::ASCIIToUTF16("0"));
  textfield->set_controller(this);
  textfield->SetVisible(false);
  AddChildView(textfield);
  return textfield;
}

const int kSpacing = 3;
const int kPadding = 8;
const int kMaxPanels = 5;
const int kChildPanelGroup = 100;
const int kChildPanelWidth = 180;
const int kChildPanelHeight = 90;
const char* orientation_values[2] = {"Horizontal", "Vertical"};
const char* main_axis_values[3] = {"Start", "Center", "End"};
const char* cross_axis_values[4] = {"Stretch", "Start", "Center", "End"};
}

BoxLayoutExample::BoxLayoutExample() : ExampleBase("Box Layout") {}

BoxLayoutExample::~BoxLayoutExample() {}

Combobox* BoxLayoutExample::CreateCombobox(const base::string16& label_text,
                                           const char** items,
                                           int count,
                                           int& vertical_pos) {
  Label* label = new Label(label_text);
  label->SetPosition(gfx::Point(kPadding, vertical_pos));
  label->SizeToPreferredSize();
  Combobox* combo_box =
      new Combobox(std::make_unique<ExampleComboboxModel>(items, count));
  combo_box->SetPosition(
      gfx::Point(label->x() + label->width() + kSpacing, vertical_pos));
  combo_box->SizeToPreferredSize();
  combo_box->set_listener(this);
  label->SetSize(gfx::Size(label->width(), combo_box->height()));
  control_panel_->AddChildView(label);
  control_panel_->AddChildView(combo_box);
  vertical_pos += combo_box->height() + kSpacing;
  return combo_box;
}

Textfield* BoxLayoutExample::CreateRawTextfield(int& horizontal_pos,
                                                int vertical_pos,
                                                bool add) {
  Textfield* text_field = new Textfield();
  text_field->SetPosition(gfx::Point(horizontal_pos, vertical_pos));
  text_field->SetDefaultWidthInChars(3);
  text_field->SetTextInputType(ui::TEXT_INPUT_TYPE_NUMBER);
  text_field->SizeToPreferredSize();
  text_field->SetText(base::ASCIIToUTF16("0"));
  text_field->set_controller(this);
  horizontal_pos += text_field->width() + kSpacing;
  if (add)
    control_panel_->AddChildView(text_field);
  return text_field;
}

Textfield* BoxLayoutExample::CreateTextfield(const base::string16& label_text,
                                             int& vertical_pos) {
  Label* label = new Label(label_text);
  label->SetPosition(gfx::Point(kPadding, vertical_pos));
  label->SizeToPreferredSize();
  int horizontal_pos = label->x() + label->width() + kSpacing;
  Textfield* text_field =
      CreateRawTextfield(horizontal_pos, vertical_pos, false);
  label->SetSize(gfx::Size(label->width(), text_field->height()));
  control_panel_->AddChildView(label);
  control_panel_->AddChildView(text_field);
  vertical_pos += text_field->height() + kSpacing;
  return text_field;
}

gfx::Size BoxLayoutExample::GetChildPanelSize() const {
  int width;
  int height;
  if (!base::StringToInt(child_panel_size_[0]->text(), &width))
    width = kChildPanelWidth;
  if (!base::StringToInt(child_panel_size_[1]->text(), &height))
    height = kChildPanelHeight;
  return gfx::Size(std::max(0, width), std::max(0, height));
}

void BoxLayoutExample::CreateExampleView(View* container) {
  container->SetLayoutManager(std::make_unique<FillLayout>());
  full_panel_ = new FullPanel();
  container->AddChildView(full_panel_);

  box_layout_panel_ = new View();
  box_layout_panel_->SetBorder(CreateSolidBorder(1, SK_ColorLTGRAY));
  full_panel_->AddChildView(box_layout_panel_);
  control_panel_ = new View();
  full_panel_->AddChildView(control_panel_);

  int vertical_pos = kSpacing;
  int horizontal_pos = kPadding;
  add_button_ =
      MdTextButton::CreateSecondaryUiButton(this, base::ASCIIToUTF16("Add"));
  add_button_->SetPosition(gfx::Point(horizontal_pos, vertical_pos));
  add_button_->SizeToPreferredSize();
  control_panel_->AddChildView(add_button_);
  horizontal_pos += add_button_->width() + kSpacing;
  for (unsigned i = 0;
       i < sizeof(child_panel_size_) / sizeof(child_panel_size_[0]); ++i) {
    child_panel_size_[i] =
        CreateRawTextfield(horizontal_pos, vertical_pos, true);
    child_panel_size_[i]->SetY(
        vertical_pos +
        (add_button_->height() - child_panel_size_[i]->height()) / 2);
  }
  child_panel_size_[0]->SetText(base::IntToString16(kChildPanelWidth));
  child_panel_size_[1]->SetText(base::IntToString16(kChildPanelHeight));
  vertical_pos += add_button_->height() + kSpacing;

  orientation_ = CreateCombobox(base::ASCIIToUTF16("Orientation"),
                                orientation_values, 2, vertical_pos);
  main_axis_alignment_ = CreateCombobox(base::ASCIIToUTF16("Main axis"),
                                        main_axis_values, 3, vertical_pos);
  cross_axis_alignment_ = CreateCombobox(base::ASCIIToUTF16("Cross axis"),
                                         cross_axis_values, 4, vertical_pos);

  between_child_spacing_ =
      CreateTextfield(base::ASCIIToUTF16("Child spacing"), vertical_pos);
  default_flex_ =
      CreateTextfield(base::ASCIIToUTF16("Default flex"), vertical_pos);
  min_cross_axis_size_ =
      CreateTextfield(base::ASCIIToUTF16("Min cross axis"), vertical_pos);

  border_insets_[0] =
      CreateTextfield(base::ASCIIToUTF16("Insets"), vertical_pos);
  horizontal_pos =
      border_insets_[0]->x() + border_insets_[0]->width() + kSpacing;
  for (unsigned i = 1; i < sizeof(border_insets_) / sizeof(border_insets_[0]);
       ++i)
    border_insets_[i] =
        CreateRawTextfield(horizontal_pos, border_insets_[0]->y(), true);

  collapse_margins_ = new Checkbox(base::ASCIIToUTF16("Collapse margins"));
  collapse_margins_->SetPosition(gfx::Point(kPadding, vertical_pos));
  collapse_margins_->SizeToPreferredSize();
  collapse_margins_->set_listener(this);
  control_panel_->AddChildView(collapse_margins_);

  UpdateLayoutManager();
}

void BoxLayoutExample::ButtonPressed(Button* sender, const ui::Event& event) {
  if (sender == add_button_) {
    if (panel_count_ < kMaxPanels) {
      ++panel_count_;
      ChildPanel* panel = new ChildPanel(this, GetChildPanelSize());
      panel->SetGroup(kChildPanelGroup);
      box_layout_panel_->AddChildView(panel);
      RefreshLayoutPanel();
    } else {
      PrintStatus("Only %i panels may be added", kMaxPanels);
    }
  } else if (sender == collapse_margins_) {
    UpdateLayoutManager();
    RefreshLayoutPanel();
  }
}

void BoxLayoutExample::OnPerformAction(Combobox* combobox) {
  if (combobox == orientation_) {
    UpdateLayoutManager();
  } else if (combobox == main_axis_alignment_) {
    layout_->set_main_axis_alignment(static_cast<BoxLayout::MainAxisAlignment>(
        main_axis_alignment_->selected_index()));
  } else if (combobox == cross_axis_alignment_) {
    layout_->set_cross_axis_alignment(
        static_cast<BoxLayout::CrossAxisAlignment>(
            cross_axis_alignment_->selected_index()));
  }
  RefreshLayoutPanel();
}

void BoxLayoutExample::ContentsChanged(Textfield* textfield,
                                       const base::string16& new_contents) {
  if (textfield == between_child_spacing_) {
    UpdateLayoutManager();
  } else if (textfield == default_flex_) {
    int default_flex;
    base::StringToInt(default_flex_->text(), &default_flex);
    layout_->SetDefaultFlex(default_flex);
  } else if (textfield == min_cross_axis_size_) {
    int min_cross_size;
    base::StringToInt(min_cross_axis_size_->text(), &min_cross_size);
    layout_->set_minimum_cross_axis_size(min_cross_size);
  } else if (textfield == border_insets_[0] || textfield == border_insets_[1] ||
             textfield == border_insets_[2] || textfield == border_insets_[3]) {
    UpdateBorderInsets();
  }
  RefreshLayoutPanel();
}

void BoxLayoutExample::RefreshLayoutPanel() {
  box_layout_panel_->Layout();
  box_layout_panel_->SchedulePaint();
}

void BoxLayoutExample::UpdateBorderInsets() {
  int inset_values[4];
  for (unsigned i = 0; i < sizeof(border_insets_) / sizeof(border_insets_[0]);
       ++i)
    base::StringToInt(border_insets_[i]->text(), &inset_values[i]);
  layout_->set_inside_border_insets(gfx::Insets(
      inset_values[0], inset_values[1], inset_values[2], inset_values[3]));
}

void BoxLayoutExample::UpdateLayoutManager() {
  int child_spacing;
  int default_flex;
  int min_cross_size;
  base::StringToInt(between_child_spacing_->text(), &child_spacing);
  base::StringToInt(default_flex_->text(), &default_flex);
  base::StringToInt(min_cross_axis_size_->text(), &min_cross_size);
  auto layout = std::make_unique<BoxLayout>(
      orientation_->selected_index() == 0 ? BoxLayout::Orientation::kHorizontal
                                          : BoxLayout::Orientation::kVertical,
      gfx::Insets(0, 0), child_spacing, collapse_margins_->checked());
  layout->set_cross_axis_alignment(static_cast<BoxLayout::CrossAxisAlignment>(
      cross_axis_alignment_->selected_index()));
  layout->set_main_axis_alignment(static_cast<BoxLayout::MainAxisAlignment>(
      main_axis_alignment_->selected_index()));
  layout->SetDefaultFlex(default_flex);
  layout->set_minimum_cross_axis_size(min_cross_size);
  layout_ = box_layout_panel_->SetLayoutManager(std::move(layout));
  UpdateBorderInsets();
  for (int i = 0; i < box_layout_panel_->child_count(); ++i) {
    ChildPanel* panel =
        static_cast<ChildPanel*>(box_layout_panel_->child_at(i));
    int flex = panel->GetFlex();
    if (flex < 0)
      layout_->ClearFlexForView(panel);
    else
      layout_->SetFlexForView(panel, flex);
  }
}

}  // namespace examples
}  // namespace views
