// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/test_child_modal_parent.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "ui/aura/window.h"
#include "ui/gfx/canvas.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/native/native_view_host.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/wm/core/window_modality_controller.h"

using views::Widget;

namespace ash {

namespace {

// Parent window size and position.
const int kWindowLeft = 170;
const int kWindowTop = 200;
const int kWindowWidth = 400;
const int kWindowHeight = 400;

// Parent window layout.
const int kButtonHeight = 35;
const int kTextfieldHeight = 35;

// Child window size.
const int kChildWindowWidth = 330;
const int kChildWindowHeight = 200;

// Child window layout.
const int kChildTextfieldLeft = 20;
const int kChildTextfieldTop = 50;
const int kChildTextfieldWidth = 290;
const int kChildTextfieldHeight = 35;

const SkColor kModalParentColor = SK_ColorWHITE;
const SkColor kChildColor = SK_ColorWHITE;

}  // namespace

class ChildModalWindow : public views::WidgetDelegateView {
 public:
  ChildModalWindow();
  ~ChildModalWindow() override;

 private:
  // Overridden from View:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize() const override;

  // Overridden from WidgetDelegate:
  base::string16 GetWindowTitle() const override;
  bool CanResize() const override;
  ui::ModalType GetModalType() const override;

  DISALLOW_COPY_AND_ASSIGN(ChildModalWindow);
};

ChildModalWindow::ChildModalWindow() {
  views::Textfield* textfield = new views::Textfield;
  AddChildView(textfield);
  textfield->SetBounds(kChildTextfieldLeft, kChildTextfieldTop,
                       kChildTextfieldWidth, kChildTextfieldHeight);
}

ChildModalWindow::~ChildModalWindow() = default;

void ChildModalWindow::OnPaint(gfx::Canvas* canvas) {
  canvas->FillRect(GetLocalBounds(), kChildColor);
}

gfx::Size ChildModalWindow::CalculatePreferredSize() const {
  return gfx::Size(kChildWindowWidth, kChildWindowHeight);
}

base::string16 ChildModalWindow::GetWindowTitle() const {
  return base::ASCIIToUTF16("Examples: Child Modal Window");
}

bool ChildModalWindow::CanResize() const {
  return false;
}

ui::ModalType ChildModalWindow::GetModalType() const {
  return ui::MODAL_TYPE_CHILD;
}

// static
void TestChildModalParent::Create() {
  Widget::CreateWindowWithContextAndBounds(
      new TestChildModalParent(), /*context=*/nullptr,
      gfx::Rect(kWindowLeft, kWindowTop, kWindowWidth, kWindowHeight))
      ->Show();
}

TestChildModalParent::TestChildModalParent()
    : widget_(std::make_unique<Widget>()),
      button_(new views::LabelButton(
          this,
          base::ASCIIToUTF16("Show/Hide Child Modal Window"))),
      textfield_(new views::Textfield),
      host_(new views::NativeViewHost),
      child_(nullptr) {
  Widget::InitParams params(Widget::InitParams::TYPE_CONTROL);
  params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  widget_->Init(params);
  widget_->GetRootView()->SetBackground(
      views::CreateSolidBackground(kModalParentColor));
  widget_->GetNativeView()->SetName("ModalParent");
  AddChildView(button_);
  AddChildView(textfield_);
  AddChildView(host_);
}

TestChildModalParent::~TestChildModalParent() = default;

void TestChildModalParent::ShowChild() {
  if (!child_)
    child_ = CreateChild();
  child_->Show();
}

aura::Window* TestChildModalParent::GetModalParent() const {
  return widget_->GetNativeView();
}

aura::Window* TestChildModalParent::GetChild() const {
  if (child_)
    return child_->GetNativeView();
  return nullptr;
}

Widget* TestChildModalParent::CreateChild() {
  Widget* child = Widget::CreateWindowWithParent(new ChildModalWindow,
                                                 GetWidget()->GetNativeView());
  wm::SetModalParent(child->GetNativeView(), GetModalParent());
  child->AddObserver(this);
  child->GetNativeView()->SetName("ChildModalWindow");
  return child;
}

base::string16 TestChildModalParent::GetWindowTitle() const {
  return base::ASCIIToUTF16("Examples: Child Modal Parent");
}

bool TestChildModalParent::CanResize() const {
  return false;
}

void TestChildModalParent::DeleteDelegate() {
  if (child_) {
    child_->RemoveObserver(this);
    child_->Close();
    child_ = NULL;
  }

  delete this;
}

void TestChildModalParent::Layout() {
  int running_y = y();
  button_->SetBounds(x(), running_y, width(), kButtonHeight);
  running_y += kButtonHeight;
  textfield_->SetBounds(x(), running_y, width(), kTextfieldHeight);
  running_y += kTextfieldHeight;
  host_->SetBounds(x(), running_y, width(), height() - running_y);
}

void TestChildModalParent::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  if (details.is_add && details.child == this) {
    host_->Attach(widget_->GetNativeWindow());
    GetWidget()->GetNativeView()->SetName("Parent");
  }
}

void TestChildModalParent::ButtonPressed(views::Button* sender,
                                         const ui::Event& event) {
  if (sender == button_) {
    if (!child_)
      child_ = CreateChild();
    if (child_->IsVisible())
      child_->Hide();
    else
      child_->Show();
  }
}

void TestChildModalParent::OnWidgetDestroying(Widget* widget) {
  if (child_) {
    DCHECK_EQ(child_, widget);
    child_ = NULL;
  }
}

}  // namespace ash
