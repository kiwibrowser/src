// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_cookie_access_policy.h"

#include <memory>

#include "android_webview/browser/aw_contents_io_thread_client.h"
#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/websocket_handshake_request_info.h"
#include "net/base/net_errors.h"

using base::AutoLock;
using content::BrowserThread;
using content::ResourceRequestInfo;
using content::WebSocketHandshakeRequestInfo;
using net::StaticCookiePolicy;

namespace android_webview {

namespace {
base::LazyInstance<AwCookieAccessPolicy>::Leaky g_lazy_instance;
}  // namespace

AwCookieAccessPolicy::~AwCookieAccessPolicy() {
}

AwCookieAccessPolicy::AwCookieAccessPolicy()
    : accept_cookies_(true) {
}

AwCookieAccessPolicy* AwCookieAccessPolicy::GetInstance() {
  return g_lazy_instance.Pointer();
}

bool AwCookieAccessPolicy::GetShouldAcceptCookies() {
  AutoLock lock(lock_);
  return accept_cookies_;
}

void AwCookieAccessPolicy::SetShouldAcceptCookies(bool allow) {
  AutoLock lock(lock_);
  accept_cookies_ = allow;
}

bool AwCookieAccessPolicy::GetShouldAcceptThirdPartyCookies(
    int render_process_id,
    int render_frame_id,
    int frame_tree_node_id) {
  std::unique_ptr<AwContentsIoThreadClient> io_thread_client =
      (frame_tree_node_id != content::RenderFrameHost::kNoFrameTreeNodeId)
          ? AwContentsIoThreadClient::FromID(frame_tree_node_id)
          : AwContentsIoThreadClient::FromID(render_process_id,
                                             render_frame_id);

  if (!io_thread_client) {
    return false;
  }
  return io_thread_client->ShouldAcceptThirdPartyCookies();
}

bool AwCookieAccessPolicy::GetShouldAcceptThirdPartyCookies(
    const net::URLRequest& request) {
  int child_id = 0;
  int render_frame_id = 0;
  int frame_tree_node_id = content::RenderFrameHost::kNoFrameTreeNodeId;
  const ResourceRequestInfo* info = ResourceRequestInfo::ForRequest(&request);
  if (info) {
    child_id = info->GetChildID();
    render_frame_id = info->GetRenderFrameID();
    frame_tree_node_id = info->GetFrameTreeNodeId();
  } else {
    const WebSocketHandshakeRequestInfo* websocket_info =
        WebSocketHandshakeRequestInfo::ForRequest(&request);
    if (!websocket_info)
      return false;
    child_id = websocket_info->GetChildId();
    render_frame_id = websocket_info->GetRenderFrameId();
  }
  return GetShouldAcceptThirdPartyCookies(child_id, render_frame_id,
                                          frame_tree_node_id);
}

bool AwCookieAccessPolicy::OnCanGetCookies(const net::URLRequest& request,
                                           const net::CookieList& cookie_list) {
  bool global = GetShouldAcceptCookies();
  bool thirdParty = GetShouldAcceptThirdPartyCookies(request);
  return AwStaticCookiePolicy(global, thirdParty)
      .AllowGet(request.url(), request.site_for_cookies());
}

bool AwCookieAccessPolicy::OnCanSetCookie(const net::URLRequest& request,
                                          const net::CanonicalCookie& cookie,
                                          net::CookieOptions* options) {
  bool global = GetShouldAcceptCookies();
  bool thirdParty = GetShouldAcceptThirdPartyCookies(request);
  return AwStaticCookiePolicy(global, thirdParty)
      .AllowSet(request.url(), request.site_for_cookies());
}

bool AwCookieAccessPolicy::AllowGetCookie(const GURL& url,
                                          const GURL& first_party,
                                          const net::CookieList& cookie_list,
                                          content::ResourceContext* context,
                                          int render_process_id,
                                          int render_frame_id) {
  bool global = GetShouldAcceptCookies();
  bool thirdParty = GetShouldAcceptThirdPartyCookies(
      render_process_id, render_frame_id,
      content::RenderFrameHost::kNoFrameTreeNodeId);
  return AwStaticCookiePolicy(global, thirdParty).AllowGet(url, first_party);
}

bool AwCookieAccessPolicy::AllowSetCookie(const GURL& url,
                                          const GURL& first_party,
                                          const net::CanonicalCookie& cookie,
                                          content::ResourceContext* context,
                                          int render_process_id,
                                          int render_frame_id,
                                          const net::CookieOptions& options) {
  bool global = GetShouldAcceptCookies();
  bool thirdParty = GetShouldAcceptThirdPartyCookies(
      render_process_id, render_frame_id,
      content::RenderFrameHost::kNoFrameTreeNodeId);
  return AwStaticCookiePolicy(global, thirdParty).AllowSet(url, first_party);
}

AwStaticCookiePolicy::AwStaticCookiePolicy(bool accept_cookies,
                                           bool accept_third_party_cookies)
    : accept_cookies_(accept_cookies),
      accept_third_party_cookies_(accept_third_party_cookies) {
}

StaticCookiePolicy::Type AwStaticCookiePolicy::GetPolicy(const GURL& url)
    const {
  // File URLs are a special case. We want file URLs to be able to set cookies
  // but (for the purpose of cookies) Chrome considers different file URLs to
  // come from different origins so we use the 'allow all' cookie policy for
  // file URLs.
  bool isFile = url.SchemeIsFile();
  if (!accept_cookies()) {
    return StaticCookiePolicy::BLOCK_ALL_COOKIES;
  }
  if (accept_third_party_cookies() || isFile) {
    return StaticCookiePolicy::ALLOW_ALL_COOKIES;
  }
  return StaticCookiePolicy::BLOCK_ALL_THIRD_PARTY_COOKIES;
}

bool AwStaticCookiePolicy::AllowSet(const GURL& url,
                                    const GURL& first_party) const {
  return StaticCookiePolicy(GetPolicy(url))
             .CanAccessCookies(url, first_party) == net::OK;
}

bool AwStaticCookiePolicy::AllowGet(const GURL& url,
                                    const GURL& first_party) const {
  return StaticCookiePolicy(GetPolicy(url))
             .CanAccessCookies(url, first_party) == net::OK;
}

}  // namespace android_webview
