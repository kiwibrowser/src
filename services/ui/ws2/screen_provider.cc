// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/screen_provider.h"

#include "ui/display/screen.h"

using display::Display;
using display::Screen;

namespace ui {
namespace ws2 {
namespace {

int64_t GetPrimaryDisplayId() {
  return Screen::GetScreen()->GetPrimaryDisplay().id();
}

int64_t GetInternalDisplayId() {
  return Display::HasInternalDisplay() ? Display::InternalDisplayId()
                                       : display::kInvalidDisplayId;
}

}  // namespace

ScreenProvider::ScreenProvider() {
  Screen::GetScreen()->AddObserver(this);
}

ScreenProvider::~ScreenProvider() {
  Screen::GetScreen()->RemoveObserver(this);
}

void ScreenProvider::AddBinding(mojom::ScreenProviderRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void ScreenProvider::SetFrameDecorationValues(
    const gfx::Insets& client_area_insets,
    int max_title_bar_button_width) {
  client_area_insets_ = client_area_insets;
  max_title_bar_button_width_ = max_title_bar_button_width;
}

void ScreenProvider::AddObserver(mojom::ScreenProviderObserverPtr observer) {
  mojom::ScreenProviderObserver* observer_impl = observer.get();
  observers_.AddPtr(std::move(observer));
  NotifyObserver(observer_impl);
}

void ScreenProvider::OnDidProcessDisplayChanges() {
  // Display changes happen in batches, so notify observers after the batch is
  // complete, rather than on every add/remove/metrics change.
  NotifyAllObservers();
}

void ScreenProvider::NotifyAllObservers() {
  observers_.ForAllPtrs([this](mojom::ScreenProviderObserver* observer) {
    NotifyObserver(observer);
  });
}

void ScreenProvider::NotifyObserver(mojom::ScreenProviderObserver* observer) {
  observer->OnDisplaysChanged(GetAllDisplays(), GetPrimaryDisplayId(),
                              GetInternalDisplayId());
}

std::vector<mojom::WsDisplayPtr> ScreenProvider::GetAllDisplays() {
  std::vector<Display> displays = Screen::GetScreen()->GetAllDisplays();

  std::vector<mojom::WsDisplayPtr> ws_displays;
  ws_displays.reserve(displays.size());

  for (const Display& display : displays) {
    mojom::WsDisplayPtr ws_display = mojom::WsDisplay::New();
    ws_display->display = display;
    ws_display->frame_decoration_values = GetFrameDecorationValues();
    ws_displays.push_back(std::move(ws_display));
  }

  return ws_displays;
}

mojom::FrameDecorationValuesPtr ScreenProvider::GetFrameDecorationValues() {
  mojom::FrameDecorationValuesPtr values = mojom::FrameDecorationValues::New();
  // TODO(jamescook): These insets are always the same. Collapse them.
  values->normal_client_area_insets = client_area_insets_;
  values->maximized_client_area_insets = client_area_insets_;
  values->max_title_bar_button_width = max_title_bar_button_width_;
  return values;
}

}  // namespace ws2
}  // namespace ui
