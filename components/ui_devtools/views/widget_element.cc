// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ui_devtools/views/widget_element.h"

#include "components/ui_devtools/Protocol.h"
#include "components/ui_devtools/ui_element_delegate.h"

namespace ui_devtools {

WidgetElement::WidgetElement(views::Widget* widget,
                             UIElementDelegate* ui_element_delegate,
                             UIElement* parent)
    : UIElement(UIElementType::WIDGET, ui_element_delegate, parent),
      widget_(widget) {
  widget_->AddRemovalsObserver(this);
}

WidgetElement::~WidgetElement() {
  widget_->RemoveRemovalsObserver(this);
}

void WidgetElement::OnWillRemoveView(views::Widget* widget, views::View* view) {
  if (view != widget->GetRootView())
    return;
  DCHECK_EQ(1u, children().size());
  UIElement* child = children()[0];
  RemoveChild(child);
  delete child;
}

void WidgetElement::OnWidgetBoundsChanged(views::Widget* widget,
                                          const gfx::Rect& new_bounds) {
  DCHECK_EQ(widget, widget_);
  delegate()->OnUIElementBoundsChanged(this);
}

std::vector<std::pair<std::string, std::string>>
WidgetElement::GetCustomProperties() const {
  return {};
}

void WidgetElement::GetBounds(gfx::Rect* bounds) const {
  *bounds = widget_->GetRestoredBounds();
}

void WidgetElement::SetBounds(const gfx::Rect& bounds) {
  widget_->SetBounds(bounds);
}

void WidgetElement::GetVisible(bool* visible) const {
  *visible = widget_->IsVisible();
}

void WidgetElement::SetVisible(bool visible) {
  if (visible == widget_->IsVisible())
    return;
  if (visible)
    widget_->Show();
  else
    widget_->Hide();
}

std::unique_ptr<protocol::Array<std::string>> WidgetElement::GetAttributes()
    const {
  auto attributes = protocol::Array<std::string>::create();
  attributes->addItem("name");
  attributes->addItem(widget_->GetName());
  attributes->addItem("active");
  attributes->addItem(widget_->IsActive() ? "true" : "false");
  return attributes;
}

std::pair<gfx::NativeWindow, gfx::Rect> WidgetElement::GetNodeWindowAndBounds()
    const {
  return std::make_pair(widget_->GetNativeWindow(),
                        widget_->GetWindowBoundsInScreen());
}

// static
views::Widget* WidgetElement::From(const UIElement* element) {
  DCHECK_EQ(UIElementType::WIDGET, element->type());
  return static_cast<const WidgetElement*>(element)->widget_;
}

}  // namespace ui_devtools
