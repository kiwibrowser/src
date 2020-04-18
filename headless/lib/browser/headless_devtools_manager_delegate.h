// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_HEADLESS_DEVTOOLS_MANAGER_DELEGATE_H_
#define HEADLESS_LIB_BROWSER_HEADLESS_DEVTOOLS_MANAGER_DELEGATE_H_

#include <map>
#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "content/public/browser/devtools_manager_delegate.h"

namespace headless {
class HeadlessBrowserImpl;

namespace protocol {
class HeadlessDevToolsSession;
}

class HeadlessDevToolsManagerDelegate
    : public content::DevToolsManagerDelegate {
 public:
  explicit HeadlessDevToolsManagerDelegate(
      base::WeakPtr<HeadlessBrowserImpl> browser);
  ~HeadlessDevToolsManagerDelegate() override;

  // DevToolsManagerDelegate implementation:
  bool HandleCommand(content::DevToolsAgentHost* agent_host,
                     content::DevToolsAgentHostClient* client,
                     base::DictionaryValue* command) override;
  scoped_refptr<content::DevToolsAgentHost> CreateNewTarget(
      const GURL& url) override;
  std::string GetDiscoveryPageHTML() override;
  bool HasBundledFrontendResources() override;
  void ClientAttached(content::DevToolsAgentHost* agent_host,
                      content::DevToolsAgentHostClient* client) override;
  void ClientDetached(content::DevToolsAgentHost* agent_host,
                      content::DevToolsAgentHostClient* client) override;

 private:
  base::WeakPtr<HeadlessBrowserImpl> browser_;
  std::map<content::DevToolsAgentHostClient*,
           std::unique_ptr<protocol::HeadlessDevToolsSession>>
      sessions_;
};

}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_HEADLESS_DEVTOOLS_MANAGER_DELEGATE_H_
