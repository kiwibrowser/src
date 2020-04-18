// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_COMPOSITING_MODE_REPORTER_IMPL_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_COMPOSITING_MODE_REPORTER_IMPL_H_

#include "base/macros.h"
#include "components/viz/service/viz_service_export.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/viz/public/interfaces/compositing/compositing_mode_watcher.mojom.h"

namespace viz {

class VIZ_SERVICE_EXPORT CompositingModeReporterImpl
    : public mojom::CompositingModeReporter {
 public:
  // Creates the CompositingModeReporterImpl and binds it to the deferred mojo
  // pointer behind the |request|.
  CompositingModeReporterImpl();
  ~CompositingModeReporterImpl() override;

  // Called for each consumer of the CompositingModeReporter interface, to
  // fulfill a mojo pointer for them.
  void BindRequest(mojom::CompositingModeReporterRequest request);

  // Call to inform the reporter that software compositing is being used instead
  // of gpu. This is a one-way setting that can not be reverted. This will
  // notify any registered CompositingModeWatchers.
  void SetUsingSoftwareCompositing();

  // mojom::CompositingModeReporter implementation.
  void AddCompositingModeWatcher(
      mojom::CompositingModeWatcherPtr watcher) override;

 private:
  bool gpu_ = true;
  mojo::BindingSet<mojom::CompositingModeReporter> bindings_;
  mojo::InterfacePtrSet<mojom::CompositingModeWatcher> watchers_;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_COMPOSITING_MODE_REPORTER_IMPL_H_
