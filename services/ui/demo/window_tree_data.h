// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_DEMO_WINDOW_TREE_DATA_H_
#define SERVICES_UI_DEMO_WINDOW_TREE_DATA_H_

#include "base/timer/timer.h"

namespace aura {
class Window;
class WindowTreeHostMus;
}  // namespace aura

namespace aura_extra {
class ImageWindowDelegate;
}  // namespace aura_extra

namespace ui {
namespace demo {

class WindowTreeData {
 public:
  explicit WindowTreeData(int square_size);
  ~WindowTreeData();

  // Initializes the window tree host and start drawing frames.
  void Init(std::unique_ptr<aura::WindowTreeHostMus> window_tree_host);
  bool IsInitialized() const { return !!window_delegate_; }

  const aura::WindowTreeHostMus* WindowTreeHost() const {
    return window_tree_host_.get();
  }

 protected:
  void SetWindowTreeHost(
      std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) {
    window_tree_host_ = std::move(window_tree_host);
  }

 private:
  // Draws one frame, incrementing the rotation angle.
  void DrawFrame();

  // Helper function to retrieve the window to which we draw the bitmap.
  aura::Window* bitmap_window();

  // The Window tree host corresponding to this data.
  std::unique_ptr<aura::WindowTreeHostMus> window_tree_host_;

  // Destroys itself when the window gets destroyed.
  aura_extra::ImageWindowDelegate* window_delegate_ = nullptr;

  // Timer for calling DrawFrame().
  base::RepeatingTimer timer_;

  // Current rotation angle for drawing.
  double angle_ = 0.0;

  // Size in pixels of the square to draw.
  const int square_size_;

  DISALLOW_COPY_AND_ASSIGN(WindowTreeData);
};

}  // namespace demo
}  // namespace ui

#endif  // SERVICES_UI_DEMO_WINDOW_TREE_DATA_H_
