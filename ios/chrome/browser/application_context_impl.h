// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_APPLICATION_CONTEXT_IMPL_H_
#define IOS_CHROME_BROWSER_APPLICATION_CONTEXT_IMPL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/web/public/network_context_owner.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace base {
class CommandLine;
class SequencedTaskRunner;
}

class ApplicationContextImpl : public ApplicationContext {
 public:
  ApplicationContextImpl(base::SequencedTaskRunner* local_state_task_runner,
                         const base::CommandLine& command_line,
                         const std::string& locale);
  ~ApplicationContextImpl() override;

  // Called before the browser threads are created.
  void PreCreateThreads();

  // Called after the threads have been created but before the message loops
  // starts running. Allows the ApplicationContext to do any initialization
  // that requres all threads running.
  void PreMainMessageLoopRun();

  // Most cleanup is done by these functions, driven from IOSChromeMainParts
  // rather than in the destructor, so that we can interleave cleanup with
  // threads being stopped.
  void StartTearDown();
  void PostDestroyThreads();

  // ApplicationContext implementation.
  void OnAppEnterForeground() override;
  void OnAppEnterBackground() override;
  bool WasLastShutdownClean() override;
  PrefService* GetLocalState() override;
  net::URLRequestContextGetter* GetSystemURLRequestContext() override;
  scoped_refptr<network::SharedURLLoaderFactory> GetSharedURLLoaderFactory()
      override;
  network::mojom::NetworkContext* GetSystemNetworkContext() override;
  const std::string& GetApplicationLocale() override;
  ios::ChromeBrowserStateManager* GetChromeBrowserStateManager() override;
  metrics_services_manager::MetricsServicesManager* GetMetricsServicesManager()
      override;
  metrics::MetricsService* GetMetricsService() override;
  ukm::UkmRecorder* GetUkmRecorder() override;
  variations::VariationsService* GetVariationsService() override;
  rappor::RapporServiceImpl* GetRapporServiceImpl() override;
  net_log::ChromeNetLog* GetNetLog() override;
  network_time::NetworkTimeTracker* GetNetworkTimeTracker() override;
  IOSChromeIOThread* GetIOSChromeIOThread() override;
  gcm::GCMDriver* GetGCMDriver() override;
  component_updater::ComponentUpdateService* GetComponentUpdateService()
      override;

 private:
  // Sets the locale used by the application.
  void SetApplicationLocale(const std::string& locale);

  // Create the local state.
  void CreateLocalState();

  // Create the gcm driver.
  void CreateGCMDriver();

  base::ThreadChecker thread_checker_;
  std::unique_ptr<PrefService> local_state_;
  std::unique_ptr<net_log::ChromeNetLog> net_log_;
  std::unique_ptr<network_time::NetworkTimeTracker> network_time_tracker_;
  std::unique_ptr<IOSChromeIOThread> ios_chrome_io_thread_;
  std::unique_ptr<metrics_services_manager::MetricsServicesManager>
      metrics_services_manager_;
  std::unique_ptr<gcm::GCMDriver> gcm_driver_;
  std::unique_ptr<component_updater::ComponentUpdateService> component_updater_;
  std::unique_ptr<ios::ChromeBrowserStateManager> chrome_browser_state_manager_;
  std::string application_locale_;

  // Sequenced task runner for local state related I/O tasks.
  const scoped_refptr<base::SequencedTaskRunner> local_state_task_runner_;

  network::mojom::NetworkContextPtr network_context_;
  network::mojom::URLLoaderFactoryPtr url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  // Created on the UI thread, destroyed on the IO thread.
  std::unique_ptr<web::NetworkContextOwner> network_context_owner_;

  bool was_last_shutdown_clean_;

  DISALLOW_COPY_AND_ASSIGN(ApplicationContextImpl);
};

#endif  // IOS_CHROME_BROWSER_APPLICATION_CONTEXT_IMPL_H_
