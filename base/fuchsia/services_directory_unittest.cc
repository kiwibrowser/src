// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/fuchsia/services_directory.h"

#include <fdio/util.h>

#include "base/bind.h"
#include "base/fuchsia/component_context.h"
#include "base/fuchsia/scoped_service_binding.h"
#include "base/fuchsia/test_fidl/cpp/fidl.h"
#include "base/message_loop/message_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace fuchsia {

class TestInterfaceImpl : public test_fidl::TestInterface {
 public:
  void Add(int32_t a, int32_t b, AddCallback callback) override {
    callback(a + b);
  }
};

// Verifies that a service connected by ServicesDirectory can be imported from
// another ServicesDirectory.
TEST(ServicesDirectoryTest, Connect) {
  MessageLoopForIO message_loop_;

  ScopedZxHandle dir_service_handle;
  ScopedZxHandle dir_client_handle;
  ASSERT_EQ(zx_channel_create(0, dir_service_handle.receive(),
                              dir_client_handle.receive()),
            ZX_OK);

  // Mount service dir and publish the service.
  ServicesDirectory service_dir(std::move(dir_service_handle));
  TestInterfaceImpl test_service;
  ScopedServiceBinding<test_fidl::TestInterface> service_binding(&service_dir,
                                                                 &test_service);

  // Open public directory from the service directory.
  ScopedZxHandle public_dir_service_handle;
  ScopedZxHandle public_dir_client_handle;
  ASSERT_EQ(zx_channel_create(0, public_dir_service_handle.receive(),
                              public_dir_client_handle.receive()),
            ZX_OK);
  ASSERT_EQ(fdio_open_at(dir_client_handle.get(), "public", 0,
                         public_dir_service_handle.release()),
            ZX_OK);

  // Create ComponentContext and connect to the test service.
  ComponentContext client_context(std::move(public_dir_client_handle));
  auto stub = client_context.ConnectToService<test_fidl::TestInterface>();

  // Call the service and wait for response.
  base::RunLoop run_loop;
  bool error = false;

  stub.set_error_handler([&run_loop, &error]() {
    error = true;
    run_loop.Quit();
  });

  stub->Add(2, 2, [&run_loop](int32_t result) {
    EXPECT_EQ(result, 4);
    run_loop.Quit();
  });

  run_loop.Run();

  EXPECT_FALSE(error);
}

}  // namespace fuchsia
}  // namespace base
