// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/models/combobox_model.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/button/radio_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/native/native_view_host.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/tabbed_pane/tabbed_pane.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/test/focus_manager_test.h"
#include "ui/views/widget/root_view.h"
#include "ui/views/widget/widget.h"

using base::ASCIIToUTF16;

namespace views {

namespace {

enum {
  TOP_CHECKBOX_ID = 1,  // 1
  LEFT_CONTAINER_ID,
  APPLE_LABEL_ID,
  APPLE_TEXTFIELD_ID,
  ORANGE_LABEL_ID,  // 5
  ORANGE_TEXTFIELD_ID,
  BANANA_LABEL_ID,
  BANANA_TEXTFIELD_ID,
  KIWI_LABEL_ID,
  KIWI_TEXTFIELD_ID,  // 10
  FRUIT_BUTTON_ID,
  FRUIT_CHECKBOX_ID,
  COMBOBOX_ID,

  RIGHT_CONTAINER_ID,
  ASPARAGUS_BUTTON_ID,  // 15
  BROCCOLI_BUTTON_ID,
  CAULIFLOWER_BUTTON_ID,

  INNER_CONTAINER_ID,
  SCROLL_VIEW_ID,
  ROSETTA_LINK_ID,  // 20
  STUPEUR_ET_TREMBLEMENT_LINK_ID,
  DINER_GAME_LINK_ID,
  RIDICULE_LINK_ID,
  CLOSET_LINK_ID,
  VISITING_LINK_ID,  // 25
  AMELIE_LINK_ID,
  JOYEUX_NOEL_LINK_ID,
  CAMPING_LINK_ID,
  BRICE_DE_NICE_LINK_ID,
  TAXI_LINK_ID,  // 30
  ASTERIX_LINK_ID,

  OK_BUTTON_ID,
  CANCEL_BUTTON_ID,
  HELP_BUTTON_ID,

  STYLE_CONTAINER_ID,  // 35
  BOLD_CHECKBOX_ID,
  ITALIC_CHECKBOX_ID,
  UNDERLINED_CHECKBOX_ID,
  STYLE_HELP_LINK_ID,
  STYLE_TEXT_EDIT_ID,  // 40

  SEARCH_CONTAINER_ID,
  SEARCH_TEXTFIELD_ID,
  SEARCH_BUTTON_ID,
  HELP_LINK_ID,

  THUMBNAIL_CONTAINER_ID,  // 45
  THUMBNAIL_STAR_ID,
  THUMBNAIL_SUPER_STAR_ID,
};

class DummyComboboxModel : public ui::ComboboxModel {
 public:
  // Overridden from ui::ComboboxModel:
  int GetItemCount() const override { return 10; }
  base::string16 GetItemAt(int index) override {
    return ASCIIToUTF16("Item ") + base::IntToString16(index);
  }
};

// A View that can act as a pane.
class PaneView : public View, public FocusTraversable {
 public:
  PaneView() : focus_search_(NULL) {}

  // If this method is called, this view will use GetPaneFocusTraversable to
  // have this provided FocusSearch used instead of the default one, allowing
  // you to trap focus within the pane.
  void EnablePaneFocus(FocusSearch* focus_search) {
    focus_search_ = focus_search;
  }

  // Overridden from View:
  FocusTraversable* GetPaneFocusTraversable() override {
    if (focus_search_)
      return this;
    else
      return NULL;
  }

  // Overridden from FocusTraversable:
  views::FocusSearch* GetFocusSearch() override { return focus_search_; }
  FocusTraversable* GetFocusTraversableParent() override { return NULL; }
  View* GetFocusTraversableParentView() override { return NULL; }

 private:
  FocusSearch* focus_search_;
};

// BorderView is a view containing a native window with its own view hierarchy.
// It is interesting to test focus traversal from a view hierarchy to an inner
// view hierarchy.
class BorderView : public NativeViewHost {
 public:
  explicit BorderView(View* child) : child_(child) {
    DCHECK(child);
    SetFocusBehavior(FocusBehavior::NEVER);
  }

  virtual internal::RootView* GetContentsRootView() {
    return static_cast<internal::RootView*>(widget_->GetRootView());
  }

  FocusTraversable* GetFocusTraversable() override {
    return static_cast<internal::RootView*>(widget_->GetRootView());
  }

  void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) override {
    NativeViewHost::ViewHierarchyChanged(details);

    if (details.child == this && details.is_add) {
      if (!widget_) {
        widget_ = std::make_unique<Widget>();
        Widget::InitParams params(Widget::InitParams::TYPE_CONTROL);
        params.parent = details.parent->GetWidget()->GetNativeView();
        params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
        widget_->Init(params);
        widget_->SetFocusTraversableParentView(this);
        widget_->SetContentsView(child_);
      }

      // We have been added to a view hierarchy, attach the native view.
      Attach(widget_->GetNativeView());
      // Also update the FocusTraversable parent so the focus traversal works.
      static_cast<internal::RootView*>(widget_->GetRootView())->
          SetFocusTraversableParent(GetWidget()->GetFocusTraversable());
    }
  }

 private:
  View* child_;
  std::unique_ptr<Widget> widget_;

  DISALLOW_COPY_AND_ASSIGN(BorderView);
};

}  // namespace

class FocusTraversalTest : public FocusManagerTest {
 public:
  ~FocusTraversalTest() override;

  void InitContentView() override;

 protected:
  FocusTraversalTest();

  View* FindViewByID(int id) {
    View* view = GetContentsView()->GetViewByID(id);
    if (view)
      return view;
    if (style_tab_)
      view = style_tab_->GetSelectedTabContentView()->GetViewByID(id);
    if (view)
      return view;
    view = search_border_view_->GetContentsRootView()->GetViewByID(id);
    if (view)
      return view;
    return NULL;
  }

 protected:
  // Helper function to advance focus multiple times in a loop. |traversal_ids|
  // is an array of view ids of length |N|. |reverse| denotes the direction in
  // which focus should be advanced.
  template <size_t N>
  void AdvanceEntireFocusLoop(const int (&traversal_ids)[N], bool reverse) {
    for (size_t i = 0; i < 3; ++i) {
      for (size_t j = 0; j < N; j++) {
        SCOPED_TRACE(testing::Message() << "reverse:" << reverse << " i:" << i
                                        << " j:" << j);
        GetFocusManager()->AdvanceFocus(reverse);
        View* focused_view = GetFocusManager()->GetFocusedView();
        EXPECT_NE(nullptr, focused_view);
        if (focused_view)
          EXPECT_EQ(traversal_ids[reverse ? N - j - 1 : j], focused_view->id());
      }
    }
  }

  TabbedPane* style_tab_;
  BorderView* search_border_view_;
  DummyComboboxModel combobox_model_;
  PaneView* left_container_;
  PaneView* right_container_;

  DISALLOW_COPY_AND_ASSIGN(FocusTraversalTest);
};

FocusTraversalTest::FocusTraversalTest()
    : style_tab_(NULL),
      search_border_view_(NULL) {
}

FocusTraversalTest::~FocusTraversalTest() {
}

void FocusTraversalTest::InitContentView() {
  // Create a complicated view hierarchy with lots of control types for
  // use by all of the focus traversal tests.
  //
  // Class name, ID, and asterisk next to focusable views:
  //
  // View
  //   Checkbox            * TOP_CHECKBOX_ID
  //   PaneView              LEFT_CONTAINER_ID
  //     Label               APPLE_LABEL_ID
  //     Textfield         * APPLE_TEXTFIELD_ID
  //     Label               ORANGE_LABEL_ID
  //     Textfield         * ORANGE_TEXTFIELD_ID
  //     Label               BANANA_LABEL_ID
  //     Textfield         * BANANA_TEXTFIELD_ID
  //     Label               KIWI_LABEL_ID
  //     Textfield         * KIWI_TEXTFIELD_ID
  //     NativeButton      * FRUIT_BUTTON_ID
  //     Checkbox          * FRUIT_CHECKBOX_ID
  //     Combobox          * COMBOBOX_ID
  //   PaneView              RIGHT_CONTAINER_ID
  //     RadioButton       * ASPARAGUS_BUTTON_ID
  //     RadioButton       * BROCCOLI_BUTTON_ID
  //     RadioButton       * CAULIFLOWER_BUTTON_ID
  //     View                INNER_CONTAINER_ID
  //       ScrollView        SCROLL_VIEW_ID
  //         View
  //           Link        * ROSETTA_LINK_ID
  //           Link        * STUPEUR_ET_TREMBLEMENT_LINK_ID
  //           Link        * DINER_GAME_LINK_ID
  //           Link        * RIDICULE_LINK_ID
  //           Link        * CLOSET_LINK_ID
  //           Link        * VISITING_LINK_ID
  //           Link        * AMELIE_LINK_ID
  //           Link        * JOYEUX_NOEL_LINK_ID
  //           Link        * CAMPING_LINK_ID
  //           Link        * BRICE_DE_NICE_LINK_ID
  //           Link        * TAXI_LINK_ID
  //           Link        * ASTERIX_LINK_ID
  //   NativeButton        * OK_BUTTON_ID
  //   NativeButton        * CANCEL_BUTTON_ID
  //   NativeButton        * HELP_BUTTON_ID
  //   TabbedPane          * STYLE_CONTAINER_ID
  //     TabStrip
  //       Tab ("Style")
  //       Tab ("Other")
  //     View
  //       View
  //         Checkbox      * BOLD_CHECKBOX_ID
  //         Checkbox      * ITALIC_CHECKBOX_ID
  //         Checkbox      * UNDERLINED_CHECKBOX_ID
  //         Link          * STYLE_HELP_LINK_ID
  //         Textfield     * STYLE_TEXT_EDIT_ID
  //       View
  //   BorderView            SEARCH_CONTAINER_ID
  //     View
  //       Textfield       * SEARCH_TEXTFIELD_ID
  //       NativeButton    * SEARCH_BUTTON_ID
  //       Link            * HELP_LINK_ID
  //   View                * THUMBNAIL_CONTAINER_ID
  //     NativeButton      * THUMBNAIL_STAR_ID
  //     NativeButton      * THUMBNAIL_SUPER_STAR_ID

  GetContentsView()->SetBackground(CreateSolidBackground(SK_ColorWHITE));

  Checkbox* cb = new Checkbox(ASCIIToUTF16("This is a checkbox"));
  GetContentsView()->AddChildView(cb);
  // In this fast paced world, who really has time for non hard-coded layout?
  cb->SetBounds(10, 10, 200, 20);
  cb->set_id(TOP_CHECKBOX_ID);

  left_container_ = new PaneView();
  left_container_->SetBorder(CreateSolidBorder(1, SK_ColorBLACK));
  left_container_->SetBackground(
      CreateSolidBackground(SkColorSetRGB(240, 240, 240)));
  left_container_->set_id(LEFT_CONTAINER_ID);
  GetContentsView()->AddChildView(left_container_);
  left_container_->SetBounds(10, 35, 250, 200);

  int label_x = 5;
  int label_width = 50;
  int label_height = 15;
  int text_field_width = 150;
  int y = 10;
  int gap_between_labels = 10;

  Label* label = new Label(ASCIIToUTF16("Apple:"));
  label->set_id(APPLE_LABEL_ID);
  left_container_->AddChildView(label);
  label->SetBounds(label_x, y, label_width, label_height);

  Textfield* text_field = new Textfield();
  text_field->set_id(APPLE_TEXTFIELD_ID);
  left_container_->AddChildView(text_field);
  text_field->SetBounds(label_x + label_width + 5, y,
                        text_field_width, label_height);

  y += label_height + gap_between_labels;

  label = new Label(ASCIIToUTF16("Orange:"));
  label->set_id(ORANGE_LABEL_ID);
  left_container_->AddChildView(label);
  label->SetBounds(label_x, y, label_width, label_height);

  text_field = new Textfield();
  text_field->set_id(ORANGE_TEXTFIELD_ID);
  left_container_->AddChildView(text_field);
  text_field->SetBounds(label_x + label_width + 5, y,
                        text_field_width, label_height);

  y += label_height + gap_between_labels;

  label = new Label(ASCIIToUTF16("Banana:"));
  label->set_id(BANANA_LABEL_ID);
  left_container_->AddChildView(label);
  label->SetBounds(label_x, y, label_width, label_height);

  text_field = new Textfield();
  text_field->set_id(BANANA_TEXTFIELD_ID);
  left_container_->AddChildView(text_field);
  text_field->SetBounds(label_x + label_width + 5, y,
                        text_field_width, label_height);

  y += label_height + gap_between_labels;

  label = new Label(ASCIIToUTF16("Kiwi:"));
  label->set_id(KIWI_LABEL_ID);
  left_container_->AddChildView(label);
  label->SetBounds(label_x, y, label_width, label_height);

  text_field = new Textfield();
  text_field->set_id(KIWI_TEXTFIELD_ID);
  left_container_->AddChildView(text_field);
  text_field->SetBounds(label_x + label_width + 5, y,
                        text_field_width, label_height);

  y += label_height + gap_between_labels;

  LabelButton* button = MdTextButton::Create(NULL, ASCIIToUTF16("Click me"));
  button->SetBounds(label_x, y + 10, 80, 30);
  button->set_id(FRUIT_BUTTON_ID);
  left_container_->AddChildView(button);
  y += 40;

  cb =  new Checkbox(ASCIIToUTF16("This is another check box"));
  cb->SetBounds(label_x + label_width + 5, y, 180, 20);
  cb->set_id(FRUIT_CHECKBOX_ID);
  left_container_->AddChildView(cb);
  y += 20;

  Combobox* combobox =  new Combobox(&combobox_model_);
  combobox->SetBounds(label_x + label_width + 5, y, 150, 30);
  combobox->set_id(COMBOBOX_ID);
  left_container_->AddChildView(combobox);

  right_container_ = new PaneView();
  right_container_->SetBorder(CreateSolidBorder(1, SK_ColorBLACK));
  right_container_->SetBackground(
      CreateSolidBackground(SkColorSetRGB(240, 240, 240)));
  right_container_->set_id(RIGHT_CONTAINER_ID);
  GetContentsView()->AddChildView(right_container_);
  right_container_->SetBounds(270, 35, 300, 200);

  y = 10;
  int radio_button_height = 18;
  int gap_between_radio_buttons = 10;
  RadioButton* radio_button = new RadioButton(ASCIIToUTF16("Asparagus"), 1);
  radio_button->set_id(ASPARAGUS_BUTTON_ID);
  right_container_->AddChildView(radio_button);
  radio_button->SetBounds(5, y, 70, radio_button_height);
  radio_button->SetGroup(1);
  y += radio_button_height + gap_between_radio_buttons;
  radio_button = new RadioButton(ASCIIToUTF16("Broccoli"), 1);
  radio_button->set_id(BROCCOLI_BUTTON_ID);
  right_container_->AddChildView(radio_button);
  radio_button->SetBounds(5, y, 70, radio_button_height);
  radio_button->SetGroup(1);
  RadioButton* radio_button_to_check = radio_button;
  y += radio_button_height + gap_between_radio_buttons;
  radio_button = new RadioButton(ASCIIToUTF16("Cauliflower"), 1);
  radio_button->set_id(CAULIFLOWER_BUTTON_ID);
  right_container_->AddChildView(radio_button);
  radio_button->SetBounds(5, y, 70, radio_button_height);
  radio_button->SetGroup(1);
  y += radio_button_height + gap_between_radio_buttons;

  View* inner_container = new View();
  inner_container->SetBorder(CreateSolidBorder(1, SK_ColorBLACK));
  inner_container->SetBackground(
      CreateSolidBackground(SkColorSetRGB(230, 230, 230)));
  inner_container->set_id(INNER_CONTAINER_ID);
  right_container_->AddChildView(inner_container);
  inner_container->SetBounds(100, 10, 150, 180);

  ScrollView* scroll_view = new ScrollView();
  scroll_view->set_id(SCROLL_VIEW_ID);
  inner_container->AddChildView(scroll_view);
  scroll_view->SetBounds(1, 1, 148, 178);

  View* scroll_content = new View();
  scroll_content->SetBounds(0, 0, 200, 200);
  scroll_content->SetBackground(
      CreateSolidBackground(SkColorSetRGB(200, 200, 200)));
  scroll_view->SetContents(scroll_content);

  static const char* const kTitles[] = {
      "Rosetta", "Stupeur et tremblement", "The diner game",
      "Ridicule", "Le placard", "Les Visiteurs", "Amelie",
      "Joyeux Noel", "Camping", "Brice de Nice",
      "Taxi", "Asterix"
  };

  static const int kIDs[] = {ROSETTA_LINK_ID,    STUPEUR_ET_TREMBLEMENT_LINK_ID,
                             DINER_GAME_LINK_ID, RIDICULE_LINK_ID,
                             CLOSET_LINK_ID,     VISITING_LINK_ID,
                             AMELIE_LINK_ID,     JOYEUX_NOEL_LINK_ID,
                             CAMPING_LINK_ID,    BRICE_DE_NICE_LINK_ID,
                             TAXI_LINK_ID,       ASTERIX_LINK_ID};

  DCHECK(arraysize(kTitles) == arraysize(kIDs));

  y = 5;
  for (size_t i = 0; i < arraysize(kTitles); ++i) {
    Link* link = new Link(ASCIIToUTF16(kTitles[i]));
    link->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    link->set_id(kIDs[i]);
    scroll_content->AddChildView(link);
    link->SetBounds(5, y, 300, 15);
    y += 15;
  }

  y = 250;
  int width = 60;
  button = MdTextButton::Create(NULL, ASCIIToUTF16("OK"));
  button->set_id(OK_BUTTON_ID);
  button->SetIsDefault(true);

  GetContentsView()->AddChildView(button);
  button->SetBounds(150, y, width, 30);

  button = MdTextButton::Create(NULL, ASCIIToUTF16("Cancel"));
  button->set_id(CANCEL_BUTTON_ID);
  GetContentsView()->AddChildView(button);
  button->SetBounds(220, y, width, 30);

  button = MdTextButton::Create(NULL, ASCIIToUTF16("Help"));
  button->set_id(HELP_BUTTON_ID);
  GetContentsView()->AddChildView(button);
  button->SetBounds(290, y, width, 30);

  y += 40;

  View* contents = NULL;
  Link* link = NULL;

  // Left bottom box with style checkboxes.
  contents = new View();
  contents->SetBackground(CreateSolidBackground(SK_ColorWHITE));
  cb = new Checkbox(ASCIIToUTF16("Bold"));
  contents->AddChildView(cb);
  cb->SetBounds(10, 10, 50, 20);
  cb->set_id(BOLD_CHECKBOX_ID);

  cb = new Checkbox(ASCIIToUTF16("Italic"));
  contents->AddChildView(cb);
  cb->SetBounds(70, 10, 50, 20);
  cb->set_id(ITALIC_CHECKBOX_ID);

  cb = new Checkbox(ASCIIToUTF16("Underlined"));
  contents->AddChildView(cb);
  cb->SetBounds(130, 10, 70, 20);
  cb->set_id(UNDERLINED_CHECKBOX_ID);

  link = new Link(ASCIIToUTF16("Help"));
  contents->AddChildView(link);
  link->SetBounds(10, 35, 70, 10);
  link->set_id(STYLE_HELP_LINK_ID);

  text_field = new Textfield();
  contents->AddChildView(text_field);
  text_field->SetBounds(10, 50, 100, 20);
  text_field->set_id(STYLE_TEXT_EDIT_ID);

  style_tab_ = new TabbedPane();
  GetContentsView()->AddChildView(style_tab_);
  style_tab_->SetBounds(10, y, 210, 100);
  style_tab_->AddTab(ASCIIToUTF16("Style"), contents);
  style_tab_->GetSelectedTab()->set_id(STYLE_CONTAINER_ID);
  style_tab_->AddTab(ASCIIToUTF16("Other"), new View());

  // Right bottom box with search.
  contents = new View();
  contents->SetBackground(CreateSolidBackground(SK_ColorWHITE));
  text_field = new Textfield();
  contents->AddChildView(text_field);
  text_field->SetBounds(10, 10, 100, 20);
  text_field->set_id(SEARCH_TEXTFIELD_ID);

  button = MdTextButton::Create(NULL, ASCIIToUTF16("Search"));
  contents->AddChildView(button);
  button->SetBounds(112, 5, 60, 30);
  button->set_id(SEARCH_BUTTON_ID);

  link = new Link(ASCIIToUTF16("Help"));
  link->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  link->set_id(HELP_LINK_ID);
  contents->AddChildView(link);
  link->SetBounds(175, 10, 30, 20);

  search_border_view_ = new BorderView(contents);
  search_border_view_->set_id(SEARCH_CONTAINER_ID);

  GetContentsView()->AddChildView(search_border_view_);
  search_border_view_->SetBounds(300, y, 240, 50);

  y += 60;

  contents = new View();
  contents->SetFocusBehavior(View::FocusBehavior::ALWAYS);
  contents->SetBackground(CreateSolidBackground(SK_ColorBLUE));
  contents->set_id(THUMBNAIL_CONTAINER_ID);
  button = MdTextButton::Create(NULL, ASCIIToUTF16("Star"));
  contents->AddChildView(button);
  button->SetBounds(5, 5, 50, 30);
  button->set_id(THUMBNAIL_STAR_ID);
  button = MdTextButton::Create(NULL, ASCIIToUTF16("SuperStar"));
  contents->AddChildView(button);
  button->SetBounds(60, 5, 100, 30);
  button->set_id(THUMBNAIL_SUPER_STAR_ID);

  GetContentsView()->AddChildView(contents);
  contents->SetBounds(250, y, 200, 50);
  // We can only call RadioButton::SetChecked() on the radio-button is part of
  // the view hierarchy.
  radio_button_to_check->SetChecked(true);
}

TEST_F(FocusTraversalTest, NormalTraversal) {
  const int kTraversalIDs[] = {TOP_CHECKBOX_ID,
                               APPLE_TEXTFIELD_ID,
                               ORANGE_TEXTFIELD_ID,
                               BANANA_TEXTFIELD_ID,
                               KIWI_TEXTFIELD_ID,
                               FRUIT_BUTTON_ID,
                               FRUIT_CHECKBOX_ID,
                               COMBOBOX_ID,
                               BROCCOLI_BUTTON_ID,
                               ROSETTA_LINK_ID,
                               STUPEUR_ET_TREMBLEMENT_LINK_ID,
                               DINER_GAME_LINK_ID,
                               RIDICULE_LINK_ID,
                               CLOSET_LINK_ID,
                               VISITING_LINK_ID,
                               AMELIE_LINK_ID,
                               JOYEUX_NOEL_LINK_ID,
                               CAMPING_LINK_ID,
                               BRICE_DE_NICE_LINK_ID,
                               TAXI_LINK_ID,
                               ASTERIX_LINK_ID,
                               OK_BUTTON_ID,
                               CANCEL_BUTTON_ID,
                               HELP_BUTTON_ID,
                               STYLE_CONTAINER_ID,
                               BOLD_CHECKBOX_ID,
                               ITALIC_CHECKBOX_ID,
                               UNDERLINED_CHECKBOX_ID,
                               STYLE_HELP_LINK_ID,
                               STYLE_TEXT_EDIT_ID,
                               SEARCH_TEXTFIELD_ID,
                               SEARCH_BUTTON_ID,
                               HELP_LINK_ID,
                               THUMBNAIL_CONTAINER_ID,
                               THUMBNAIL_STAR_ID,
                               THUMBNAIL_SUPER_STAR_ID};

  SCOPED_TRACE("NormalTraversal");

  // Let's traverse the whole focus hierarchy (several times, to make sure it
  // loops OK).
  GetFocusManager()->ClearFocus();
  AdvanceEntireFocusLoop(kTraversalIDs, false);

  // Let's traverse in reverse order.
  GetFocusManager()->ClearFocus();
  AdvanceEntireFocusLoop(kTraversalIDs, true);
}

#if defined(OS_MACOSX)
// Test focus traversal with full keyboard access off on Mac.
TEST_F(FocusTraversalTest, NormalTraversalMac) {
  GetFocusManager()->SetKeyboardAccessible(false);

  // Now only views with FocusBehavior of ALWAYS will be focusable.
  const int kTraversalIDs[] = {APPLE_TEXTFIELD_ID,    ORANGE_TEXTFIELD_ID,
                               BANANA_TEXTFIELD_ID,   KIWI_TEXTFIELD_ID,
                               STYLE_TEXT_EDIT_ID,    SEARCH_TEXTFIELD_ID,
                               THUMBNAIL_CONTAINER_ID};

  SCOPED_TRACE("NormalTraversalMac");

  // Let's traverse the whole focus hierarchy (several times, to make sure it
  // loops OK).
  GetFocusManager()->ClearFocus();
  AdvanceEntireFocusLoop(kTraversalIDs, false);

  // Let's traverse in reverse order.
  GetFocusManager()->ClearFocus();
  AdvanceEntireFocusLoop(kTraversalIDs, true);
}

// Test toggling full keyboard access correctly changes the focused view on Mac.
TEST_F(FocusTraversalTest, FullKeyboardToggle) {
  // Give focus to TOP_CHECKBOX_ID .
  FindViewByID(TOP_CHECKBOX_ID)->RequestFocus();
  EXPECT_EQ(TOP_CHECKBOX_ID, GetFocusManager()->GetFocusedView()->id());

  // Turn off full keyboard access. Focus should move to next view with ALWAYS
  // focus behavior.
  GetFocusManager()->SetKeyboardAccessible(false);
  EXPECT_EQ(APPLE_TEXTFIELD_ID, GetFocusManager()->GetFocusedView()->id());

  // Turning on full keyboard access should not change the focused view.
  GetFocusManager()->SetKeyboardAccessible(true);
  EXPECT_EQ(APPLE_TEXTFIELD_ID, GetFocusManager()->GetFocusedView()->id());

  // Give focus to SEARCH_BUTTON_ID.
  FindViewByID(SEARCH_BUTTON_ID)->RequestFocus();
  EXPECT_EQ(SEARCH_BUTTON_ID, GetFocusManager()->GetFocusedView()->id());

  // Turn off full keyboard access. Focus should move to next view with ALWAYS
  // focus behavior.
  GetFocusManager()->SetKeyboardAccessible(false);
  EXPECT_EQ(THUMBNAIL_CONTAINER_ID, GetFocusManager()->GetFocusedView()->id());

  // See focus advances correctly in both directions.
  GetFocusManager()->AdvanceFocus(false);
  EXPECT_EQ(APPLE_TEXTFIELD_ID, GetFocusManager()->GetFocusedView()->id());

  GetFocusManager()->AdvanceFocus(true);
  EXPECT_EQ(THUMBNAIL_CONTAINER_ID, GetFocusManager()->GetFocusedView()->id());
}
#endif  // OS_MACOSX

TEST_F(FocusTraversalTest, TraversalWithNonEnabledViews) {
  const int kDisabledIDs[] = {
      BANANA_TEXTFIELD_ID, FRUIT_CHECKBOX_ID,    COMBOBOX_ID,
      ASPARAGUS_BUTTON_ID, CAULIFLOWER_BUTTON_ID, CLOSET_LINK_ID,
      VISITING_LINK_ID,    BRICE_DE_NICE_LINK_ID, TAXI_LINK_ID,
      ASTERIX_LINK_ID,     HELP_BUTTON_ID,        BOLD_CHECKBOX_ID,
      SEARCH_TEXTFIELD_ID, HELP_LINK_ID};

  const int kTraversalIDs[] = {
      TOP_CHECKBOX_ID,    APPLE_TEXTFIELD_ID,
      ORANGE_TEXTFIELD_ID, KIWI_TEXTFIELD_ID,
      FRUIT_BUTTON_ID,     BROCCOLI_BUTTON_ID,
      ROSETTA_LINK_ID,     STUPEUR_ET_TREMBLEMENT_LINK_ID,
      DINER_GAME_LINK_ID,  RIDICULE_LINK_ID,
      AMELIE_LINK_ID,      JOYEUX_NOEL_LINK_ID,
      CAMPING_LINK_ID,     OK_BUTTON_ID,
      CANCEL_BUTTON_ID,    STYLE_CONTAINER_ID,
      ITALIC_CHECKBOX_ID, UNDERLINED_CHECKBOX_ID,
      STYLE_HELP_LINK_ID,  STYLE_TEXT_EDIT_ID,
      SEARCH_BUTTON_ID,    THUMBNAIL_CONTAINER_ID,
      THUMBNAIL_STAR_ID,   THUMBNAIL_SUPER_STAR_ID};

  SCOPED_TRACE("TraversalWithNonEnabledViews");

  // Let's disable some views.
  for (size_t i = 0; i < arraysize(kDisabledIDs); i++) {
    View* v = FindViewByID(kDisabledIDs[i]);
    ASSERT_TRUE(v != NULL);
    v->SetEnabled(false);
  }

  // Let's do one traversal (several times, to make sure it loops ok).
  GetFocusManager()->ClearFocus();
  AdvanceEntireFocusLoop(kTraversalIDs, false);

  // Same thing in reverse.
  GetFocusManager()->ClearFocus();
  AdvanceEntireFocusLoop(kTraversalIDs, true);
}

TEST_F(FocusTraversalTest, TraversalWithInvisibleViews) {
  const int kInvisibleIDs[] = {TOP_CHECKBOX_ID, OK_BUTTON_ID,
                               THUMBNAIL_CONTAINER_ID};

  const int kTraversalIDs[] = {
      APPLE_TEXTFIELD_ID,  ORANGE_TEXTFIELD_ID,
      BANANA_TEXTFIELD_ID, KIWI_TEXTFIELD_ID,
      FRUIT_BUTTON_ID,     FRUIT_CHECKBOX_ID,
      COMBOBOX_ID,         BROCCOLI_BUTTON_ID,
      ROSETTA_LINK_ID,     STUPEUR_ET_TREMBLEMENT_LINK_ID,
      DINER_GAME_LINK_ID,  RIDICULE_LINK_ID,
      CLOSET_LINK_ID,      VISITING_LINK_ID,
      AMELIE_LINK_ID,      JOYEUX_NOEL_LINK_ID,
      CAMPING_LINK_ID,     BRICE_DE_NICE_LINK_ID,
      TAXI_LINK_ID,        ASTERIX_LINK_ID,
      CANCEL_BUTTON_ID,    HELP_BUTTON_ID,
      STYLE_CONTAINER_ID,  BOLD_CHECKBOX_ID,
      ITALIC_CHECKBOX_ID, UNDERLINED_CHECKBOX_ID,
      STYLE_HELP_LINK_ID,  STYLE_TEXT_EDIT_ID,
      SEARCH_TEXTFIELD_ID, SEARCH_BUTTON_ID,
      HELP_LINK_ID};

  SCOPED_TRACE("TraversalWithInvisibleViews");

  // Let's make some views invisible.
  for (size_t i = 0; i < arraysize(kInvisibleIDs); i++) {
    View* v = FindViewByID(kInvisibleIDs[i]);
    ASSERT_TRUE(v != NULL);
    v->SetVisible(false);
  }

  // Let's do one traversal (several times, to make sure it loops ok).
  GetFocusManager()->ClearFocus();
  AdvanceEntireFocusLoop(kTraversalIDs, false);

  // Same thing in reverse.
  GetFocusManager()->ClearFocus();
  AdvanceEntireFocusLoop(kTraversalIDs, true);
}

TEST_F(FocusTraversalTest, PaneTraversal) {
  // Tests trapping the traversal within a pane - useful for full
  // keyboard accessibility for toolbars.

  // First test the left container.
  const int kLeftTraversalIDs[] = {APPLE_TEXTFIELD_ID,  ORANGE_TEXTFIELD_ID,
                                   BANANA_TEXTFIELD_ID, KIWI_TEXTFIELD_ID,
                                   FRUIT_BUTTON_ID,     FRUIT_CHECKBOX_ID,
                                   COMBOBOX_ID};

  SCOPED_TRACE("PaneTraversal");

  FocusSearch focus_search_left(left_container_, true, false);
  left_container_->EnablePaneFocus(&focus_search_left);
  FindViewByID(COMBOBOX_ID)->RequestFocus();

  // Traverse the focus hierarchy within the pane several times.
  AdvanceEntireFocusLoop(kLeftTraversalIDs, false);

  // Traverse in reverse order.
  FindViewByID(APPLE_TEXTFIELD_ID)->RequestFocus();
  AdvanceEntireFocusLoop(kLeftTraversalIDs, true);

  // Now test the right container, but this time with accessibility mode.
  // Make some links not focusable, but mark one of them as
  // "accessibility focusable", so it should show up in the traversal.
  const int kRightTraversalIDs[] = {
      BROCCOLI_BUTTON_ID,  DINER_GAME_LINK_ID, RIDICULE_LINK_ID,
      CLOSET_LINK_ID,      VISITING_LINK_ID,   AMELIE_LINK_ID,
      JOYEUX_NOEL_LINK_ID, CAMPING_LINK_ID,    BRICE_DE_NICE_LINK_ID,
      TAXI_LINK_ID,        ASTERIX_LINK_ID};

  FocusSearch focus_search_right(right_container_, true, true);
  right_container_->EnablePaneFocus(&focus_search_right);
  FindViewByID(ROSETTA_LINK_ID)->SetFocusBehavior(View::FocusBehavior::NEVER);
  FindViewByID(STUPEUR_ET_TREMBLEMENT_LINK_ID)
      ->SetFocusBehavior(View::FocusBehavior::NEVER);
  FindViewByID(DINER_GAME_LINK_ID)
      ->SetFocusBehavior(View::FocusBehavior::ACCESSIBLE_ONLY);
  FindViewByID(ASTERIX_LINK_ID)->RequestFocus();

  // Traverse the focus hierarchy within the pane several times.
  AdvanceEntireFocusLoop(kRightTraversalIDs, false);

  // Traverse in reverse order.
  FindViewByID(BROCCOLI_BUTTON_ID)->RequestFocus();
  AdvanceEntireFocusLoop(kRightTraversalIDs, true);
}

class FocusTraversalNonFocusableTest : public FocusManagerTest {
 public:
  ~FocusTraversalNonFocusableTest() override {}

  void InitContentView() override;

 protected:
  FocusTraversalNonFocusableTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusTraversalNonFocusableTest);
};

void FocusTraversalNonFocusableTest::InitContentView() {
  // Create a complex nested view hierarchy with no focusable views. This is a
  // regression test for http://crbug.com/453699. There was previously a bug
  // where advancing focus backwards through this tree resulted in an
  // exponential number of nodes being searched. (Each time it traverses one of
  // the x1-x3-x2 triangles, it will traverse the left sibling of x1, (x+1)0,
  // twice, which means it will visit O(2^n) nodes.)
  //
  // |              0         |
  // |            /   \       |
  // |          /       \     |
  // |         10        1    |
  // |        /  \      / \   |
  // |      /      \   /   \  |
  // |     20      11  2   3  |
  // |    / \      / \        |
  // |   /   \    /   \       |
  // |  ...  21  12   13      |
  // |       / \              |
  // |      /   \             |
  // |     22   23            |

  View* v = GetContentsView();
  // Create 30 groups of 4 nodes. |v| is the top of each group.
  for (int i = 0; i < 300; i += 10) {
    // |v|'s left child is the top of the next group. If |v| is 20, this is 30.
    View* v10 = new View;
    v10->set_id(i + 10);
    v->AddChildView(v10);

    // |v|'s right child. If |v| is 20, this is 21.
    View* v1 = new View;
    v1->set_id(i + 1);
    v->AddChildView(v1);

    // |v|'s right child has two children. If |v| is 20, these are 22 and 23.
    View* v2 = new View;
    v2->set_id(i + 2);
    View* v3 = new View;
    v3->set_id(i + 3);
    v1->AddChildView(v2);
    v1->AddChildView(v3);

    v = v10;
  }
}

// See explanation in InitContentView.
// NOTE: The failure mode of this test (if http://crbug.com/453699 were to
// regress) is a timeout, due to exponential run time.
TEST_F(FocusTraversalNonFocusableTest, PathologicalSiblingTraversal) {
  // Advance forwards from the root node.
  GetFocusManager()->ClearFocus();
  GetFocusManager()->AdvanceFocus(false);
  EXPECT_FALSE(GetFocusManager()->GetFocusedView());

  // Advance backwards from the root node.
  GetFocusManager()->ClearFocus();
  GetFocusManager()->AdvanceFocus(true);
  EXPECT_FALSE(GetFocusManager()->GetFocusedView());
}

}  // namespace views
