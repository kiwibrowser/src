// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/components/quick_launch/public/mojom/constants.mojom.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/public/interfaces/window_server_test.mojom.h"
#include "ui/views/layout/layout_provider.h"

namespace ash {

void RunCallback(bool* success, base::RepeatingClosure callback, bool result) {
  *success = result;
  std::move(callback).Run();
}

class AppLaunchTest : public service_manager::test::ServiceTest {
 public:
  AppLaunchTest() : ServiceTest("mash_unittests") {}
  ~AppLaunchTest() override = default;

 private:
  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch("use-test-config");
    ServiceTest::SetUp();
  }

  views::LayoutProvider layout_provider_;

  DISALLOW_COPY_AND_ASSIGN(AppLaunchTest);
};

TEST_F(AppLaunchTest, TestQuickLaunch) {
  connector()->StartService(mojom::kServiceName);
  connector()->StartService(quick_launch::mojom::kServiceName);

  ui::mojom::WindowServerTestPtr test_interface;
  connector()->BindInterface(ui::mojom::kServiceName, &test_interface);

  base::RunLoop run_loop;
  bool success = false;
  test_interface->EnsureClientHasDrawnWindow(
      quick_launch::mojom::kServiceName,
      base::BindOnce(&RunCallback, &success, run_loop.QuitClosure()));
  run_loop.Run();
  EXPECT_TRUE(success);
}

}  // namespace ash
