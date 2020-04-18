// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/test/ash_test_suite.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/debug/debugger.h"
#include "base/message_loop/message_loop_current.h"
#include "base/process/launch.h"
#include "base/run_loop.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread.h"
#include "build/build_config.h"
#include "components/exo/wayland/clients/test/wayland_client_test.h"
#include "mojo/edk/embedder/embedder.h"

namespace exo {
namespace {

const char kRunWithExternalWaylandServer[] = "run-with-external-wayland-server";

class ExoClientPerfTestSuite : public ash::AshTestSuite {
 public:
  ExoClientPerfTestSuite(int argc, char** argv)
      : ash::AshTestSuite(argc, argv),
        run_with_external_wayland_server_(
            base::CommandLine::ForCurrentProcess()->HasSwitch(
                kRunWithExternalWaylandServer)) {}

  int Run() {
    Initialize();

    base::Thread client_thread("ClientThread");
    client_thread.Start();

    base::RunLoop run_loop;
    client_thread.message_loop()->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&ExoClientPerfTestSuite::RunTestsOnClientThread,
                   base::Unretained(this), run_loop.QuitWhenIdleClosure()));
    run_loop.Run();

    Shutdown();
    return result_;
  }

 private:
  // Overriden from ash::AshTestSuite:
  void Initialize() override {
    if (!base::debug::BeingDebugged())
      base::RaiseProcessToHighPriority();

    if (run_with_external_wayland_server_) {
      base::TestSuite::Initialize();

      scoped_task_environment_ =
          std::make_unique<base::test::ScopedTaskEnvironment>(
              base::test::ScopedTaskEnvironment::MainThreadType::UI);
    } else {
      // We only need initialized ash related stuff for running wayland server
      // within the test.
      ash::AshTestSuite::Initialize();

      // Initialize task envrionment here instead of Test::SetUp(), because all
      // tests and their SetUp() will be running in client thread.
      scoped_task_environment_ =
          std::make_unique<base::test::ScopedTaskEnvironment>(
              base::test::ScopedTaskEnvironment::MainThreadType::UI);

      // Set the UI thread message loop to WaylandClientTest, so all tests can
      // post tasks to UI thread.
      WaylandClientTest::SetUIMessageLoop(base::MessageLoopCurrent::Get());
    }
  }

  void Shutdown() override {
    if (run_with_external_wayland_server_) {
      scoped_task_environment_ = nullptr;
      base::TestSuite::Shutdown();
    } else {
      WaylandClientTest::SetUIMessageLoop(nullptr);
      scoped_task_environment_ = nullptr;
      ash::AshTestSuite::Shutdown();
    }
  }

 private:
  void RunTestsOnClientThread(const base::Closure& finished_closure) {
    result_ = RUN_ALL_TESTS();
    finished_closure.Run();
  }

  // Do not run the wayland server within the test.
  const bool run_with_external_wayland_server_ = false;

  std::unique_ptr<base::test::ScopedTaskEnvironment> scoped_task_environment_;

  // Result of RUN_ALL_TESTS().
  int result_ = 1;

  DISALLOW_COPY_AND_ASSIGN(ExoClientPerfTestSuite);
};

}  // namespace
}  // namespace exo

int main(int argc, char** argv) {
  mojo::edk::Init();

  exo::ExoClientPerfTestSuite test_suite(argc, argv);

  return base::LaunchUnitTestsSerially(
      argc, argv,
      base::Bind(&exo::ExoClientPerfTestSuite::Run,
                 base::Unretained(&test_suite)));
}
