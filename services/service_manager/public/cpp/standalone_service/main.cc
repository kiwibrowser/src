// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/public/c/main.h"
#include "base/at_exit.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/debug/stack_trace.h"
#include "base/feature_list.h"
#include "base/i18n/icu_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/process/launch.h"
#include "base/task_scheduler/task_scheduler.h"
#include "build/build_config.h"
#include "services/service_manager/public/cpp/standalone_service/standalone_service.h"
#include "services/service_manager/public/cpp/standalone_service/switches.h"
#include "services/service_manager/public/mojom/service.mojom.h"
#include "services/service_manager/runner/init.h"

#if defined(OS_MACOSX)
#include "base/mac/bundle_locations.h"
#endif

namespace {

// TODO(rockot): We should consider removing ServiceMain and instead allowing
// service sources to define a CreateService method which returns a new instance
// of service_manager::Service. This would reduce boilerplate in service sources
// since they all effectively do the same thing via
// service_manager::ServiceRunner.
void RunServiceMain(service_manager::mojom::ServiceRequest request) {
  MojoResult result = ServiceMain(request.PassMessagePipe().release().value());
  DCHECK_EQ(result, MOJO_RESULT_OK);
}

}  // namespace

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  base::CommandLine::Init(argc, argv);

#if !defined(OFFICIAL_BUILD) && defined(OS_WIN)
  base::RouteStdioToConsole(false);
#endif

  service_manager::InitializeLogging();

  base::i18n::InitializeICU();

#if !defined(OFFICIAL_BUILD)
  // Initialize stack dumping before initializing sandbox to make sure symbol
  // names in all loaded libraries will be cached.
  base::debug::EnableInProcessStackDumping();
#endif

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  base::FeatureList::InitializeInstance(
      command_line->GetSwitchValueASCII(switches::kEnableFeatures),
      command_line->GetSwitchValueASCII(switches::kDisableFeatures));

  service_manager::WaitForDebuggerIfNecessary();
  service_manager::RunStandaloneService(base::Bind(&RunServiceMain));

  base::TaskScheduler::GetInstance()->Shutdown();

  return 0;
}
