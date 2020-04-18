// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/server_window_drawn_tracker.h"

#include "services/ui/ws/server_window.h"
#include "services/ui/ws/server_window_drawn_tracker_observer.h"

namespace ui {

namespace ws {

ServerWindowDrawnTracker::ServerWindowDrawnTracker(
    ServerWindow* window,
    ServerWindowDrawnTrackerObserver* observer)
    : window_(window),
      observer_(observer),
      drawn_(window->IsDrawn()),
      weak_factory_(this) {
  AddObservers();
}

ServerWindowDrawnTracker::~ServerWindowDrawnTracker() {
  RemoveObservers();
}

void ServerWindowDrawnTracker::SetDrawn(ServerWindow* ancestor, bool drawn) {
  // If |windows_| is empty when this code runs, that means |window_| has been
  // destroyed. So set |window_| to nullptr, but make sure the right value is
  // sent to OnDrawnStateChanged().
  ServerWindow* window = window_;
  if (windows_.empty())
    window_ = nullptr;

  if (drawn == drawn_)
    return;

  drawn_ = drawn;
  observer_->OnDrawnStateChanged(ancestor, window, drawn);
}

void ServerWindowDrawnTracker::AddObservers() {
  if (!window_) {
    root_ = nullptr;
    return;
  }

  ServerWindow* last = window_;
  for (ServerWindow* v = window_; v; v = v->parent()) {
    v->AddObserver(this);
    windows_.insert(v);
    last = v;
  }
  DCHECK(last);
  root_ = last;
}

void ServerWindowDrawnTracker::RemoveObservers() {
  for (ServerWindow* window : windows_)
    window->RemoveObserver(this);

  windows_.clear();
}

void ServerWindowDrawnTracker::OnWindowDestroying(ServerWindow* window) {
  if (!drawn_)
    return;
  observer_->OnDrawnStateWillChange(window->parent(), window_, false);
}

void ServerWindowDrawnTracker::OnWindowDestroyed(ServerWindow* window) {
  // As windows are removed before being destroyed, resulting in
  // OnWindowHierarchyChanged() and us removing ourself as an observer, the only
  // window we should ever get notified of destruction on is |window_|.
  DCHECK_EQ(window, window_);
  RemoveObservers();
  SetDrawn(nullptr, false);
}

void ServerWindowDrawnTracker::OnWillChangeWindowHierarchy(
    ServerWindow* window,
    ServerWindow* new_parent,
    ServerWindow* old_parent) {
  bool new_is_drawn = new_parent && new_parent->IsDrawn();
  if (new_is_drawn) {
    for (ServerWindow* w = window_; new_is_drawn && w != old_parent;
         w = w->parent()) {
      new_is_drawn = w->visible();
    }
  }
  if (drawn_ != new_is_drawn) {
    auto ref = weak_factory_.GetWeakPtr();
    observer_->OnDrawnStateWillChange(new_is_drawn ? nullptr : old_parent,
                                      window_, new_is_drawn);
    // Allow for the |observer_| to delete |this|.
    if (!ref)
      return;
  }

  if (!root_->Contains(new_parent))
    observer_->OnRootWillChange(old_parent, window_);
}

void ServerWindowDrawnTracker::OnWindowHierarchyChanged(
    ServerWindow* window,
    ServerWindow* new_parent,
    ServerWindow* old_parent) {
  ServerWindow* old_root = root_;
  RemoveObservers();
  AddObservers();
  const bool is_drawn = window_->IsDrawn();
  auto ref = weak_factory_.GetWeakPtr();
  SetDrawn(is_drawn ? nullptr : old_parent, is_drawn);
  // Allow for the |observer_| to delete |this|.
  if (!ref)
    return;
  if (old_root != root_)
    observer_->OnRootDidChange(old_parent, window);
}

void ServerWindowDrawnTracker::OnWillChangeWindowVisibility(
    ServerWindow* window) {
  bool will_change = false;
  if (drawn_) {
    // If |window_| is currently drawn, then any change of visibility of the
    // windows will toggle the drawn status.
    will_change = true;
  } else {
    // If |window| is currently visible, then it's becoming invisible, and so
    // |window_| will remain not drawn.
    if (window->visible()) {
      will_change = false;
    } else {
      bool is_drawn = (window->GetRootForDrawn() == window) ||
                      (window->parent() && window->parent()->IsDrawn());
      if (is_drawn) {
        for (ServerWindow* w = window_; is_drawn && w != window;
             w = w->parent())
          is_drawn = w->visible();
      }
      will_change = drawn_ != is_drawn;
    }
  }
  if (will_change) {
    bool new_is_drawn = !drawn_;
    observer_->OnDrawnStateWillChange(new_is_drawn ? nullptr : window->parent(),
                                      window_, new_is_drawn);
  }
}

void ServerWindowDrawnTracker::OnWindowVisibilityChanged(ServerWindow* window) {
  const bool is_drawn = window_->IsDrawn();
  SetDrawn(is_drawn ? nullptr : window->parent(), is_drawn);
}

}  // namespace ws

}  // namespace ui
