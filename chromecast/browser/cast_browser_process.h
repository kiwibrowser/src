// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_CAST_BROWSER_PROCESS_H_
#define CHROMECAST_BROWSER_CAST_BROWSER_PROCESS_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"

class PrefService;

namespace net {
class NetLog;
}  // namespace net

namespace chromecast {
class CastService;
class CastScreen;
class ConnectivityChecker;

namespace metrics {
class CastMetricsServiceClient;
}  // namespace metrics

namespace shell {
class CastBrowserContext;
class CastContentBrowserClient;
class RemoteDebuggingServer;

class CastBrowserProcess {
 public:
  // Gets the global instance of CastBrowserProcess. Does not create lazily and
  // assumes the instance already exists.
  static CastBrowserProcess* GetInstance();

  CastBrowserProcess();
  virtual ~CastBrowserProcess();

  void SetBrowserContext(std::unique_ptr<CastBrowserContext> browser_context);
  void SetCastContentBrowserClient(CastContentBrowserClient* browser_client);
  void SetCastService(std::unique_ptr<CastService> cast_service);
#if defined(USE_AURA)
  void SetCastScreen(std::unique_ptr<CastScreen> cast_screen);
#endif  // defined(USE_AURA)
  void SetMetricsServiceClient(
      std::unique_ptr<metrics::CastMetricsServiceClient>
          metrics_service_client);
  void SetPrefService(std::unique_ptr<PrefService> pref_service);
  void SetRemoteDebuggingServer(
      std::unique_ptr<RemoteDebuggingServer> remote_debugging_server);
  void SetConnectivityChecker(
      scoped_refptr<ConnectivityChecker> connectivity_checker);
  void SetNetLog(net::NetLog* net_log);

  CastContentBrowserClient* browser_client() const {
    return cast_content_browser_client_;
  }
  CastBrowserContext* browser_context() const { return browser_context_.get(); }
  CastService* cast_service() const { return cast_service_.get(); }
#if defined(USE_AURA)
  CastScreen* cast_screen() const { return cast_screen_.get(); }
#endif  // defined(USE_AURA)
  metrics::CastMetricsServiceClient* metrics_service_client() const {
    return metrics_service_client_.get();
  }
  PrefService* pref_service() const { return pref_service_.get(); }
  ConnectivityChecker* connectivity_checker() const {
    return connectivity_checker_.get();
  }
  RemoteDebuggingServer* remote_debugging_server() const {
    return remote_debugging_server_.get();
  }
  net::NetLog* net_log() const { return net_log_; }

 private:
  // Note: The following order should match the order they are set in
  // CastBrowserMainParts.
#if defined(USE_AURA)
  std::unique_ptr<CastScreen> cast_screen_;
#endif  // defined(USE_AURA)
  std::unique_ptr<PrefService> pref_service_;
  scoped_refptr<ConnectivityChecker> connectivity_checker_;
  std::unique_ptr<CastBrowserContext> browser_context_;
  std::unique_ptr<metrics::CastMetricsServiceClient> metrics_service_client_;
  std::unique_ptr<RemoteDebuggingServer> remote_debugging_server_;

  CastContentBrowserClient* cast_content_browser_client_;
  net::NetLog* net_log_;

  // Note: CastService must be destroyed before others.
  std::unique_ptr<CastService> cast_service_;

  DISALLOW_COPY_AND_ASSIGN(CastBrowserProcess);
};

}  // namespace shell
}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_CAST_BROWSER_PROCESS_H_
