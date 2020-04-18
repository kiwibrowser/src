// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_DEMO_MUS_DEMO_EXTERNAL_H_
#define SERVICES_UI_DEMO_MUS_DEMO_EXTERNAL_H_

#include <memory>
#include <vector>

#include "services/ui/demo/mus_demo.h"

namespace ui {
namespace demo {

// MusDemoExternal demonstrates Mus operating in "external" mode: A new platform
// window (and hence acceleratedWidget) is created for each aura window.
class MusDemoExternal : public MusDemo {
 public:
  MusDemoExternal();
  ~MusDemoExternal() final;

 private:
  void OpenNewWindow();

  // ui::demo::MusDemo:
  void OnStartImpl() final;
  std::unique_ptr<aura::WindowTreeClient> CreateWindowTreeClient() final;

  // aura::WindowTreeClientDelegate:
  void OnEmbed(std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) final;
  void OnEmbedRootDestroyed(aura::WindowTreeHostMus* window_tree_host) final;

  size_t initialized_windows_count_ = 0;

  size_t number_of_windows_ = 1;

  DISALLOW_COPY_AND_ASSIGN(MusDemoExternal);
};

}  // namespace demo
}  // namespace ui

#endif  // SERVICES_UI_DEMO_MUS_DEMO_EXTERNAL_H_
