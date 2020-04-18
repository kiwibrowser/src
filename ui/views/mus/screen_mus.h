// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_MUS_SCREEN_MUS_H_
#define UI_VIEWS_MUS_SCREEN_MUS_H_

#include "mojo/public/cpp/bindings/binding.h"
#include "services/ui/public/interfaces/screen_provider.mojom.h"
#include "ui/display/screen_base.h"
#include "ui/views/mus/mus_export.h"

namespace service_manager {
class Connector;
}

namespace views {

class ScreenMusDelegate;

// Screen implementation backed by ui::mojom::ScreenProvider.
class VIEWS_MUS_EXPORT ScreenMus : public display::ScreenBase,
                                   public ui::mojom::ScreenProviderObserver {
 public:
  explicit ScreenMus(ScreenMusDelegate* delegate);
  ~ScreenMus() override;

  void Init(service_manager::Connector* connector);

 private:
  friend class ScreenMusTestApi;

  // display::Screen:
  display::Display GetDisplayNearestWindow(
      gfx::NativeWindow window) const override;
  gfx::Point GetCursorScreenPoint() override;
  bool IsWindowUnderCursor(gfx::NativeWindow window) override;
  aura::Window* GetWindowAtScreenPoint(const gfx::Point& point) override;

  // ui::mojom::ScreenProvider:
  void OnDisplaysChanged(std::vector<ui::mojom::WsDisplayPtr> ws_displays,
                         int64_t primary_display_id,
                         int64_t internal_display_id) override;

  ScreenMusDelegate* delegate_;
  ui::mojom::ScreenProviderPtr screen_provider_;
  mojo::Binding<ui::mojom::ScreenProviderObserver>
      screen_provider_observer_binding_;

  DISALLOW_COPY_AND_ASSIGN(ScreenMus);
};

}  // namespace views

#endif  // UI_VIEWS_MUS_SCREEN_MUS_H_
