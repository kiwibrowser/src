// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/public/cpp/service_runner.h"

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/process/launch.h"
#include "base/run_loop.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace service_manager {

int g_service_runner_argc;
const char* const* g_service_runner_argv;

ServiceRunner::ServiceRunner(Service* service)
    : service_(base::WrapUnique(service)),
      message_loop_type_(base::MessageLoop::TYPE_DEFAULT),
      has_run_(false) {}

ServiceRunner::~ServiceRunner() {}

void ServiceRunner::InitBaseCommandLine() {
  base::CommandLine::Init(g_service_runner_argc, g_service_runner_argv);
}

void ServiceRunner::set_message_loop_type(base::MessageLoop::Type type) {
  DCHECK_NE(base::MessageLoop::TYPE_CUSTOM, type);
  DCHECK(!has_run_);

  message_loop_type_ = type;
}

MojoResult ServiceRunner::Run(MojoHandle service_request_handle,
                              bool init_base) {
  DCHECK(!has_run_);
  has_run_ = true;

  std::unique_ptr<base::AtExitManager> at_exit;
  if (init_base) {
    InitBaseCommandLine();
    at_exit.reset(new base::AtExitManager);
  }

  {
    std::unique_ptr<base::MessageLoop> loop;
    loop.reset(new base::MessageLoop(message_loop_type_));

    context_.reset(new ServiceContext(
        std::move(service_),
        mojom::ServiceRequest(mojo::MakeScopedHandle(
            mojo::MessagePipeHandle(service_request_handle)))));
    base::RunLoop run_loop;
    context_->SetQuitClosure(run_loop.QuitClosure());
    run_loop.Run();
    context_.reset();
  }
  return MOJO_RESULT_OK;
}

MojoResult ServiceRunner::Run(MojoHandle service_request_handle) {
  return Run(service_request_handle, false);
}

void ServiceRunner::Quit() {
  base::RunLoop::QuitCurrentWhenIdleDeprecated();
}

}  // namespace service_manager
