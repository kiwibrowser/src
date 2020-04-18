// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/cronet_global_state.h"

#include "base/at_exit.h"
#include "base/feature_list.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_scheduler.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "url/url_util.h"

// This file provides minimal "stub" implementations of the Cronet global-state
// functions for the native library build, sufficient to have cronet_tests and
// cronet_unittests build.

namespace cronet {

namespace {

scoped_refptr<base::SingleThreadTaskRunner> InitializeAndCreateTaskRunner() {
// TODO(https://crbug.com/816705): Component builds result in //base and other
// process-global state being shared between the library and the test suite.
// Since we only expect Cronet native library component build to be used to run
// cronet_tests, we can assume that suite will define some things, for now.
#if !defined(COMPONENT_BUILD)
  // TODO(wez): Remove this once AtExitManager dependencies are gone.
  ignore_result(new base::AtExitManager);
#endif

  base::FeatureList::InitializeInstance(std::string(), std::string());

  url::Initialize();

  // Note that in component builds this TaskScheduler will be shared with the
  // calling process, if it also depends on //base. In particular this means
  // that the Cronet test binaries must avoid initializing or shutting-down the
  // TaskScheduler themselves.
  base::TaskScheduler::CreateAndStartWithDefaultParams("cronet");

  return base::CreateSingleThreadTaskRunnerWithTraits({});
}

base::SingleThreadTaskRunner* InitTaskRunner() {
  static scoped_refptr<base::SingleThreadTaskRunner> init_task_runner =
      InitializeAndCreateTaskRunner();
  return init_task_runner.get();
}

}  // namespace

void EnsureInitialized() {
  ignore_result(InitTaskRunner());
}

bool OnInitThread() {
  return InitTaskRunner()->BelongsToCurrentThread();
}

void PostTaskToInitThread(const base::Location& posted_from,
                          base::OnceClosure task) {
  InitTaskRunner()->PostTask(posted_from, std::move(task));
}

std::unique_ptr<net::ProxyConfigService> CreateProxyConfigService(
    const scoped_refptr<base::SequencedTaskRunner>& io_task_runner) {
  return net::ProxyResolutionService::CreateSystemProxyConfigService(
      io_task_runner);
}

std::unique_ptr<net::ProxyResolutionService> CreateProxyResolutionService(
    std::unique_ptr<net::ProxyConfigService> proxy_config_service,
    net::NetLog* net_log) {
  return net::ProxyResolutionService::CreateUsingSystemProxyResolver(
      std::move(proxy_config_service), net_log);
}

std::string CreateDefaultUserAgent(const std::string& partial_user_agent) {
  return partial_user_agent;
}

}  // namespace cronet
