// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/cast_browser_process.h"

#include <utility>

#include "base/logging.h"
#include "build/build_config.h"
#include "chromecast/browser/cast_browser_context.h"
#include "chromecast/browser/devtools/remote_debugging_server.h"
#include "chromecast/browser/metrics/cast_metrics_service_client.h"
#include "chromecast/net/connectivity_checker.h"
#include "chromecast/service/cast_service.h"
#include "components/prefs/pref_service.h"

#if defined(USE_AURA)
#include "chromecast/graphics/cast_screen.h"
#endif  // defined(USE_AURA)

namespace chromecast {
namespace shell {

namespace {
CastBrowserProcess* g_instance = NULL;
}  // namespace

// static
CastBrowserProcess* CastBrowserProcess::GetInstance() {
  DCHECK(g_instance);
  return g_instance;
}

CastBrowserProcess::CastBrowserProcess()
    : cast_content_browser_client_(nullptr),
      net_log_(nullptr) {
  DCHECK(!g_instance);
  g_instance = this;
}

CastBrowserProcess::~CastBrowserProcess() {
  DCHECK_EQ(g_instance, this);
  if (pref_service_)
    pref_service_->CommitPendingWrite();
  g_instance = NULL;
}

void CastBrowserProcess::SetBrowserContext(
    std::unique_ptr<CastBrowserContext> browser_context) {
  DCHECK(!browser_context_);
  browser_context_.swap(browser_context);
}

void CastBrowserProcess::SetCastContentBrowserClient(
    CastContentBrowserClient* cast_content_browser_client) {
  DCHECK(!cast_content_browser_client_);
  cast_content_browser_client_ = cast_content_browser_client;
}

void CastBrowserProcess::SetCastService(
    std::unique_ptr<CastService> cast_service) {
  DCHECK(!cast_service_);
  cast_service_.swap(cast_service);
}

#if defined(USE_AURA)
void CastBrowserProcess::SetCastScreen(
    std::unique_ptr<CastScreen> cast_screen) {
  DCHECK(!cast_screen_);
  cast_screen_ = std::move(cast_screen);
}
#endif  // defined(USE_AURA)

void CastBrowserProcess::SetMetricsServiceClient(
    std::unique_ptr<metrics::CastMetricsServiceClient> metrics_service_client) {
  DCHECK(!metrics_service_client_);
  metrics_service_client_.swap(metrics_service_client);
}

void CastBrowserProcess::SetPrefService(
    std::unique_ptr<PrefService> pref_service) {
  DCHECK(!pref_service_);
  pref_service_.swap(pref_service);
}

void CastBrowserProcess::SetRemoteDebuggingServer(
    std::unique_ptr<RemoteDebuggingServer> remote_debugging_server) {
  DCHECK(!remote_debugging_server_);
  remote_debugging_server_.swap(remote_debugging_server);
}

void CastBrowserProcess::SetConnectivityChecker(
    scoped_refptr<ConnectivityChecker> connectivity_checker) {
  DCHECK(!connectivity_checker_);
  connectivity_checker_.swap(connectivity_checker);
}

void CastBrowserProcess::SetNetLog(net::NetLog* net_log) {
  DCHECK(!net_log_);
  net_log_ = net_log;
}

}  // namespace shell
}  // namespace chromecast
