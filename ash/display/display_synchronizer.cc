// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/display_synchronizer.h"

#include "ash/display/mirror_window_controller.h"
#include "ash/host/ash_window_tree_host.h"
#include "ash/shell.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "ui/aura/mus/window_manager_delegate.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/ui_base_switches_util.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/manager/managed_display_info.h"

namespace ash {
namespace {

aura::WindowTreeHostMus* ToWindowTreeHostMus(AshWindowTreeHost* ash_host) {
  return static_cast<aura::WindowTreeHostMus*>(ash_host->AsWindowTreeHost());
}

// Get the metrics for the display with the given |id|.
ui::mojom::WmViewportMetricsPtr GetMetricsForDisplay(int64_t id) {
  ui::mojom::WmViewportMetricsPtr metrics = ui::mojom::WmViewportMetrics::New();
  const display::ManagedDisplayInfo& display_info =
      Shell::Get()->display_manager()->GetDisplayInfo(id);
  metrics->bounds_in_pixels = display_info.bounds_in_native();
  metrics->device_scale_factor = display_info.device_scale_factor();
  metrics->ui_scale_factor = display_info.configured_ui_scale();
  return metrics;
}

}  // namespace

DisplaySynchronizer::DisplaySynchronizer(
    aura::WindowManagerClient* window_manager_client)
    : window_manager_client_(window_manager_client) {
  Shell::Get()->window_tree_host_manager()->AddObserver(this);
  Shell::Get()->display_manager()->AddObserver(this);
  SendDisplayConfigurationToServer();
}

DisplaySynchronizer::~DisplaySynchronizer() {
  Shell::Get()->display_manager()->RemoveObserver(this);
  Shell::Get()->window_tree_host_manager()->RemoveObserver(this);
}

void DisplaySynchronizer::SendDisplayConfigurationToServer() {
  if (processing_display_changes_)
    return;

  display::DisplayManager* display_manager = Shell::Get()->display_manager();
  const size_t display_count = display_manager->GetNumDisplays();
  if (display_count == 0)
    return;

  std::vector<display::Display> displays;
  std::vector<ui::mojom::WmViewportMetricsPtr> metrics;
  for (size_t i = 0; i < display_count; ++i) {
    displays.push_back(display_manager->GetDisplayAt(i));
    metrics.push_back(GetMetricsForDisplay(displays[i].id()));
  }
  std::vector<display::Display> mirrors;
  MirrorWindowController* mirror_window_controller =
      Shell::Get()->window_tree_host_manager()->mirror_window_controller();
  for (const auto& mirror :
       display_manager->software_mirroring_display_list()) {
    if (base::FeatureList::IsEnabled(features::kMash)) {
      // If mus is hosting viz, the window server handle mirrors internally.
      mirrors.push_back(mirror);
      metrics.push_back(GetMetricsForDisplay(mirror.id()));
    } else if (mirror_window_controller->GetAshWindowTreeHostForDisplayId(
                   mirror.id())) {
      // Otherwise, if MirrorWindowController constructed a window tree host
      // the window server should construct a normal ws::Display for the mirror.
      displays.push_back(mirror);
      metrics.push_back(GetMetricsForDisplay(mirror.id()));
    }
  }

  window_manager_client_->SetDisplayConfiguration(
      displays, std::move(metrics),
      WindowTreeHostManager::GetPrimaryDisplayId(), mirrors);

  sent_initial_config_ = true;
}

void DisplaySynchronizer::OnDisplaysInitialized() {
  SendDisplayConfigurationToServer();
}

void DisplaySynchronizer::OnDisplayConfigurationChanged() {
  SendDisplayConfigurationToServer();
}

void DisplaySynchronizer::OnWindowTreeHostReusedForDisplay(
    AshWindowTreeHost* window_tree_host,
    const display::Display& display) {
  aura::WindowTreeHostMus* window_tree_host_mus =
      ToWindowTreeHostMus(window_tree_host);
  if (window_tree_host_mus->display_id() == display.id())
    return;
  const display::ManagedDisplayInfo& display_info =
      Shell::Get()->display_manager()->GetDisplayInfo(display.id());
  ui::mojom::WmViewportMetricsPtr viewport_metrics =
      ui::mojom::WmViewportMetrics::New();
  viewport_metrics->bounds_in_pixels = display_info.bounds_in_native();
  viewport_metrics->device_scale_factor = display.device_scale_factor();
  viewport_metrics->ui_scale_factor = display_info.configured_ui_scale();
  window_manager_client_->AddDisplayReusingWindowTreeHost(
      ToWindowTreeHostMus(window_tree_host), display,
      std::move(viewport_metrics));
}

void DisplaySynchronizer::OnWindowTreeHostsSwappedDisplays(
    AshWindowTreeHost* host1,
    AshWindowTreeHost* host2) {
  DCHECK(host1);
  DCHECK(host2);
  window_manager_client_->SwapDisplayRoots(ToWindowTreeHostMus(host1),
                                           ToWindowTreeHostMus(host2));
}

void DisplaySynchronizer::OnWillProcessDisplayChanges() {
  DCHECK(!processing_display_changes_);
  processing_display_changes_ = true;
}

void DisplaySynchronizer::OnDidProcessDisplayChanges() {
  DCHECK(processing_display_changes_);
  processing_display_changes_ = false;
  SendDisplayConfigurationToServer();
}

void DisplaySynchronizer::OnDisplayMetricsChanged(
    const display::Display& display,
    uint32_t changed_metrics) {
  // Changing only the work area doesn't trigger
  // OnDisplayConfigurationChanged().
  // Wait for the initial config before sending anything as initial display
  // creation may trigger numerous calls to OnDisplayConfigurationChanged().
  if (sent_initial_config_ && changed_metrics == DISPLAY_METRIC_WORK_AREA)
    SendDisplayConfigurationToServer();
}

}  // namespace ash
