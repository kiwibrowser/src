// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_DEVTOOLS_MANAGER_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_DEVTOOLS_MANAGER_DELEGATE_H_

#include <memory>
#include <string>

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "content/public/browser/devtools_agent_host.h"
#include "url/gurl.h"

namespace base {
class DictionaryValue;
}

namespace content {

class DevToolsAgentHostClient;
class WebContents;

class CONTENT_EXPORT DevToolsManagerDelegate {
 public:
  // Opens the inspector for |agent_host|.
  virtual void Inspect(DevToolsAgentHost* agent_host);

  // Returns DevToolsAgentHost type to use for given |web_contents| target.
  virtual std::string GetTargetType(WebContents* web_contents);

  // Returns DevToolsAgentHost title to use for given |web_contents| target.
  virtual std::string GetTargetTitle(WebContents* web_contents);

  // Returns DevToolsAgentHost title to use for given |web_contents| target.
  virtual std::string GetTargetDescription(WebContents* web_contents);

  // Returns all targets embedder would like to report as debuggable
  // remotely.
  virtual DevToolsAgentHost::List RemoteDebuggingTargets();

  // Creates new inspectable target given the |url|.
  virtual scoped_refptr<DevToolsAgentHost> CreateNewTarget(const GURL& url);

  // Called when a new client is attached/detached.
  virtual void ClientAttached(DevToolsAgentHost* agent_host,
                              DevToolsAgentHostClient* client);
  virtual void ClientDetached(DevToolsAgentHost* agent_host,
                              DevToolsAgentHostClient* client);

  // Returns true if the command has been handled, false otherwise.
  virtual bool HandleCommand(DevToolsAgentHost* agent_host,
                             DevToolsAgentHostClient* client,
                             base::DictionaryValue* command);

  // Should return discovery page HTML that should list available tabs
  // and provide attach links.
  virtual std::string GetDiscoveryPageHTML();

  // Returns whether frontend resources are bundled within the binary.
  virtual bool HasBundledFrontendResources();

  // Makes browser target easily discoverable for remote debugging.
  // This should only return true when remote debugging endpoint is not
  // accessible by the web (for example in Chrome for Android where it is
  // exposed via UNIX named socket) or when content/ embedder is built for
  // running in the controlled environment (for example a special build for
  // the Lab testing). If you want to return true here, please get security
  // clearance from the devtools owners.
  virtual bool IsBrowserTargetDiscoverable();

  virtual ~DevToolsManagerDelegate();
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_DEVTOOLS_MANAGER_DELEGATE_H_
