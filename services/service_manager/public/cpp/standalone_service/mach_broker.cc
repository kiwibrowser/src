// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/public/cpp/standalone_service/mach_broker.h"

#include "base/logging.h"

namespace service_manager {

namespace {
const char kBootstrapPortName[] = "mojo_service_manager";
}

// static
void MachBroker::SendTaskPortToParent() {
  bool result = base::MachPortBroker::ChildSendTaskPortToParent(
      kBootstrapPortName);
  DCHECK(result);
}

// static
MachBroker* MachBroker::GetInstance() {
  static MachBroker* broker = new MachBroker;
  return broker;
}

MachBroker::MachBroker() : broker_(kBootstrapPortName) {
  bool result = broker_.Init();
  DCHECK(result);
}

MachBroker::~MachBroker() {}

void MachBroker::ExpectPid(base::ProcessHandle pid) {
  broker_.AddPlaceholderForPid(pid);
}

void MachBroker::RemovePid(base::ProcessHandle pid) {
  broker_.InvalidatePid(pid);
}

}  // namespace service_manager
