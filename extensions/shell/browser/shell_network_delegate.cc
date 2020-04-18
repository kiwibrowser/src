// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/shell_network_delegate.h"

#include <map>

#include "base/lazy_instance.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/browser/api/web_request/web_request_api.h"
#include "extensions/browser/api/web_request/web_request_info.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/process_manager.h"
#include "net/url_request/url_request.h"

namespace extensions {

namespace {

bool g_accept_all_cookies = true;

using RequestMap = std::map<net::URLRequest*, std::unique_ptr<WebRequestInfo>>;
base::LazyInstance<RequestMap>::Leaky g_requests = LAZY_INSTANCE_INITIALIZER;

// Returns the corresponding WebRequestInfo object for the request, creating it
// if necessary.
WebRequestInfo* GetWebRequestInfo(net::URLRequest* request) {
  auto it = g_requests.Get().find(request);
  if (it == g_requests.Get().end()) {
    it = g_requests.Get()
             .emplace(request, std::make_unique<WebRequestInfo>(request))
             .first;
  }
  return it->second.get();
}

}  // namespace

ShellNetworkDelegate::ShellNetworkDelegate(
      void* browser_context, InfoMap* extension_info_map) {
  browser_context_ = browser_context;
  extension_info_map_ = extension_info_map;
}

ShellNetworkDelegate::~ShellNetworkDelegate() {}

void ShellNetworkDelegate::SetAcceptAllCookies(bool accept) {
  g_accept_all_cookies = accept;
}

int ShellNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {
  WebRequestInfo* web_request_info = GetWebRequestInfo(request);
  return ExtensionWebRequestEventRouter::GetInstance()->OnBeforeRequest(
      browser_context_, extension_info_map_.get(), web_request_info, callback,
      new_url);
}

int ShellNetworkDelegate::OnBeforeStartTransaction(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers) {
  return ExtensionWebRequestEventRouter::GetInstance()->OnBeforeSendHeaders(
      browser_context_, extension_info_map_.get(), GetWebRequestInfo(request),
      callback, headers);
}

void ShellNetworkDelegate::OnStartTransaction(
    net::URLRequest* request,
    const net::HttpRequestHeaders& headers) {
  ExtensionWebRequestEventRouter::GetInstance()->OnSendHeaders(
      browser_context_, extension_info_map_.get(), GetWebRequestInfo(request),
      headers);
}

int ShellNetworkDelegate::OnHeadersReceived(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  return ExtensionWebRequestEventRouter::GetInstance()->OnHeadersReceived(
      browser_context_, extension_info_map_.get(), GetWebRequestInfo(request),
      callback, original_response_headers, override_response_headers,
      allowed_unsafe_redirect_url);
}

void ShellNetworkDelegate::OnBeforeRedirect(
    net::URLRequest* request,
    const GURL& new_location) {
  auto* info = GetWebRequestInfo(request);
  info->AddResponseInfoFromURLRequest(request);
  ExtensionWebRequestEventRouter::GetInstance()->OnBeforeRedirect(
      browser_context_, extension_info_map_.get(), info, new_location);
}

void ShellNetworkDelegate::OnResponseStarted(net::URLRequest* request,
                                             int net_error) {
  auto* info = GetWebRequestInfo(request);
  info->AddResponseInfoFromURLRequest(request);
  ExtensionWebRequestEventRouter::GetInstance()->OnResponseStarted(
      browser_context_, extension_info_map_.get(), info, net_error);
}

void ShellNetworkDelegate::OnCompleted(net::URLRequest* request,
                                       bool started,
                                       int net_error) {
  DCHECK_NE(net::ERR_IO_PENDING, net_error);

  if (net_error == net::OK) {
    bool is_redirect = request->response_headers() &&
        net::HttpResponseHeaders::IsRedirectResponseCode(
            request->response_headers()->response_code());
    if (!is_redirect) {
      ExtensionWebRequestEventRouter::GetInstance()->OnCompleted(
          browser_context_, extension_info_map_.get(),
          GetWebRequestInfo(request), net_error);
    }
  } else {
    ExtensionWebRequestEventRouter::GetInstance()->OnErrorOccurred(
        browser_context_, extension_info_map_.get(), GetWebRequestInfo(request),
        started, net_error);
  }
}

void ShellNetworkDelegate::OnURLRequestDestroyed(
    net::URLRequest* request) {
  auto it = g_requests.Get().find(request);
  DCHECK(it != g_requests.Get().end());
  ExtensionWebRequestEventRouter::GetInstance()->OnRequestWillBeDestroyed(
      browser_context_, it->second.get());
  g_requests.Get().erase(it);
}

void ShellNetworkDelegate::OnPACScriptError(
    int line_number,
    const base::string16& error) {
}

net::NetworkDelegate::AuthRequiredResponse
ShellNetworkDelegate::OnAuthRequired(
    net::URLRequest* request,
    const net::AuthChallengeInfo& auth_info,
    const AuthCallback& callback,
    net::AuthCredentials* credentials) {
  auto* info = GetWebRequestInfo(request);
  info->AddResponseInfoFromURLRequest(request);
  return ExtensionWebRequestEventRouter::GetInstance()->OnAuthRequired(
      browser_context_, extension_info_map_.get(), info, auth_info, callback,
      credentials);
}

}  // namespace extensions
