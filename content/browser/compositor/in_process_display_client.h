// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_IN_PROCESS_DISPLAY_CLIENT_H_
#define CONTENT_BROWSER_COMPOSITOR_IN_PROCESS_DISPLAY_CLIENT_H_

#include <memory>
#include <vector>

#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/privileged/interfaces/compositing/display_private.mojom.h"
#include "ui/gfx/native_widget_types.h"

namespace viz {
class LayeredWindowUpdaterImpl;
}

namespace content {

// A DisplayClient that can be used to display received
// gfx::CALayerParams in a CALayer tree in this process.
class InProcessDisplayClient : public viz::mojom::DisplayClient {
 public:
  explicit InProcessDisplayClient(gfx::AcceleratedWidget widget);
  ~InProcessDisplayClient() override;

  viz::mojom::DisplayClientPtr GetBoundPtr(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

 private:
  // viz::mojom::DisplayClient implementation:
  void OnDisplayReceivedCALayerParams(
      const gfx::CALayerParams& ca_layer_params) override;
  void DidSwapAfterSnapshotRequestReceived(
      const std::vector<ui::LatencyInfo>& latency_info) override;
  void CreateLayeredWindowUpdater(
      viz::mojom::LayeredWindowUpdaterRequest request) override;

  mojo::Binding<viz::mojom::DisplayClient> binding_;
#if defined(OS_MACOSX) || defined(OS_WIN)
  gfx::AcceleratedWidget widget_;
#endif

#if defined(OS_WIN)
  std::unique_ptr<viz::LayeredWindowUpdaterImpl> layered_window_updater_;
#endif
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_IN_PROCESS_DISPLAY_CLIENT_H_
