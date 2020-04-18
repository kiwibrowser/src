// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/accessibility/ax_aura_obj_cache.h"

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "base/strings/string_util.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/focus_client.h"
#include "ui/aura/window.h"
#include "ui/views/accessibility/ax_aura_obj_wrapper.h"
#include "ui/views/accessibility/ax_view_obj_wrapper.h"
#include "ui/views/accessibility/ax_widget_obj_wrapper.h"
#include "ui/views/accessibility/ax_window_obj_wrapper.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace views {

aura::client::FocusClient* GetFocusClient(aura::Window* root_window) {
  if (!root_window)
    return nullptr;
  return aura::client::GetFocusClient(root_window);
}

// static
AXAuraObjCache* AXAuraObjCache::GetInstance() {
  return base::Singleton<AXAuraObjCache>::get();
}

AXAuraObjWrapper* AXAuraObjCache::GetOrCreate(View* view) {
  return CreateInternal<AXViewObjWrapper>(view, view_to_id_map_);
}

AXAuraObjWrapper* AXAuraObjCache::GetOrCreate(Widget* widget) {
  return CreateInternal<AXWidgetObjWrapper>(widget, widget_to_id_map_);
}

AXAuraObjWrapper* AXAuraObjCache::GetOrCreate(aura::Window* window) {
  return CreateInternal<AXWindowObjWrapper>(window, window_to_id_map_);
}

int32_t AXAuraObjCache::GetID(View* view) const {
  return GetIDInternal(view, view_to_id_map_);
}

int32_t AXAuraObjCache::GetID(Widget* widget) const {
  return GetIDInternal(widget, widget_to_id_map_);
}

int32_t AXAuraObjCache::GetID(aura::Window* window) const {
  return GetIDInternal(window, window_to_id_map_);
}

void AXAuraObjCache::Remove(View* view) {
  RemoveInternal(view, view_to_id_map_);
}

void AXAuraObjCache::RemoveViewSubtree(View* view) {
  Remove(view);
  for (int i = 0; i < view->child_count(); ++i)
    RemoveViewSubtree(view->child_at(i));
}

void AXAuraObjCache::Remove(Widget* widget) {
  RemoveInternal(widget, widget_to_id_map_);

  // When an entire widget is deleted, it doesn't always send a notification
  // on each of its views, so we need to explore them recursively.
  if (widget->GetRootView())
    RemoveViewSubtree(widget->GetRootView());
}

void AXAuraObjCache::Remove(aura::Window* window, aura::Window* parent) {
  int id = GetIDInternal(parent, window_to_id_map_);
  AXAuraObjWrapper* parent_window_obj = Get(id);
  RemoveInternal(window, window_to_id_map_);
  if (parent && delegate_)
    delegate_->OnChildWindowRemoved(parent_window_obj);
}

AXAuraObjWrapper* AXAuraObjCache::Get(int32_t id) {
  auto it = cache_.find(id);

  if (it == cache_.end())
    return nullptr;

  return it->second.get();
}

void AXAuraObjCache::Remove(int32_t id) {
  AXAuraObjWrapper* obj = Get(id);

  if (id == -1 || !obj)
    return;

  cache_.erase(id);
}

void AXAuraObjCache::GetTopLevelWindows(
    std::vector<AXAuraObjWrapper*>* children) {
  for (auto it = window_to_id_map_.begin(); it != window_to_id_map_.end();
       ++it) {
    if (!it->first->parent())
      children->push_back(GetOrCreate(it->first));
  }
}

AXAuraObjWrapper* AXAuraObjCache::GetFocus() {
  View* focused_view = GetFocusedView();
  if (focused_view)
    return GetOrCreate(focused_view);
  return nullptr;
}

void AXAuraObjCache::OnFocusedViewChanged() {
  View* view = GetFocusedView();
  if (view)
    view->NotifyAccessibilityEvent(ax::mojom::Event::kFocus, true);
}

void AXAuraObjCache::FireEvent(AXAuraObjWrapper* aura_obj,
                               ax::mojom::Event event_type) {
  if (delegate_)
    delegate_->OnEvent(aura_obj, event_type);
}

AXAuraObjCache::AXAuraObjCache() = default;

AXAuraObjCache::~AXAuraObjCache() {
  is_destroying_ = true;
  cache_.clear();
}

View* AXAuraObjCache::GetFocusedView() {
  if (root_windows_.empty())
    return nullptr;
  aura::client::FocusClient* focus_client =
      GetFocusClient(*root_windows_.begin());
  if (!focus_client)
    return nullptr;

  aura::Window* focused_window = focus_client->GetFocusedWindow();
  if (!focused_window)
    return nullptr;

  Widget* focused_widget = Widget::GetWidgetForNativeView(focused_window);
  while (!focused_widget) {
    focused_window = focused_window->parent();
    if (!focused_window)
      break;

    focused_widget = Widget::GetWidgetForNativeView(focused_window);
  }

  if (!focused_widget)
    return nullptr;

  FocusManager* focus_manager = focused_widget->GetFocusManager();
  if (!focus_manager)
    return nullptr;

  View* focused_view = focus_manager->GetFocusedView();
  if (focused_view)
    return focused_view;

  if (focused_window->GetProperty(
          aura::client::kAccessibilityFocusFallsbackToWidgetKey)) {
    // If no view is focused, falls back to root view.
    return focused_widget->GetRootView();
  }

  return nullptr;
}

void AXAuraObjCache::OnWindowFocused(aura::Window* gained_focus,
                                     aura::Window* lost_focus) {
  OnFocusedViewChanged();
}

void AXAuraObjCache::OnRootWindowObjCreated(aura::Window* window) {
  if (root_windows_.empty() && GetFocusClient(window))
    GetFocusClient(window)->AddObserver(this);
  root_windows_.insert(window);
}

void AXAuraObjCache::OnRootWindowObjDestroyed(aura::Window* window) {
  root_windows_.erase(window);
  if (root_windows_.empty() && GetFocusClient(window))
    GetFocusClient(window)->RemoveObserver(this);
}

template <typename AuraViewWrapper, typename AuraView>
AXAuraObjWrapper* AXAuraObjCache::CreateInternal(
    AuraView* aura_view,
    std::map<AuraView*, int32_t>& aura_view_to_id_map) {
  if (!aura_view)
    return nullptr;

  auto it = aura_view_to_id_map.find(aura_view);

  if (it != aura_view_to_id_map.end())
    return Get(it->second);

  AXAuraObjWrapper* wrapper = new AuraViewWrapper(aura_view);
  int32_t id = wrapper->GetUniqueId().Get();
  aura_view_to_id_map[aura_view] = id;
  cache_[id] = base::WrapUnique(wrapper);
  return wrapper;
}

template <typename AuraView>
int32_t AXAuraObjCache::GetIDInternal(
    AuraView* aura_view,
    const std::map<AuraView*, int32_t>& aura_view_to_id_map) const {
  if (!aura_view)
    return -1;

  auto it = aura_view_to_id_map.find(aura_view);
  if (it != aura_view_to_id_map.end())
    return it->second;

  return -1;
}

template <typename AuraView>
void AXAuraObjCache::RemoveInternal(
    AuraView* aura_view,
    std::map<AuraView*, int32_t>& aura_view_to_id_map) {
  int32_t id = GetID(aura_view);
  if (id == -1)
    return;
  aura_view_to_id_map.erase(aura_view);
  Remove(id);
}

}  // namespace views
