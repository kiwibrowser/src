// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_APP_CONTENT_SERVICE_MANAGER_MAIN_DELEGATE_H_
#define CONTENT_APP_CONTENT_SERVICE_MANAGER_MAIN_DELEGATE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "content/public/app/content_main.h"
#include "services/service_manager/embedder/main_delegate.h"

namespace content {

class ContentMainRunnerImpl;

class ContentServiceManagerMainDelegate : public service_manager::MainDelegate {
 public:
  explicit ContentServiceManagerMainDelegate(const ContentMainParams& params);
  ~ContentServiceManagerMainDelegate() override;

  // service_manager::MainDelegate:
  int Initialize(const InitializeParams& params) override;
#if !defined(CHROME_MULTIPLE_DLL_CHILD)
  scoped_refptr<base::SingleThreadTaskRunner>
  GetServiceManagerTaskRunnerForEmbedderProcess() override;
#endif  // !defined(CHROME_MULTIPLE_DLL_CHILD)
  bool IsEmbedderSubprocess() override;
  int RunEmbedderProcess() override;
  void ShutDownEmbedderProcess() override;
  service_manager::ProcessType OverrideProcessType() override;
  void OverrideMojoConfiguration(mojo::edk::Configuration* config) override;
  std::unique_ptr<base::Value> CreateServiceCatalog() override;
  bool ShouldLaunchAsServiceProcess(
      const service_manager::Identity& identity) override;
  void AdjustServiceProcessCommandLine(
      const service_manager::Identity& identity,
      base::CommandLine* command_line) override;
  void OnServiceManagerInitialized(
      const base::Closure& quit_closure,
      service_manager::BackgroundServiceManager* service_manager) override;
  std::unique_ptr<service_manager::Service> CreateEmbeddedService(
      const std::string& service_name) override;

 private:
  ContentMainParams content_main_params_;
  std::unique_ptr<ContentMainRunnerImpl> content_main_runner_;

#if defined(OS_ANDROID)
  bool initialized_ = false;
#endif

  DISALLOW_COPY_AND_ASSIGN(ContentServiceManagerMainDelegate);
};

}  // namespace content

#endif  // CONTENT_APP_CONTENT_SERVICE_MANAGER_MAIN_DELEGATE_H_
