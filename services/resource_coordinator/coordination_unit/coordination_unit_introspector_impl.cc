// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/coordination_unit_introspector_impl.h"

#include <vector>

#include "base/process/process_handle.h"
#include "base/time/time.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/page_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/process_coordination_unit_impl.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace resource_coordinator {

CoordinationUnitIntrospectorImpl::CoordinationUnitIntrospectorImpl() = default;

CoordinationUnitIntrospectorImpl::~CoordinationUnitIntrospectorImpl() = default;

void CoordinationUnitIntrospectorImpl::GetProcessToURLMap(
    GetProcessToURLMapCallback callback) {
  std::vector<resource_coordinator::mojom::ProcessInfoPtr> process_infos;
  std::vector<ProcessCoordinationUnitImpl*> process_cus =
      ProcessCoordinationUnitImpl::GetAllProcessCoordinationUnits();
  for (auto* process_cu : process_cus) {
    int64_t pid;
    if (!process_cu->GetProperty(mojom::PropertyType::kPID, &pid))
      continue;

    mojom::ProcessInfoPtr process_info(mojom::ProcessInfo::New());
    process_info->pid = pid;
    DCHECK_NE(base::kNullProcessId, process_info->pid);

    int64_t launch_time;
    if (process_cu->GetProperty(mojom::PropertyType::kLaunchTime,
                                &launch_time)) {
      process_info->launch_time = base::Time::FromTimeT(launch_time);
    }

    std::set<PageCoordinationUnitImpl*> page_cus =
        process_cu->GetAssociatedPageCoordinationUnits();
    std::vector<resource_coordinator::mojom::PageInfoPtr> page_infos;
    for (PageCoordinationUnitImpl* page_cu : page_cus) {
      int64_t ukm_source_id;
      if (page_cu->GetProperty(
              resource_coordinator::mojom::PropertyType::kUKMSourceId,
              &ukm_source_id)) {
        mojom::PageInfoPtr page_info(mojom::PageInfo::New());
        page_info->ukm_source_id = ukm_source_id;
        page_info->is_visible = page_cu->IsVisible();
        page_info->time_since_last_visibility_change =
            page_cu->TimeSinceLastVisibilityChange();
        page_info->time_since_last_navigation =
            page_cu->TimeSinceLastNavigation();
        process_info->page_infos.push_back(std::move(page_info));
      }
    }
    process_infos.push_back(std::move(process_info));
  }
  std::move(callback).Run(std::move(process_infos));
}

void CoordinationUnitIntrospectorImpl::BindToInterface(
    resource_coordinator::mojom::CoordinationUnitIntrospectorRequest request,
    const service_manager::BindSourceInfo& source_info) {
  bindings_.AddBinding(this, std::move(request));
}

}  // namespace resource_coordinator
