// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/local_discovery/service_discovery_shared_client.h"

#include <memory>

#include "build/build_config.h"
#include "net/net_buildflags.h"

#if defined(OS_WIN)
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/metrics/histogram_macros.h"
#include "base/path_service.h"
#include "base/task_scheduler/post_task.h"
#include "base/timer/elapsed_timer.h"
#include "chrome/installer/util/browser_distribution.h"
#include "chrome/installer/util/firewall_manager_win.h"
#endif

#if defined(OS_MACOSX)
#include "chrome/browser/local_discovery/service_discovery_client_mac_factory.h"
#endif

#if BUILDFLAG(ENABLE_MDNS)
#include "base/memory/ref_counted.h"
#include "chrome/browser/local_discovery/service_discovery_client_mdns.h"
#endif

namespace local_discovery {

using content::BrowserThread;

namespace {

#if defined(OS_WIN)
void ReportFirewallStats() {
  base::FilePath exe_path;
  if (!base::PathService::Get(base::FILE_EXE, &exe_path))
    return;
  base::ElapsedTimer timer;
  std::unique_ptr<installer::FirewallManager> manager =
      installer::FirewallManager::Create(BrowserDistribution::GetDistribution(),
                                         exe_path);
  if (!manager)
    return;
  bool is_firewall_ready = manager->CanUseLocalPorts();
  UMA_HISTOGRAM_TIMES("LocalDiscovery.FirewallAccessTime", timer.Elapsed());
  UMA_HISTOGRAM_BOOLEAN("LocalDiscovery.IsFirewallReady", is_firewall_ready);
}
#endif  // defined(OS_WIN)

ServiceDiscoverySharedClient* g_service_discovery_client = nullptr;

}  // namespace

ServiceDiscoverySharedClient::ServiceDiscoverySharedClient() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!g_service_discovery_client);
  g_service_discovery_client = this;
}

ServiceDiscoverySharedClient::~ServiceDiscoverySharedClient() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_EQ(g_service_discovery_client, this);
  g_service_discovery_client = nullptr;
}

// static
scoped_refptr<ServiceDiscoverySharedClient>
    ServiceDiscoverySharedClient::GetInstance() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
#if BUILDFLAG(ENABLE_MDNS) || defined(OS_MACOSX)
  if (g_service_discovery_client)
    return g_service_discovery_client;

#if defined(OS_WIN)
  static bool is_firewall_state_reported = false;
  if (!is_firewall_state_reported) {
    is_firewall_state_reported = true;
    auto task_runner = base::CreateCOMSTATaskRunnerWithTraits(
        {base::TaskPriority::BACKGROUND, base::MayBlock()});
    task_runner->PostTask(FROM_HERE, base::BindOnce(&ReportFirewallStats));
  }
#endif  // defined(OS_WIN)

#if defined(OS_MACOSX)
  return ServiceDiscoveryClientMacFactory::CreateInstance();
#else
  return base::MakeRefCounted<ServiceDiscoveryClientMdns>();
#endif  // defined(OS_MACOSX)
#else
  NOTIMPLEMENTED();
  return nullptr;
#endif  // BUILDFLAG(ENABLE_MDNS) || defined(OS_MACOSX)
}

}  // namespace local_discovery
