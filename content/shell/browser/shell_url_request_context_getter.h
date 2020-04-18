// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_SHELL_URL_REQUEST_CONTEXT_GETTER_H_
#define CONTENT_SHELL_BROWSER_SHELL_URL_REQUEST_CONTEXT_GETTER_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_interceptor.h"
#include "net/url_request/url_request_job_factory.h"

namespace net {
class CertVerifier;
class HostResolver;
class NetLog;
class NetworkDelegate;
class ProxyConfigService;
class ProxyResolutionService;
class URLRequestContext;
}

namespace content {

class ShellURLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  ShellURLRequestContextGetter(
      bool ignore_certificate_errors,
      bool off_the_record,
      const base::FilePath& base_path,
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
      ProtocolHandlerMap* protocol_handlers,
      URLRequestInterceptorScopedVector request_interceptors,
      net::NetLog* net_log);

  // net::URLRequestContextGetter implementation.
  net::URLRequestContext* GetURLRequestContext() override;
  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override;

  net::HostResolver* host_resolver();

  void NotifyContextShuttingDown();

 protected:
  ~ShellURLRequestContextGetter() override;

  // Used by subclasses to create their own implementation of NetworkDelegate
  // and net::ProxyResolutionService.
  virtual std::unique_ptr<net::NetworkDelegate> CreateNetworkDelegate();
  virtual std::unique_ptr<net::CertVerifier> GetCertVerifier();
  // GetProxyConfigService() and GetProxyService() are mutually exclusive.
  // if the subclass returns something in GetProxyService(), the return
  // ProxyConfigService will not be used. Called on the UI thread, unlike other
  // virtual methods.
  virtual std::unique_ptr<net::ProxyConfigService> GetProxyConfigService();
  // If this returns nullptr, the URLRequestContextBuilder will create the
  // service.
  virtual std::unique_ptr<net::ProxyResolutionService> GetProxyService();

 private:
  bool ignore_certificate_errors_;
  bool off_the_record_;
  bool shut_down_;
  base::FilePath base_path_;
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  net::NetLog* net_log_;

  std::unique_ptr<net::ProxyConfigService> proxy_config_service_;
  std::unique_ptr<net::URLRequestContext> url_request_context_;
  ProtocolHandlerMap protocol_handlers_;
  URLRequestInterceptorScopedVector request_interceptors_;

  DISALLOW_COPY_AND_ASSIGN(ShellURLRequestContextGetter);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_SHELL_URL_REQUEST_CONTEXT_GETTER_H_
