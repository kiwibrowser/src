// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/background/background_service_manager.h"

#include <memory>

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/values.h"
#include "services/service_manager/background/tests/test.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace service_manager {
namespace {

const char kTestName[] = "background_service_manager_unittest";
const char kAppName[] = "background_service_manager_test_service";

// The parent unit test suite service, not the underlying test service.
class ServiceImpl : public Service {
 public:
  ServiceImpl() {}
  ~ServiceImpl() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ServiceImpl);
};

void SetFlagAndRunClosure(bool* flag, const base::Closure& closure) {
  *flag = true;
  closure.Run();
}

// Uses BackgroundServiceManager to start the service manager in the background
// and connects to background_service_manager_test_service, verifying we can
// send a message to the service.
#if defined(OS_ANDROID)
// TODO(crbug.com/589784): This test is disabled, as it fails
// on the Android GN bot.
#define MAYBE_Basic DISABLED_Basic
#else
#define MAYBE_Basic Basic
#endif
TEST(BackgroundServiceManagerTest, MAYBE_Basic) {
  base::test::ScopedTaskEnvironment scoped_task_environment;
  BackgroundServiceManager background_service_manager(nullptr, nullptr);
  mojom::ServicePtr service;
  ServiceContext service_context(std::make_unique<ServiceImpl>(),
                                 mojo::MakeRequest(&service));
  background_service_manager.RegisterService(
      Identity(kTestName, mojom::kRootUserID), std::move(service), nullptr);

  mojom::TestServicePtr test_service;
  service_context.connector()->BindInterface(kAppName, &test_service);
  base::RunLoop run_loop;
  bool got_result = false;
  test_service->Test(
      base::Bind(&SetFlagAndRunClosure, &got_result, run_loop.QuitClosure()));
  run_loop.Run();
  EXPECT_TRUE(got_result);
}

}  // namespace
}  // namespace service_manager
