// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_DEVTOOLS_AGENT_HOST_CLIENT_H_
#define CONTENT_PUBLIC_BROWSER_DEVTOOLS_AGENT_HOST_CLIENT_H_

#include <string>

#include "content/common/content_export.h"

namespace content {

class DevToolsAgentHost;

// DevToolsAgentHostClient can attach to a DevToolsAgentHost and start
// debugging it.
class CONTENT_EXPORT DevToolsAgentHostClient {
 public:
  virtual ~DevToolsAgentHostClient() {}

  // Dispatches given protocol message on the client.
  virtual void DispatchProtocolMessage(DevToolsAgentHost* agent_host,
                                       const std::string& message) = 0;

  // This method is called when attached agent host is closed.
  virtual void AgentHostClosed(DevToolsAgentHost* agent_host) = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_DEVTOOLS_AGENT_HOST_CLIENT_H_
