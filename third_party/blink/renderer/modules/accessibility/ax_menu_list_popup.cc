/*
 * Copyright (C) 2010 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/accessibility/ax_menu_list_popup.h"

#include "third_party/blink/renderer/core/html/forms/html_select_element.h"
#include "third_party/blink/renderer/modules/accessibility/ax_menu_list_option.h"
#include "third_party/blink/renderer/modules/accessibility/ax_object_cache_impl.h"

namespace blink {

using namespace HTMLNames;

AXMenuListPopup::AXMenuListPopup(AXObjectCacheImpl& ax_object_cache)
    : AXMockObject(ax_object_cache), active_index_(-1) {}

bool AXMenuListPopup::IsVisible() const {
  return !IsOffScreen();
}

bool AXMenuListPopup::IsOffScreen() const {
  if (!parent_)
    return true;

  return parent_->IsCollapsed();
}

AXRestriction AXMenuListPopup::Restriction() const {
  return parent_ && parent_->Restriction() == kDisabled ? kDisabled : kNone;
}

bool AXMenuListPopup::ComputeAccessibilityIsIgnored(
    IgnoredReasons* ignored_reasons) const {
  return AccessibilityIsIgnoredByDefault(ignored_reasons);
}

AXMenuListOption* AXMenuListPopup::MenuListOptionAXObject(
    HTMLElement* element) const {
  DCHECK(element);
  if (!IsHTMLOptionElement(*element))
    return nullptr;

  AXObject* object = AXObjectCache().GetOrCreate(element);
  if (!object || !object->IsMenuListOption())
    return nullptr;

  return ToAXMenuListOption(object);
}

int AXMenuListPopup::GetSelectedIndex() const {
  if (!parent_)
    return -1;

  Node* parent_node = parent_->GetNode();
  if (!IsHTMLSelectElement(parent_node))
    return -1;

  HTMLSelectElement* html_select_element = ToHTMLSelectElement(parent_node);
  return html_select_element->selectedIndex();
}

bool AXMenuListPopup::OnNativeClickAction() {
  if (!parent_)
    return false;

  return parent_->OnNativeClickAction();
}

void AXMenuListPopup::AddChildren() {
  DCHECK(!IsDetached());
  if (!parent_)
    return;

  Node* parent_node = parent_->GetNode();
  if (!IsHTMLSelectElement(parent_node))
    return;

  HTMLSelectElement* html_select_element = ToHTMLSelectElement(parent_node);
  have_children_ = true;

  if (active_index_ == -1)
    active_index_ = GetSelectedIndex();

  for (auto* const option_element : html_select_element->GetOptionList()) {
    AXMenuListOption* option = MenuListOptionAXObject(option_element);
    if (option) {
      option->SetParent(this);
      children_.push_back(option);
    }
  }
}

void AXMenuListPopup::UpdateChildrenIfNecessary() {
  if (have_children_ && parent_ && parent_->NeedsToUpdateChildren())
    ClearChildren();

  if (!have_children_)
    AddChildren();
}

void AXMenuListPopup::DidUpdateActiveOption(int option_index,
                                            bool fire_notifications) {
  UpdateChildrenIfNecessary();

  int old_index = active_index_;
  active_index_ = option_index;

  if (!fire_notifications)
    return;

  AXObjectCacheImpl& cache = AXObjectCache();
  if (old_index != option_index && old_index >= 0 &&
      old_index < static_cast<int>(children_.size())) {
    AXObject* previous_child = children_[old_index].Get();
    cache.PostNotification(previous_child,
                           AXObjectCacheImpl::kAXMenuListItemUnselected);
  }

  if (option_index >= 0 && option_index < static_cast<int>(children_.size())) {
    AXObject* child = children_[option_index].Get();
    cache.PostNotification(this, AXObjectCacheImpl::kAXChildrenChanged);
    cache.PostNotification(this, AXObjectCacheImpl::kAXActiveDescendantChanged);
    cache.PostNotification(child, AXObjectCacheImpl::kAXMenuListItemSelected);
  }
}

void AXMenuListPopup::DidHide() {
  AXObjectCacheImpl& cache = AXObjectCache();
  cache.PostNotification(this, AXObjectCacheImpl::kAXHide);
  if (ActiveDescendant())
    cache.PostNotification(this, AXObjectCacheImpl::kAXChildrenChanged);
    cache.PostNotification(ActiveDescendant(),
                           AXObjectCacheImpl::kAXMenuListItemUnselected);
}

void AXMenuListPopup::DidShow() {
  if (!have_children_)
    AddChildren();

  AXObjectCacheImpl& cache = AXObjectCache();
  cache.PostNotification(this, AXObjectCacheImpl::kAXShow);
  int selected_index = GetSelectedIndex();
  if (selected_index >= 0 &&
      selected_index < static_cast<int>(children_.size())) {
    DidUpdateActiveOption(selected_index);
  } else {
    cache.PostNotification(parent_,
                           AXObjectCacheImpl::kAXFocusedUIElementChanged);
  }
}

AXObject* AXMenuListPopup::ActiveDescendant() {
  // Some Windows screen readers don't work properly if the active descendant
  // gets the focus before they focus the list menu popup.
  if (parent_ && !parent_->IsFocused())
    return nullptr;

  if (active_index_ < 0 || active_index_ >= static_cast<int>(Children().size()))
    return nullptr;

  return children_[active_index_].Get();
}

}  // namespace blink
