// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/window_transient_descendant_iterator.h"
#include <ui/aura/window.h>
#include <ui/wm/core/window_util.h>

namespace ash {
namespace wm {

namespace {

aura::Window* GetTransientRoot(aura::Window* window) {
  while (window && ::wm::GetTransientParent(window))
    window = ::wm::GetTransientParent(window);
  return window;
}

}  // namespace

WindowTransientDescendantIterator::WindowTransientDescendantIterator()
    : current_window_(nullptr) {}

WindowTransientDescendantIterator::WindowTransientDescendantIterator(
    aura::Window* root_window)
    : current_window_(root_window) {
  DCHECK(!::wm::GetTransientParent(root_window));
}

// Performs a pre-order traversal of the transient descendants.
const WindowTransientDescendantIterator& WindowTransientDescendantIterator::
operator++() {
  DCHECK(current_window_);

  const aura::Window::Windows transient_children =
      ::wm::GetTransientChildren(current_window_);

  if (!transient_children.empty()) {
    current_window_ = transient_children.front();
  } else {
    while (current_window_) {
      aura::Window* parent = ::wm::GetTransientParent(current_window_);
      if (!parent) {
        current_window_ = nullptr;
        break;
      }
      const aura::Window::Windows transient_siblings =
          ::wm::GetTransientChildren(parent);
      auto iter = std::find(transient_siblings.begin(),
                            transient_siblings.end(), current_window_);
      ++iter;
      if (iter != transient_siblings.end()) {
        current_window_ = *iter;
        break;
      }
      current_window_ = ::wm::GetTransientParent(current_window_);
    }
  }
  return *this;
}

bool WindowTransientDescendantIterator::operator!=(
    const WindowTransientDescendantIterator& other) const {
  return current_window_ != other.current_window_;
}

aura::Window* WindowTransientDescendantIterator::operator*() const {
  return current_window_;
}

WindowTransientDescendantIteratorRange::WindowTransientDescendantIteratorRange(
    const WindowTransientDescendantIterator& begin)
    : begin_(begin) {}

WindowTransientDescendantIteratorRange GetTransientTreeIterator(
    aura::Window* window) {
  return WindowTransientDescendantIteratorRange(
      WindowTransientDescendantIterator(GetTransientRoot(window)));
}

}  // namespace wm
}  // namespace ash
