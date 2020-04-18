// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_WINDOW_SERVER_SERVICE_TEST_BASE_H_
#define SERVICES_UI_WS2_WINDOW_SERVER_SERVICE_TEST_BASE_H_

#include "base/macros.h"
#include "services/service_manager/public/cpp/service_test.h"

namespace ui {
namespace ws2 {

// Base class for all window manager ServiceTests to perform some common setup.
class WindowServerServiceTestBase : public service_manager::test::ServiceTest {
 public:
  WindowServerServiceTestBase();
  ~WindowServerServiceTestBase() override;

  virtual void OnBindInterface(
      const service_manager::BindSourceInfo& source_info,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle interface_pipe) = 0;

 private:
  // service_manager::test::ServiceTest:
  std::unique_ptr<service_manager::Service> CreateService() override;

  DISALLOW_COPY_AND_ASSIGN(WindowServerServiceTestBase);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_WINDOW_SERVER_SERVICE_TEST_BASE_H_
