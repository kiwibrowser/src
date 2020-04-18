// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/standalone/context.h"

#include <stddef.h>
#include <stdint.h>

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/json/json_file_value_serializer.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/process/process_info.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "services/catalog/catalog.h"
#include "services/service_manager/connect_params.h"
#include "services/service_manager/connect_util.h"
#include "services/service_manager/runner/common/switches.h"
#include "services/service_manager/runner/host/service_process_launcher_factory.h"
#include "services/service_manager/service_manager.h"
#include "services/service_manager/switches.h"

#if !defined(OS_IOS)
#include "services/service_manager/runner/host/service_process_launcher.h"
#endif

#if defined(OS_MACOSX)
#include "services/service_manager/public/cpp/standalone_service/mach_broker.h"
#endif

namespace service_manager {
namespace {

#if !defined(OS_IOS)
// Used to ensure we only init once.
class ServiceProcessLauncherFactoryImpl : public ServiceProcessLauncherFactory {
 public:
  ServiceProcessLauncherFactoryImpl(ServiceProcessLauncherDelegate* delegate)
      : delegate_(delegate) {}

 private:
   std::unique_ptr<ServiceProcessLauncher> Create(
      const base::FilePath& service_path) override {
     return std::make_unique<ServiceProcessLauncher>(delegate_, service_path);
  }

  ServiceProcessLauncherDelegate* delegate_;
};
#endif  // !defined(OS_IOS)

void OnInstanceQuit(const std::string& name, const Identity& identity) {
  if (name == identity.name())
    base::RunLoop::QuitCurrentWhenIdleDeprecated();
}

const char kService[] = "service";

}  // namespace

Context::Context(
    ServiceProcessLauncherDelegate* service_process_launcher_delegate,
    std::unique_ptr<base::Value> catalog_contents)
    : main_entry_time_(base::Time::Now()) {
  TRACE_EVENT0("service_manager", "Context::Context");

  std::unique_ptr<ServiceProcessLauncherFactory>
      service_process_launcher_factory;

// iOS does not support launching services in their own processes (and does
// not build ServiceProcessLauncher).
#if !defined(OS_IOS)
  service_process_launcher_factory =
      std::make_unique<ServiceProcessLauncherFactoryImpl>(
          service_process_launcher_delegate);
#endif
  service_manager_.reset(
      new ServiceManager(std::move(service_process_launcher_factory),
                         std::move(catalog_contents), nullptr));
}

Context::~Context() = default;

void Context::RunCommandLineApplication() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(kService))
    Run(command_line->GetSwitchValueASCII(kService));
}

void Context::Run(const std::string& name) {
  service_manager_->SetInstanceQuitCallback(base::Bind(&OnInstanceQuit, name));

  std::unique_ptr<ConnectParams> params(new ConnectParams);
  params->set_source(CreateServiceManagerIdentity());
  params->set_target(Identity(name, mojom::kRootUserID));
  service_manager_->Connect(std::move(params));
}

}  // namespace service_manager
