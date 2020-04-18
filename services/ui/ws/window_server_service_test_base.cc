// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/window_server_service_test_base.h"

#include <memory>

#include "base/command_line.h"
#include "base/macros.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "services/ui/common/switches.h"
#include "ui/gl/gl_switches.h"

namespace ui {

namespace {

const char kTestAppName[] = "ui_service_unittests";

class WindowServerServiceTestClient
    : public service_manager::test::ServiceTestClient {
 public:
  explicit WindowServerServiceTestClient(WindowServerServiceTestBase* test)
      : ServiceTestClient(test), test_(test) {}
  ~WindowServerServiceTestClient() override {}

 private:
  // service_manager::test::ServiceTestClient:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    test_->OnBindInterface(source_info, interface_name,
                           std::move(interface_pipe));
  }

  WindowServerServiceTestBase* test_;

  DISALLOW_COPY_AND_ASSIGN(WindowServerServiceTestClient);
};

void EnsureCommandLineSwitch(const std::string& name) {
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (!cmd_line->HasSwitch(name))
    cmd_line->AppendSwitch(name);
}

}  // namespace

WindowServerServiceTestBase::WindowServerServiceTestBase()
    : ServiceTest(kTestAppName) {
  EnsureCommandLineSwitch(switches::kUseTestConfig);
  EnsureCommandLineSwitch(::switches::kOverrideUseSoftwareGLForTests);
}

WindowServerServiceTestBase::~WindowServerServiceTestBase() {}

std::unique_ptr<service_manager::Service>
WindowServerServiceTestBase::CreateService() {
  return std::make_unique<WindowServerServiceTestClient>(this);
}

}  // namespace ui
