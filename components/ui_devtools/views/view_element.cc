// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ui_devtools/views/view_element.h"

#include "base/strings/utf_string_conversions.h"
#include "components/ui_devtools/Protocol.h"
#include "components/ui_devtools/ui_element_delegate.h"
#include "ui/views/widget/widget.h"

namespace ui_devtools {

ViewElement::ViewElement(views::View* view,
                         UIElementDelegate* ui_element_delegate,
                         UIElement* parent)
    : UIElement(UIElementType::VIEW, ui_element_delegate, parent), view_(view) {
  view_->AddObserver(this);
}

ViewElement::~ViewElement() {
  view_->RemoveObserver(this);
}

void ViewElement::OnChildViewRemoved(views::View* parent, views::View* view) {
  DCHECK_EQ(parent, view_);
  auto iter = std::find_if(
      children().begin(), children().end(), [view](UIElement* child) {
        return view ==
               UIElement::GetBackingElement<views::View, ViewElement>(child);
      });
  DCHECK(iter != children().end());
  UIElement* child_element = *iter;
  RemoveChild(child_element);
  delete child_element;
}

void ViewElement::OnChildViewAdded(views::View* parent, views::View* view) {
  DCHECK_EQ(parent, view_);
  AddChild(new ViewElement(view, delegate(), this),
           children().empty() ? nullptr : children().back());
}

void ViewElement::OnChildViewReordered(views::View* parent, views::View* view) {
  DCHECK_EQ(parent, view_);
  auto iter = std::find_if(
      children().begin(), children().end(), [view](UIElement* child) {
        return view ==
               UIElement::GetBackingElement<views::View, ViewElement>(child);
      });
  DCHECK(iter != children().end());
  UIElement* child_element = *iter;
  ReorderChild(child_element, parent->GetIndexOf(view));
}

void ViewElement::OnViewBoundsChanged(views::View* view) {
  DCHECK_EQ(view_, view);
  delegate()->OnUIElementBoundsChanged(this);
}

std::vector<std::pair<std::string, std::string>>
ViewElement::GetCustomProperties() const {
  base::string16 description;
  if (view_->GetTooltipText(gfx::Point(), &description)) {
    return {std::make_pair<std::string, std::string>(
        "tooltip", base::UTF16ToUTF8(description))};
  }
  return {};
}

void ViewElement::GetBounds(gfx::Rect* bounds) const {
  *bounds = view_->bounds();
}

void ViewElement::SetBounds(const gfx::Rect& bounds) {
  view_->SetBoundsRect(bounds);
}

void ViewElement::GetVisible(bool* visible) const {
  *visible = view_->visible();
}

void ViewElement::SetVisible(bool visible) {
  view_->SetVisible(visible);
}

std::unique_ptr<protocol::Array<std::string>> ViewElement::GetAttributes()
    const {
  auto attributes = protocol::Array<std::string>::create();
  // TODO(lgrey): Change name to class after updating tests.
  attributes->addItem("name");
  attributes->addItem(view_->GetClassName());
  return attributes;
}

std::pair<gfx::NativeWindow, gfx::Rect> ViewElement::GetNodeWindowAndBounds()
    const {
  return std::make_pair(view_->GetWidget()->GetNativeWindow(),
                        view_->GetBoundsInScreen());
}

// static
views::View* ViewElement::From(const UIElement* element) {
  DCHECK_EQ(UIElementType::VIEW, element->type());
  return static_cast<const ViewElement*>(element)->view_;
}

template <>
int UIElement::FindUIElementIdForBackendElement<views::View>(
    views::View* element) const {
  if (type_ == UIElementType::VIEW &&
      UIElement::GetBackingElement<views::View, ViewElement>(this) == element) {
    return node_id_;
  }
  for (auto* child : children_) {
    int ui_element_id = child->FindUIElementIdForBackendElement(element);
    if (ui_element_id)
      return ui_element_id;
  }
  return 0;
}

}  // namespace ui_devtools
