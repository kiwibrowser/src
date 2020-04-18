// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_PROCESS_MAP_H_
#define SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_PROCESS_MAP_H_

#include <map>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/process/process_handle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/identity.h"
#include "services/service_manager/public/mojom/service_manager.mojom.h"

namespace service_manager {
class Connector;
}  // namespace service_manager

namespace memory_instrumentation {

// Maintains a map from service_manager::Identity to base::ProcessId by
// listening for connections. This allows |pid| lookup by
// service_manager::Identity. The assumption is that all the processes that will
// register, via their client libraries, to the memory instrumentation service
// will also necessarily connect to the service manager.
class ProcessMap : public service_manager::mojom::ServiceManagerListener {
 public:
  explicit ProcessMap(service_manager::Connector* connector);
  ~ProcessMap() override;

  // Returns the pid for a client given its identity.
  base::ProcessId GetProcessId(const service_manager::Identity&) const;

 protected:
 private:
  FRIEND_TEST_ALL_PREFIXES(ProcessMapTest, TypicalCase);
  FRIEND_TEST_ALL_PREFIXES(ProcessMapTest, PresentInInit);
  FRIEND_TEST_ALL_PREFIXES(ProcessMapTest, NullsInInitIgnored);

  using Identity = service_manager::Identity;
  using RunningServiceInfoPtr = service_manager::mojom::RunningServiceInfoPtr;

  // Overridden from service_manager::mojom::ServiceManagerListener.
  void OnInit(std::vector<RunningServiceInfoPtr> instances) override;
  void OnServiceCreated(RunningServiceInfoPtr instance) override;
  void OnServiceStarted(const Identity&, uint32_t pid) override;
  void OnServiceFailedToStart(const Identity&) override;
  void OnServiceStopped(const Identity&) override;
  void OnServicePIDReceived(const service_manager::Identity& identity,
                            uint32_t pid) override;

  mojo::Binding<service_manager::mojom::ServiceManagerListener> binding_;
  std::map<Identity, base::ProcessId> instances_;

  DISALLOW_COPY_AND_ASSIGN(ProcessMap);
};

}  // namespace memory_instrumentation
#endif  // SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_PROCESS_MAP_H_
