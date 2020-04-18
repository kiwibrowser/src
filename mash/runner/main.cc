// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>

#include "base/at_exit.h"
#include "base/base_paths.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/debug/debugger.h"
#include "base/debug/stack_trace.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/i18n/icu_util.h"
#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"
#include "build/build_config.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "services/service_manager/runner/init.h"
#include "services/service_manager/standalone/context.h"
#include "services/service_manager/switches.h"

namespace {

const base::FilePath::CharType kMashCatalogFilename[] =
    FILE_PATH_LITERAL("mash_catalog.json");

}  // namespace

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);

  base::AtExitManager at_exit;
  service_manager::InitializeLogging();
  service_manager::WaitForDebuggerIfNecessary();

#if !defined(OFFICIAL_BUILD) && defined(OS_WIN)
  base::RouteStdioToConsole(false);
#endif

#if !defined(OFFICIAL_BUILD)
  base::debug::EnableInProcessStackDumping();
#endif
  base::PlatformThread::SetName("service_manager");

  std::string catalog_contents;
  base::FilePath exe_path;
  base::PathService::Get(base::DIR_EXE, &exe_path);
  base::FilePath catalog_path = exe_path.Append(kMashCatalogFilename);
  bool result = base::ReadFileToString(catalog_path, &catalog_contents);
  DCHECK(result);
  std::unique_ptr<base::Value> manifest_value =
      base::JSONReader::Read(catalog_contents);
  DCHECK(manifest_value);

#if defined(OS_WIN) && defined(COMPONENT_BUILD)
  // In Windows component builds, ensure that loaded service binaries always
  // search this exe's dir for DLLs.
  SetDllDirectory(exe_path.value().c_str());
#endif

  base::i18n::InitializeICU();

  mojo::edk::Init();

  base::Thread ipc_thread("IPC thread");
  ipc_thread.StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));

  // We can use fast IPC shutdown here since service manager termination must
  // effectively bring down all services as well.
  mojo::edk::ScopedIPCSupport ipc_support(
      ipc_thread.task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::FAST);

  base::test::ScopedTaskEnvironment scoped_task_environment;
  service_manager::Context service_manager_context(nullptr,
                                                   std::move(manifest_value));
  scoped_task_environment.GetMainThreadTaskRunner()->PostTask(
      FROM_HERE,
      base::Bind(&service_manager::Context::RunCommandLineApplication,
                 base::Unretained(&service_manager_context)));
  base::RunLoop().Run();
  return 0;
}
