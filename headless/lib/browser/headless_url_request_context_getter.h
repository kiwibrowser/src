// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_HEADLESS_URL_REQUEST_CONTEXT_GETTER_H_
#define HEADLESS_LIB_BROWSER_HEADLESS_URL_REQUEST_CONTEXT_GETTER_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "content/public/browser/browser_context.h"
#include "headless/public/headless_browser.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_job_factory.h"

namespace net {
class HostResolver;
class ProxyConfigService;
}

namespace headless {
class HeadlessBrowserContextOptions;
class HeadlessBrowserContextImpl;

class HeadlessURLRequestContextGetter
    : public net::URLRequestContextGetter,
      public HeadlessBrowserContext::Observer {
 public:
  HeadlessURLRequestContextGetter(
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
      content::ProtocolHandlerMap* protocol_handlers,
      ProtocolHandlerMap context_protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors,
      HeadlessBrowserContextOptions* options,
      net::NetLog* net_log,
      HeadlessBrowserContextImpl* headless_browser_context);

  // net::URLRequestContextGetter implementation:
  net::URLRequestContext* GetURLRequestContext() override;
  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override;

  net::HostResolver* host_resolver() const;

  // HeadlessBrowserContext::Observer implementation:
  void OnHeadlessBrowserContextDestruct() override;

  void NotifyContextShuttingDown();

 protected:
  ~HeadlessURLRequestContextGetter() override;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // The |options| object given to the constructor is not guaranteed to outlive
  // this class, so we make copies of the parts we need to access on the IO
  // thread.
  std::string accept_language_;
  std::string user_agent_;
  std::string host_resolver_rules_;
  const net::ProxyConfig* proxy_config_;  // Not owned.

  std::unique_ptr<net::ProxyConfigService> proxy_config_service_;
  std::unique_ptr<net::URLRequestContext> url_request_context_;
  content::ProtocolHandlerMap protocol_handlers_;
  content::URLRequestInterceptorScopedVector request_interceptors_;
  net::NetLog* net_log_;  // Not owned
  bool capture_resource_metadata_;

  base::Lock lock_;  // Protects |headless_browser_context_|.
  HeadlessBrowserContextImpl* headless_browser_context_;  // Not owned.

  bool shut_down_ = false;

  DISALLOW_COPY_AND_ASSIGN(HeadlessURLRequestContextGetter);
};

}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_HEADLESS_URL_REQUEST_CONTEXT_GETTER_H_
