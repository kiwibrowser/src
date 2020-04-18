// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/layout_test/layout_test_url_request_context_getter.h"

#include <utility>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/shell/browser/shell_network_delegate.h"
#include "content/shell/common/layout_test/layout_test_switches.h"
#include "net/cert/cert_verifier.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "services/network/ignore_errors_cert_verifier.h"

namespace content {

LayoutTestURLRequestContextGetter::LayoutTestURLRequestContextGetter(
    bool ignore_certificate_errors,
    bool off_the_record,
    const base::FilePath& base_path,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    ProtocolHandlerMap* protocol_handlers,
    URLRequestInterceptorScopedVector request_interceptors,
    net::NetLog* net_log)
    : ShellURLRequestContextGetter(ignore_certificate_errors,
                                   off_the_record,
                                   base_path,
                                   std::move(io_task_runner),
                                   protocol_handlers,
                                   std::move(request_interceptors),
                                   net_log) {
  // Must first be created on the UI thread.
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

LayoutTestURLRequestContextGetter::~LayoutTestURLRequestContextGetter() {
}

std::unique_ptr<net::NetworkDelegate>
LayoutTestURLRequestContextGetter::CreateNetworkDelegate() {
  ShellNetworkDelegate::SetBlockThirdPartyCookies(true);
  return base::WrapUnique(new ShellNetworkDelegate);
}

std::unique_ptr<net::CertVerifier>
LayoutTestURLRequestContextGetter::GetCertVerifier() {
  return network::IgnoreErrorsCertVerifier::MaybeWrapCertVerifier(
      *base::CommandLine::ForCurrentProcess(), switches::kRunWebTests,
      net::CertVerifier::CreateDefault());
}

std::unique_ptr<net::ProxyConfigService>
LayoutTestURLRequestContextGetter::GetProxyConfigService() {
  return nullptr;
}

std::unique_ptr<net::ProxyResolutionService>
LayoutTestURLRequestContextGetter::GetProxyService() {
  return net::ProxyResolutionService::CreateDirect();
}

}  // namespace content
