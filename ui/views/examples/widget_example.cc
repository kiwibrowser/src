// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/examples/widget_example.h"

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_delegate.h"

using base::ASCIIToUTF16;

namespace views {
namespace examples {

namespace {

class WidgetDialogExample : public DialogDelegateView {
 public:
  WidgetDialogExample();
  ~WidgetDialogExample() override;
  base::string16 GetWindowTitle() const override;
  View* CreateExtraView() override;
  View* CreateFootnoteView() override;
};

class ModalDialogExample : public WidgetDialogExample {
 public:
  ModalDialogExample() {}

  // WidgetDelegate:
  ui::ModalType GetModalType() const override { return ui::MODAL_TYPE_WINDOW; }

 private:
  DISALLOW_COPY_AND_ASSIGN(ModalDialogExample);
};

WidgetDialogExample::WidgetDialogExample() {
  SetBackground(CreateSolidBackground(SK_ColorGRAY));
  SetLayoutManager(
      std::make_unique<BoxLayout>(BoxLayout::kVertical, gfx::Insets(10), 10));
  AddChildView(new Label(ASCIIToUTF16("Dialog contents label!")));
}

WidgetDialogExample::~WidgetDialogExample() {}

base::string16 WidgetDialogExample::GetWindowTitle() const {
  return ASCIIToUTF16("Dialog Widget Example");
}

View* WidgetDialogExample::CreateExtraView() {
  return MdTextButton::CreateSecondaryUiButton(nullptr,
                                               ASCIIToUTF16("Extra button!"));
}

View* WidgetDialogExample::CreateFootnoteView() {
  return new Label(ASCIIToUTF16("Footnote label!"));
}

}  // namespace

WidgetExample::WidgetExample() : ExampleBase("Widget") {
}

WidgetExample::~WidgetExample() {
}

void WidgetExample::CreateExampleView(View* container) {
  container->SetLayoutManager(
      std::make_unique<BoxLayout>(BoxLayout::kHorizontal, gfx::Insets(), 10));
  BuildButton(container, "Popup widget", POPUP);
  BuildButton(container, "Dialog widget", DIALOG);
  BuildButton(container, "Modal Dialog", MODAL_DIALOG);
#if defined(OS_LINUX)
  // Windows does not support TYPE_CONTROL top-level widgets.
  BuildButton(container, "Child widget", CHILD);
#endif
}

void WidgetExample::BuildButton(View* container,
                                const std::string& label,
                                int tag) {
  LabelButton* button = new LabelButton(this, ASCIIToUTF16(label));
  button->SetFocusForPlatform();
  button->set_request_focus_on_press(true);
  button->set_tag(tag);
  container->AddChildView(button);
}

void WidgetExample::ShowWidget(View* sender, Widget::InitParams params) {
  // Setup shared Widget heirarchy and bounds parameters.
  params.parent = sender->GetWidget()->GetNativeView();
  params.bounds = gfx::Rect(sender->GetBoundsInScreen().CenterPoint(),
                            gfx::Size(300, 200));

  Widget* widget = new Widget();
  widget->Init(params);

  // If the Widget has no contents by default, add a view with a 'Close' button.
  if (!widget->GetContentsView()) {
    View* contents = new View();
    contents->SetLayoutManager(
        std::make_unique<BoxLayout>(BoxLayout::kHorizontal));
    contents->SetBackground(CreateSolidBackground(SK_ColorGRAY));
    BuildButton(contents, "Close", CLOSE_WIDGET);
    widget->SetContentsView(contents);
  }

  widget->Show();
}

void WidgetExample::ButtonPressed(Button* sender, const ui::Event& event) {
  switch (sender->tag()) {
    case POPUP:
      ShowWidget(sender, Widget::InitParams(Widget::InitParams::TYPE_POPUP));
      break;
    case DIALOG: {
      DialogDelegate::CreateDialogWidget(new WidgetDialogExample(), NULL,
                                         sender->GetWidget()->GetNativeView())
          ->Show();
      break;
    }
    case MODAL_DIALOG: {
      DialogDelegate::CreateDialogWidget(new ModalDialogExample(), NULL,
          sender->GetWidget()->GetNativeView())->Show();
      break;
    }
    case CHILD:
      ShowWidget(sender, Widget::InitParams(Widget::InitParams::TYPE_CONTROL));
      break;
    case CLOSE_WIDGET:
      sender->GetWidget()->Close();
      break;
  }
}

}  // namespace examples
}  // namespace views
