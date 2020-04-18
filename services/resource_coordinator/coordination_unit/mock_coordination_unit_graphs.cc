// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/mock_coordination_unit_graphs.h"

#include <string>

#include "services/resource_coordinator/coordination_unit/coordination_unit_base.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/page_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/process_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/system_coordination_unit_impl.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_id.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_types.h"

namespace service_manager {
class ServiceContextRef;
}

namespace resource_coordinator {

MockSinglePageInSingleProcessCoordinationUnitGraph::
    MockSinglePageInSingleProcessCoordinationUnitGraph()
    : system(TestCoordinationUnitWrapper<SystemCoordinationUnitImpl>::Create()),
      frame(TestCoordinationUnitWrapper<FrameCoordinationUnitImpl>::Create()),
      process(
          TestCoordinationUnitWrapper<ProcessCoordinationUnitImpl>::Create()),
      page(TestCoordinationUnitWrapper<PageCoordinationUnitImpl>::Create()) {
  page->AddFrame(frame->id());
  process->AddFrame(frame->id());
  process->SetPID(1);
}

MockSinglePageInSingleProcessCoordinationUnitGraph::
    ~MockSinglePageInSingleProcessCoordinationUnitGraph() = default;

MockMultiplePagesInSingleProcessCoordinationUnitGraph::
    MockMultiplePagesInSingleProcessCoordinationUnitGraph()
    : other_frame(
          TestCoordinationUnitWrapper<FrameCoordinationUnitImpl>::Create()),
      other_page(
          TestCoordinationUnitWrapper<PageCoordinationUnitImpl>::Create()) {
  other_page->AddFrame(other_frame->id());
  process->AddFrame(other_frame->id());
}

MockMultiplePagesInSingleProcessCoordinationUnitGraph::
    ~MockMultiplePagesInSingleProcessCoordinationUnitGraph() = default;

MockSinglePageWithMultipleProcessesCoordinationUnitGraph::
    MockSinglePageWithMultipleProcessesCoordinationUnitGraph()
    : child_frame(
          TestCoordinationUnitWrapper<FrameCoordinationUnitImpl>::Create()),
      other_process(
          TestCoordinationUnitWrapper<ProcessCoordinationUnitImpl>::Create()) {
  frame->AddChildFrame(child_frame->id());
  page->AddFrame(child_frame->id());
  other_process->AddFrame(child_frame->id());
  other_process->SetPID(2);
}

MockSinglePageWithMultipleProcessesCoordinationUnitGraph::
    ~MockSinglePageWithMultipleProcessesCoordinationUnitGraph() = default;

MockMultiplePagesWithMultipleProcessesCoordinationUnitGraph::
    MockMultiplePagesWithMultipleProcessesCoordinationUnitGraph()
    : child_frame(
          TestCoordinationUnitWrapper<FrameCoordinationUnitImpl>::Create()),
      other_process(
          TestCoordinationUnitWrapper<ProcessCoordinationUnitImpl>::Create()) {
  other_frame->AddChildFrame(child_frame->id());
  other_page->AddFrame(child_frame->id());
  other_process->AddFrame(child_frame->id());
  other_process->SetPID(2);
}

MockMultiplePagesWithMultipleProcessesCoordinationUnitGraph::
    ~MockMultiplePagesWithMultipleProcessesCoordinationUnitGraph() = default;

}  // namespace resource_coordinator
