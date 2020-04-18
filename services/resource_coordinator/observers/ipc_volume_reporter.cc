// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/observers/ipc_volume_reporter.h"

#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"

namespace resource_coordinator {

#define UMA_FRAME_IPC_COUNT_PER_MINUTE(ipc_count)                       \
  UMA_HISTOGRAM_CUSTOM_COUNTS("ResourceCoordinator.IPCPerMinute.Frame", \
                              ipc_count, 1, 1000, 25)

#define UMA_PAGE_IPC_COUNT_PER_MINUTE(ipc_count)                       \
  UMA_HISTOGRAM_CUSTOM_COUNTS("ResourceCoordinator.IPCPerMinute.Page", \
                              ipc_count, 1, 1000, 25)

#define UMA_PROCESS_IPC_COUNT_PER_MINUTE(ipc_count)                       \
  UMA_HISTOGRAM_CUSTOM_COUNTS("ResourceCoordinator.IPCPerMinute.Process", \
                              ipc_count, 1, 1000, 25)

const base::TimeDelta kReportInterval = base::TimeDelta::FromMinutes(1);

IPCVolumeReporter::IPCVolumeReporter(std::unique_ptr<base::Timer> timer)
    : timer_(std::move(timer)),
      frame_ipc_count_(0),
      page_ipc_count_(0),
      process_ipc_count_(0) {
  timer_->Start(FROM_HERE, kReportInterval,
                base::BindRepeating(&IPCVolumeReporter::ReportIPCVolume,
                                    base::Unretained(this)));
}

IPCVolumeReporter::~IPCVolumeReporter() {
  timer_->Stop();
}

bool IPCVolumeReporter::ShouldObserve(
    const CoordinationUnitBase* coordination_unit) {
  return true;
}

void IPCVolumeReporter::OnFramePropertyChanged(
    const FrameCoordinationUnitImpl* frame_cu,
    const mojom::PropertyType property_type,
    int64_t value) {
  ++frame_ipc_count_;
}

void IPCVolumeReporter::OnPagePropertyChanged(
    const PageCoordinationUnitImpl* page_cu,
    const mojom::PropertyType property_type,
    int64_t value) {
  ++page_ipc_count_;
}

void IPCVolumeReporter::OnProcessPropertyChanged(
    const ProcessCoordinationUnitImpl* process_cu,
    const mojom::PropertyType property_type,
    int64_t value) {
  ++process_ipc_count_;
}

void IPCVolumeReporter::OnFrameEventReceived(
    const FrameCoordinationUnitImpl* frame_cu,
    const mojom::Event event) {
  ++frame_ipc_count_;
}

void IPCVolumeReporter::OnPageEventReceived(
    const PageCoordinationUnitImpl* page_cu,
    const mojom::Event event) {
  ++page_ipc_count_;
}

void IPCVolumeReporter::OnProcessEventReceived(
    const ProcessCoordinationUnitImpl* process_cu,
    const mojom::Event event) {
  ++process_ipc_count_;
}

void IPCVolumeReporter::ReportIPCVolume() {
  UMA_FRAME_IPC_COUNT_PER_MINUTE(frame_ipc_count_);
  UMA_PAGE_IPC_COUNT_PER_MINUTE(page_ipc_count_);
  UMA_PROCESS_IPC_COUNT_PER_MINUTE(process_ipc_count_);
  frame_ipc_count_ = 0;
  page_ipc_count_ = 0;
  process_ipc_count_ = 0;

  timer_->Start(FROM_HERE, kReportInterval,
                base::BindRepeating(&IPCVolumeReporter::ReportIPCVolume,
                                    base::Unretained(this)));
}

}  // namespace resource_coordiantor
