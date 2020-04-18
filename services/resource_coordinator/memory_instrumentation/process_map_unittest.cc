// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/strings/stringprintf.h"
#include "services/resource_coordinator/memory_instrumentation/process_map.h"
#include "services/service_manager/public/cpp/identity.h"
#include "services/service_manager/public/mojom/service_manager.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace memory_instrumentation {

using RunningServiceInfoPtr = service_manager::mojom::RunningServiceInfoPtr;
using ServiceManagerListenerRequest =
    service_manager::mojom::ServiceManagerListenerRequest;

RunningServiceInfoPtr MakeTestServiceInfo(
    const service_manager::Identity& identity,
    uint32_t pid) {
  RunningServiceInfoPtr info(service_manager::mojom::RunningServiceInfo::New());
  info->identity = identity;
  info->pid = pid;
  return info;
}

TEST(ProcessMapTest, PresentInInit) {
  // Add a dummy entry so that the actual |ids| indexes are 1-based.
  std::vector<service_manager::Identity> ids{service_manager::Identity()};
  std::vector<RunningServiceInfoPtr> instances;
  ProcessMap process_map(nullptr);
  for (uint32_t i = 1; i <= 3; i++) {
    ids.push_back(service_manager::Identity(base::StringPrintf("id%d", i)));
    instances.push_back(MakeTestServiceInfo(ids.back(), i /* pid */));
  }
  process_map.OnInit(std::move(instances));
  EXPECT_EQ(static_cast<base::ProcessId>(1), process_map.GetProcessId(ids[1]));
  EXPECT_EQ(static_cast<base::ProcessId>(2), process_map.GetProcessId(ids[2]));
  EXPECT_EQ(static_cast<base::ProcessId>(3), process_map.GetProcessId(ids[3]));

  process_map.OnServiceStopped(ids[1]);
  EXPECT_EQ(base::kNullProcessId, process_map.GetProcessId(ids[1]));
  EXPECT_EQ(static_cast<base::ProcessId>(3), process_map.GetProcessId(ids[3]));
}

TEST(ProcessMapTest, NullsInInitIgnored) {
  ProcessMap process_map(nullptr);
  service_manager::Identity id1("id1");
  service_manager::Identity id2("id2");

  std::vector<RunningServiceInfoPtr> instances;
  instances.push_back(MakeTestServiceInfo(id1, base::kNullProcessId));
  instances.push_back(MakeTestServiceInfo(id2, 2));

  process_map.OnInit(std::move(instances));
  EXPECT_EQ(base::kNullProcessId, process_map.GetProcessId(id1));
  EXPECT_EQ(static_cast<base::ProcessId>(2), process_map.GetProcessId(id2));

  process_map.OnServicePIDReceived(id1, 1 /* pid */);
  EXPECT_EQ(static_cast<base::ProcessId>(1), process_map.GetProcessId(id1));
  EXPECT_EQ(static_cast<base::ProcessId>(2), process_map.GetProcessId(id2));
}

TEST(ProcessMapTest, TypicalCase) {
  ProcessMap process_map(nullptr);
  service_manager::Identity id1("id1");
  EXPECT_EQ(base::kNullProcessId, process_map.GetProcessId(id1));
  process_map.OnInit(std::vector<RunningServiceInfoPtr>());
  EXPECT_EQ(base::kNullProcessId, process_map.GetProcessId(id1));

  process_map.OnServiceCreated(MakeTestServiceInfo(id1, 1 /* pid */));
  process_map.OnServiceStarted(id1, 1 /* pid */);
  EXPECT_EQ(base::kNullProcessId, process_map.GetProcessId(id1));

  process_map.OnServicePIDReceived(id1, 1 /* pid */);
  EXPECT_EQ(static_cast<base::ProcessId>(1), process_map.GetProcessId(id1));

  // Adding a separate service with a different identity should have no effect
  // on the first identity registered.
  service_manager::Identity id2("id2");
  process_map.OnServiceCreated(MakeTestServiceInfo(id2, 2 /* pid */));
  process_map.OnServicePIDReceived(id2, 2 /* pid */);
  EXPECT_EQ(static_cast<base::ProcessId>(1), process_map.GetProcessId(id1));
  EXPECT_EQ(static_cast<base::ProcessId>(2), process_map.GetProcessId(id2));

  // Once the service is stopped, searching for its id should return a null pid.
  process_map.OnServiceStopped(id1);
  EXPECT_EQ(base::kNullProcessId, process_map.GetProcessId(id1));
  EXPECT_EQ(static_cast<base::ProcessId>(2), process_map.GetProcessId(id2));

  // Unknown identities return a null pid.
  service_manager::Identity id3("id3");
  EXPECT_EQ(base::kNullProcessId, process_map.GetProcessId(id3));
}

}  // namespace memory_instrumentation
