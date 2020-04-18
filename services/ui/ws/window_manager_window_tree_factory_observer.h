// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_WINDOW_MANAGER_WINDOW_TREE_FACTORY_OBSERVER_H_
#define SERVICES_UI_WS_WINDOW_MANAGER_WINDOW_TREE_FACTORY_OBSERVER_H_

namespace ui {
namespace ws {

class WindowManagerWindowTreeFactory;

class WindowManagerWindowTreeFactoryObserver {
 public:
  // Called when the WindowTree associated with |factory| has been set
  virtual void OnWindowManagerWindowTreeFactoryReady(
      WindowManagerWindowTreeFactory* factory) = 0;

 protected:
  virtual ~WindowManagerWindowTreeFactoryObserver() {}
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_WINDOW_MANAGER_WINDOW_TREE_FACTORY_OBSERVER_H_
