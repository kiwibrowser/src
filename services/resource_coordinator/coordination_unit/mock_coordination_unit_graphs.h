// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_MOCK_COORDINATION_UNIT_GRAPHS_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_MOCK_COORDINATION_UNIT_GRAPHS_H_

#include "services/resource_coordinator/coordination_unit/coordination_unit_test_harness.h"

namespace resource_coordinator {

class FrameCoordinationUnitImpl;
class PageCoordinationUnitImpl;
class ProcessCoordinationUnitImpl;
class SystemCoordinationUnitImpl;

// The following coordination unit graph topology is created to emulate a
// scenario when a single page executes in a single process:
//
// Pr  Pg
//  \ /
//   F
//
// Where:
// F: frame
// Pr: process(pid:1)
// Pg: page
struct MockSinglePageInSingleProcessCoordinationUnitGraph {
  MockSinglePageInSingleProcessCoordinationUnitGraph();
  ~MockSinglePageInSingleProcessCoordinationUnitGraph();
  TestCoordinationUnitWrapper<SystemCoordinationUnitImpl> system;
  TestCoordinationUnitWrapper<FrameCoordinationUnitImpl> frame;
  TestCoordinationUnitWrapper<ProcessCoordinationUnitImpl> process;
  TestCoordinationUnitWrapper<PageCoordinationUnitImpl> page;
};

// The following coordination unit graph topology is created to emulate a
// scenario where multiple pages are executing in a single process:
//
// Pg  Pr OPg
//  \ / \ /
//   F  OF
//
// Where:
// F: frame
// OF: other_frame
// Pg: page
// OPg: other_page
// Pr: process(pid:1)
struct MockMultiplePagesInSingleProcessCoordinationUnitGraph
    : public MockSinglePageInSingleProcessCoordinationUnitGraph {
  MockMultiplePagesInSingleProcessCoordinationUnitGraph();
  ~MockMultiplePagesInSingleProcessCoordinationUnitGraph();
  TestCoordinationUnitWrapper<FrameCoordinationUnitImpl> other_frame;
  TestCoordinationUnitWrapper<PageCoordinationUnitImpl> other_page;
};

// The following coordination unit graph topology is created to emulate a
// scenario where a single page that has frames is executing in different
// processes (e.g. out-of-process iFrames):
//
// Pg  Pr
// |\ /
// | F  OPr
// |  \ /
// |__CF
//
// Where:
// F: frame
// CF: child_frame
// Pg: page
// Pr: process(pid:1)
// OPr: other_process(pid:2)
struct MockSinglePageWithMultipleProcessesCoordinationUnitGraph
    : public MockSinglePageInSingleProcessCoordinationUnitGraph {
  MockSinglePageWithMultipleProcessesCoordinationUnitGraph();
  ~MockSinglePageWithMultipleProcessesCoordinationUnitGraph();
  TestCoordinationUnitWrapper<FrameCoordinationUnitImpl> child_frame;
  TestCoordinationUnitWrapper<ProcessCoordinationUnitImpl> other_process;
};

// The following coordination unit graph topology is created to emulate a
// scenario where multiple pages are utilizing multiple processes (e.g.
// out-of-process iFrames and multiple pages in a process):
//
// Pg  Pr OPg___
//  \ / \ /     |
//   F   OF OPr |
//        \ /   |
//         CF___|
//
// Where:
// F: frame
// OF: other_frame
// CF: another_frame
// Pg: page
// OPg: other_page
// Pr: process(pid:1)
// OPr: other_process(pid:2)
struct MockMultiplePagesWithMultipleProcessesCoordinationUnitGraph
    : public MockMultiplePagesInSingleProcessCoordinationUnitGraph {
  MockMultiplePagesWithMultipleProcessesCoordinationUnitGraph();
  ~MockMultiplePagesWithMultipleProcessesCoordinationUnitGraph();
  TestCoordinationUnitWrapper<FrameCoordinationUnitImpl> child_frame;
  TestCoordinationUnitWrapper<ProcessCoordinationUnitImpl> other_process;
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_MOCK_COORDINATION_UNIT_GRAPHS_H_
