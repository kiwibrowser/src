// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/headless_devtools_manager_delegate.h"

#include "build/build_config.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/web_contents.h"
#include "headless/grit/headless_lib_resources.h"
#include "headless/lib/browser/headless_browser_context_impl.h"
#include "headless/lib/browser/headless_browser_impl.h"
#include "headless/lib/browser/headless_web_contents_impl.h"
#include "headless/lib/browser/protocol/headless_devtools_session.h"
#include "ui/base/resource/resource_bundle.h"

namespace headless {

HeadlessDevToolsManagerDelegate::HeadlessDevToolsManagerDelegate(
    base::WeakPtr<HeadlessBrowserImpl> browser)
    : browser_(std::move(browser)) {}

HeadlessDevToolsManagerDelegate::~HeadlessDevToolsManagerDelegate() = default;

bool HeadlessDevToolsManagerDelegate::HandleCommand(
    content::DevToolsAgentHost* agent_host,
    content::DevToolsAgentHostClient* client,
    base::DictionaryValue* command) {
  if (!browser_)
    return false;
  DCHECK(sessions_.find(client) != sessions_.end());
  auto response = sessions_[client]->dispatcher()->dispatch(
      protocol::toProtocolValue(command, 1000));
  return response != protocol::DispatchResponse::Status::kFallThrough;
}

scoped_refptr<content::DevToolsAgentHost>
HeadlessDevToolsManagerDelegate::CreateNewTarget(const GURL& url) {
  if (!browser_)
    return nullptr;

  HeadlessBrowserContext* context = browser_->GetDefaultBrowserContext();
  HeadlessWebContentsImpl* web_contents_impl = HeadlessWebContentsImpl::From(
      context->CreateWebContentsBuilder()
          .SetInitialURL(url)
          .SetWindowSize(browser_->options()->window_size)
          .Build());
  return content::DevToolsAgentHost::GetOrCreateFor(
      web_contents_impl->web_contents());
}

std::string HeadlessDevToolsManagerDelegate::GetDiscoveryPageHTML() {
  return ui::ResourceBundle::GetSharedInstance()
      .GetRawDataResource(IDR_HEADLESS_LIB_DEVTOOLS_DISCOVERY_PAGE)
      .as_string();
}

bool HeadlessDevToolsManagerDelegate::HasBundledFrontendResources() {
  return true;
}

void HeadlessDevToolsManagerDelegate::ClientAttached(
    content::DevToolsAgentHost* agent_host,
    content::DevToolsAgentHostClient* client) {
  DCHECK(sessions_.find(client) == sessions_.end());
  sessions_[client] = std::make_unique<protocol::HeadlessDevToolsSession>(
      browser_, agent_host, client);
}

void HeadlessDevToolsManagerDelegate::ClientDetached(
    content::DevToolsAgentHost* agent_host,
    content::DevToolsAgentHostClient* client) {
  sessions_.erase(client);
}

}  // namespace headless
