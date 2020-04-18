// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_OBSERVER_IPC_VOLUME_REPORTER_H_
#define SERVICES_RESOURCE_COORDINATOR_OBSERVER_IPC_VOLUME_REPORTER_H_

#include "base/metrics/histogram_macros.h"
#include "base/timer/timer.h"
#include "services/resource_coordinator/observers/coordination_unit_graph_observer.h"
#include "services/resource_coordinator/resource_coordinator_clock.h"

namespace resource_coordinator {

class IPCVolumeReporter : public CoordinationUnitGraphObserver {
 public:
  IPCVolumeReporter(std::unique_ptr<base::Timer> timer);
  ~IPCVolumeReporter() override;
  // CoordinationUnitGraphObserver implementation.
  bool ShouldObserve(const CoordinationUnitBase* coordination_unit) override;
  void OnFramePropertyChanged(const FrameCoordinationUnitImpl* frame_cu,
                              const mojom::PropertyType property_type,
                              int64_t value) override;
  void OnPagePropertyChanged(const PageCoordinationUnitImpl* page_cu,
                             const mojom::PropertyType property_type,
                             int64_t value) override;
  void OnProcessPropertyChanged(const ProcessCoordinationUnitImpl* process_cu,
                                const mojom::PropertyType property_type,
                                int64_t value) override;
  void OnFrameEventReceived(const FrameCoordinationUnitImpl* frame_cu,
                            const mojom::Event event) override;
  void OnPageEventReceived(const PageCoordinationUnitImpl* page_cu,
                           const mojom::Event event) override;
  void OnProcessEventReceived(const ProcessCoordinationUnitImpl* process_cu,
                              const mojom::Event event) override;

 protected:
  base::Timer* timer() const { return timer_.get(); }

 private:
  void ReportIPCVolume();

  std::unique_ptr<base::Timer> timer_;
  uint32_t frame_ipc_count_;
  uint32_t page_ipc_count_;
  uint32_t process_ipc_count_;

  DISALLOW_COPY_AND_ASSIGN(IPCVolumeReporter);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_OBSERVER_IPC_VOLUME_REPORTER_H_
