// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/process_coordination_unit_impl.h"

#include "base/logging.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/page_coordination_unit_impl.h"

namespace resource_coordinator {

// static
std::vector<ProcessCoordinationUnitImpl*>
ProcessCoordinationUnitImpl::GetAllProcessCoordinationUnits() {
  auto cus = CoordinationUnitBase::GetCoordinationUnitsOfType(
      CoordinationUnitType::kProcess);
  std::vector<ProcessCoordinationUnitImpl*> process_cus;
  for (auto* process_cu : cus) {
    process_cus.push_back(
        ProcessCoordinationUnitImpl::FromCoordinationUnitBase(process_cu));
  }
  return process_cus;
}

ProcessCoordinationUnitImpl::ProcessCoordinationUnitImpl(
    const CoordinationUnitID& id,
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : CoordinationUnitInterface(id, std::move(service_ref)) {}

ProcessCoordinationUnitImpl::~ProcessCoordinationUnitImpl() {
  for (auto* child_frame : frame_coordination_units_)
    child_frame->RemoveProcessCoordinationUnit(this);
}

void ProcessCoordinationUnitImpl::AddFrame(const CoordinationUnitID& cu_id) {
  DCHECK(cu_id.type == CoordinationUnitType::kFrame);
  auto* frame_cu = FrameCoordinationUnitImpl::GetCoordinationUnitByID(cu_id);
  if (!frame_cu)
    return;
  if (AddFrame(frame_cu)) {
    frame_cu->AddProcessCoordinationUnit(this);
  }
}

void ProcessCoordinationUnitImpl::RemoveFrame(const CoordinationUnitID& cu_id) {
  DCHECK(cu_id != id());
  FrameCoordinationUnitImpl* frame_cu =
      FrameCoordinationUnitImpl::GetCoordinationUnitByID(cu_id);
  if (!frame_cu)
    return;
  if (RemoveFrame(frame_cu)) {
    frame_cu->RemoveProcessCoordinationUnit(this);
  }
}

void ProcessCoordinationUnitImpl::SetCPUUsage(double cpu_usage) {
  SetProperty(mojom::PropertyType::kCPUUsage, cpu_usage * 1000);
}

void ProcessCoordinationUnitImpl::SetExpectedTaskQueueingDuration(
    base::TimeDelta duration) {
  SetProperty(mojom::PropertyType::kExpectedTaskQueueingDuration,
              duration.InMilliseconds());
}

void ProcessCoordinationUnitImpl::SetLaunchTime(base::Time launch_time) {
  SetProperty(mojom::PropertyType::kLaunchTime, launch_time.ToTimeT());
}

void ProcessCoordinationUnitImpl::SetMainThreadTaskLoadIsLow(
    bool main_thread_task_load_is_low) {
  SetProperty(mojom::PropertyType::kMainThreadTaskLoadIsLow,
              main_thread_task_load_is_low);
}

void ProcessCoordinationUnitImpl::SetPID(int64_t pid) {
  SetProperty(mojom::PropertyType::kPID, pid);
}

const std::set<FrameCoordinationUnitImpl*>&
ProcessCoordinationUnitImpl::GetFrameCoordinationUnits() const {
  return frame_coordination_units_;
}

// There is currently not a direct relationship between processes and
// pages. However, frames are children of both processes and frames, so we
// find all of the pages that are reachable from the process's child
// frames.
std::set<PageCoordinationUnitImpl*>
ProcessCoordinationUnitImpl::GetAssociatedPageCoordinationUnits() const {
  std::set<PageCoordinationUnitImpl*> page_cus;
  for (auto* frame_cu : frame_coordination_units_) {
    if (auto* page_cu = frame_cu->GetPageCoordinationUnit())
      page_cus.insert(page_cu);
  }
  return page_cus;
}

void ProcessCoordinationUnitImpl::OnPropertyChanged(
    const mojom::PropertyType property_type,
    int64_t value) {
  for (auto& observer : observers())
    observer.OnProcessPropertyChanged(this, property_type, value);
}

bool ProcessCoordinationUnitImpl::AddFrame(
    FrameCoordinationUnitImpl* frame_cu) {
  bool success = frame_coordination_units_.count(frame_cu)
                     ? false
                     : frame_coordination_units_.insert(frame_cu).second;
  return success;
}

bool ProcessCoordinationUnitImpl::RemoveFrame(
    FrameCoordinationUnitImpl* frame_cu) {
  return frame_coordination_units_.erase(frame_cu) > 0;
}

}  // namespace resource_coordinator
