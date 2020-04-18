// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_EMBEDDER_MAIN_DELEGATE_H_
#define SERVICES_SERVICE_MANAGER_EMBEDDER_MAIN_DELEGATE_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "mojo/edk/embedder/configuration.h"
#include "services/service_manager/background/background_service_manager.h"
#include "services/service_manager/embedder/process_type.h"
#include "services/service_manager/embedder/service_manager_embedder_export.h"
#include "services/service_manager/public/cpp/identity.h"
#include "services/service_manager/public/cpp/service.h"

namespace base {
class CommandLine;
class Value;
namespace mac {
class ScopedNSAutoreleasePool;
}
}

namespace service_manager {

// An interface which must be implemented by Service Manager embedders to
// control basic process initialization and shutdown, as well as early branching
// to run specific types of subprocesses.
class SERVICE_MANAGER_EMBEDDER_EXPORT MainDelegate {
 public:
  // Extra parameters passed to MainDelegate::Initialize.
  struct InitializeParams {
#if defined(OS_MACOSX)
    // The outermost autorelease pool, allocated by internal service manager
    // logic. This is guaranteed to live throughout the extent of Run().
    base::mac::ScopedNSAutoreleasePool* autorelease_pool = nullptr;
#endif
  };

  MainDelegate();
  virtual ~MainDelegate();

  // Perform early process initialization. Returns -1 if successful, or the exit
  // code with which the process should be terminated due to initialization
  // failure.
  virtual int Initialize(const InitializeParams& params) = 0;

  // Creates an thread and returns the SingleThreadTaskRunner on which
  // ServiceManager should run.
  virtual scoped_refptr<base::SingleThreadTaskRunner>
  GetServiceManagerTaskRunnerForEmbedderProcess();

  // Indicates whether this (embedder) process should be treated as a subprocess
  // for the sake of some platform-specific environment initialization details.
  virtual bool IsEmbedderSubprocess();

  // Runs the embedder's own main process logic. Called exactly once after a
  // successful call to Initialize(), and only if the Service Manager core does
  // not know what to do otherwise -- i.e., if it is not starting a new Service
  // Manager instance or launching an embedded service.
  //
  // Returns the exit code to use when terminating the process after
  // RunEmbedderProcess() (and then ShutDown()) completes.
  virtual int RunEmbedderProcess();

  // Called just before process exit if RunEmbedderProcess() was called.
  virtual void ShutDownEmbedderProcess();

  // Force execution of the current process as a specific process type. May
  // return |ProcessType::kDefault| to avoid overriding.
  virtual ProcessType OverrideProcessType();

  // Allows the embedder to override the process-wide Mojop configuration.
  virtual void OverrideMojoConfiguration(mojo::edk::Configuration* config);

  // Create the service catalog to be used by the Service Manager. May return
  // null to use the default (empty) catalog, if you're into that.
  virtual std::unique_ptr<base::Value> CreateServiceCatalog();

  // Indicates whether a process started by the service manager for a given
  // target service identity should be run as a real service process (|true|)
  // or if the service manager should delegate to the embedder to initialize the
  // new process (|false|).
  virtual bool ShouldLaunchAsServiceProcess(const Identity& identity);

  // Allows the embedder to override command line switches for a service process
  // to be launched.
  virtual void AdjustServiceProcessCommandLine(const Identity& identity,
                                               base::CommandLine* command_line);

  // Allows the embedder to perform arbitrary initialization within the Service
  // Manager process immediately before the Service Manager runs its main loop.
  //
  // |quit_closure| is a callback the embedder may retain and invoke at any time
  // to cleanly terminate Service Manager execution.
  virtual void OnServiceManagerInitialized(
      const base::Closure& quit_closure,
      BackgroundServiceManager* service_manager);

  // Runs an embedded service by name. If the embedder does not know how to
  // create an instance of the named service, it should return null.
  virtual std::unique_ptr<Service> CreateEmbeddedService(
      const std::string& service_name);
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_EMBEDDER_MAIN_DELEGATE_H_
