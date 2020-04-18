// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/shell_url_request_context_getter.h"

#include <utility>

#include "content/public/browser/resource_request_info.h"
#include "extensions/browser/info_map.h"
#include "extensions/shell/browser/shell_network_delegate.h"

namespace extensions {

ShellURLRequestContextGetter::ShellURLRequestContextGetter(
    content::BrowserContext* browser_context,
    bool ignore_certificate_errors,
    const base::FilePath& base_path,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors,
    net::NetLog* net_log,
    InfoMap* extension_info_map)
    : content::ShellURLRequestContextGetter(ignore_certificate_errors,
                                            false /* not incognito */,
                                            base_path,
                                            std::move(io_task_runner),
                                            protocol_handlers,
                                            std::move(request_interceptors),
                                            net_log),
      browser_context_(browser_context),
      extension_info_map_(extension_info_map) {}

ShellURLRequestContextGetter::~ShellURLRequestContextGetter() {
}

std::unique_ptr<net::NetworkDelegate>
ShellURLRequestContextGetter::CreateNetworkDelegate() {
  return std::make_unique<ShellNetworkDelegate>(browser_context_,
                                                extension_info_map_);
}

}  // namespace extensions
