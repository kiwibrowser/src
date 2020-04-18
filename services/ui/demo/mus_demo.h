// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_DEMO_MUS_DEMO_H_
#define SERVICES_UI_DEMO_MUS_DEMO_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "services/service_manager/public/cpp/service.h"
#include "ui/aura/mus/window_tree_client_delegate.h"
#include "ui/display/screen_base.h"

namespace aura {
class Env;
class PropertyConverter;

namespace client {
class DefaultCaptureClient;
}
}  // namespace aura

namespace wm {
class WMState;
}

namespace ui {
namespace demo {

class WindowTreeData;

// A simple MUS Demo service. This service connects to the service:ui, adds a
// new window to the root Window, and draws a spinning square in the center of
// the window. Provides a simple way to demonstrate that the graphic stack works
// as intended.
class MusDemo : public service_manager::Service,
                public aura::WindowTreeClientDelegate {
 public:
  MusDemo();
  ~MusDemo() override;

 protected:
  void AddPrimaryDisplay(const display::Display& display);

  // These functions help to manage the list of WindowTreeData structures.
  // AppendWindowTreeData is used to add an uninitialized structure at the end
  // of the list. When a new WindowTreeHostMus is created and is sent to
  // MusDemo (via OnWmNewDisplay or OnEmbed), the WindowTreeData is initialized
  // by a call to InitWindowTreeData and the demo starts. When the destruction
  // of the WindowTreeHostMus is announced to MusDemo (via OnWmDisplayRemoved
  // or OnEmbedRootDestroyed), the corresponding WindowTreeData is removed by
  // a call to RemoveWindowTreeData.
  void AppendWindowTreeData(std::unique_ptr<WindowTreeData> window_tree_data);
  void InitWindowTreeData(
      std::unique_ptr<aura::WindowTreeHostMus> window_tree_host);
  void RemoveWindowTreeData(aura::WindowTreeHostMus* window_tree_host);

  aura::WindowTreeClient* window_tree_client() {
    return window_tree_client_.get();
  }

 private:
  virtual void OnStartImpl() = 0;
  virtual std::unique_ptr<aura::WindowTreeClient> CreateWindowTreeClient() = 0;
  bool HasPendingWindowTreeData() const;

  // service_manager::Service:
  void OnStart() override;

  // aura::WindowTreeClientDelegate:
  void OnEmbed(
      std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) override;
  void OnUnembed(aura::Window* root) override;
  void OnEmbedRootDestroyed(aura::WindowTreeHostMus* window_tree_host) override;
  void OnLostConnection(aura::WindowTreeClient* client) override;
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              int64_t display_id,
                              aura::Window* target) override;
  aura::PropertyConverter* GetPropertyConverter() override;

  std::unique_ptr<aura::WindowTreeClient> window_tree_client_;
  std::unique_ptr<aura::Env> env_;
  std::unique_ptr<display::ScreenBase> screen_;

  std::unique_ptr<aura::client::DefaultCaptureClient> capture_client_;
  std::unique_ptr<::wm::WMState> wm_state_;
  std::unique_ptr<aura::PropertyConverter> property_converter_;

  // List of WindowTreeData structures. When a new window is opened, its data
  // is appended at the end of that list and it remains uninitialized until the
  // next call to InitWindowTreeData.
  std::vector<std::unique_ptr<WindowTreeData>> window_tree_data_list_;

  DISALLOW_COPY_AND_ASSIGN(MusDemo);
};

}  // namespace demo
}  // namespace ui

#endif  // SERVICES_UI_DEMO_MUS_DEMO_H_
