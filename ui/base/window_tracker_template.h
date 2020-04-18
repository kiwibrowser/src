// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_WINDOW_TRACKER_TEMPLATE_H_
#define UI_BASE_WINDOW_TRACKER_TEMPLATE_H_

#include <vector>

#include "base/macros.h"
#include "base/stl_util.h"

namespace ui {

// This class is used to track an ordered list of objects that support an
// observer interface with the function OnWindowDestroying(). When the object
// is destroyed it is removed from the ordered list of objects.
// Examples of T include aura::Window and its corresponding
// aura::WindowObserver interface.
template <class T, class TObserver>
class WindowTrackerTemplate : public TObserver {
 public:
  // A vector<> is used for tracking the windows (instead of a set<>) because
  // the user may want to know about the order of the windows that have been
  // added.
  using WindowList = std::vector<T*>;

  explicit WindowTrackerTemplate(const WindowList& windows) {
    for (T* window : windows)
      Add(window);
  }
  WindowTrackerTemplate() {}
  ~WindowTrackerTemplate() override { RemoveAll(); }

  // Returns the set of windows being observed.
  const WindowList& windows() const { return windows_; }

  // Adds |window| to the set of Windows being tracked.
  void Add(T* window) {
    if (base::ContainsValue(windows_, window))
      return;

    window->AddObserver(this);
    windows_.push_back(window);
  }

  void RemoveAll() {
    for (T* window : windows_)
      window->RemoveObserver(this);
    windows_.clear();
  }

  // Removes |window| from the set of windows being tracked.
  void Remove(T* window) {
    auto iter = std::find(windows_.begin(), windows_.end(), window);
    if (iter != windows_.end()) {
      window->RemoveObserver(this);
      windows_.erase(iter);
    }
  }

  T* Pop() {
    DCHECK(!windows_.empty());
    T* result = windows_[0];
    Remove(result);
    return result;
  }

  // Returns true if |window| was previously added and has not been removed or
  // deleted.
  bool Contains(T* window) const {
    return base::ContainsValue(windows_, window);
  }

  // Observer overrides:
  void OnWindowDestroying(T* window) override {
    DCHECK(Contains(window));
    Remove(window);
  }

 private:
  WindowList windows_;

  DISALLOW_COPY_AND_ASSIGN(WindowTrackerTemplate);
};

}  // namespace ui

#endif  // UI_BASE_WINDOW_TRACKER_TEMPLATE_H_
