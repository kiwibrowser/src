// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_TESTING_MOCK_DEVTOOLS_AGENT_HOST_H_
#define HEADLESS_PUBLIC_UTIL_TESTING_MOCK_DEVTOOLS_AGENT_HOST_H_

#include "content/public/browser/devtools_agent_host.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace headless {

class MockDevToolsAgentHost : public content::DevToolsAgentHost {
 public:
  MockDevToolsAgentHost();
  ~MockDevToolsAgentHost() override;

  // DevToolsAgentHost implementation.
  MOCK_METHOD1(AttachClient, void(content::DevToolsAgentHostClient* client));
  MOCK_METHOD1(AttachRestrictedClient,
               bool(content::DevToolsAgentHostClient* client));
  MOCK_METHOD1(DetachClient, bool(content::DevToolsAgentHostClient* client));
  MOCK_METHOD2(DispatchProtocolMessage,
               bool(content::DevToolsAgentHostClient* client,
                    const std::string& message));
  MOCK_METHOD2(SendProtocolMessageToClient,
               bool(int session_id, const std::string& message));
  MOCK_METHOD0(IsAttached, bool());
  MOCK_METHOD3(InspectElement,
               void(content::RenderFrameHost* frame_host, int x, int y));
  MOCK_METHOD0(GetId, std::string());
  MOCK_METHOD0(GetParentId, std::string());
  MOCK_METHOD0(GetOpenerId, std::string());
  MOCK_METHOD0(GetDescription, std::string());
  MOCK_METHOD0(GetFaviconURL, GURL());
  MOCK_METHOD0(GetFrontendURL, std::string());
  MOCK_METHOD0(GetLastActivityTime, base::TimeTicks());
  MOCK_METHOD0(GetBrowserContext, content::BrowserContext*());
  MOCK_METHOD0(GetWebContents, content::WebContents*());
  MOCK_METHOD0(DisconnectWebContents, void());
  MOCK_METHOD1(ConnectWebContents, void(content::WebContents* wc));

  MOCK_METHOD0(GetType, std::string());
  MOCK_METHOD0(GetTitle, std::string());
  MOCK_METHOD0(GetURL, GURL());
  MOCK_METHOD0(Activate, bool());
  MOCK_METHOD0(Reload, void());
  MOCK_METHOD0(Close, bool());
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_TESTING_MOCK_DEVTOOLS_AGENT_HOST_H_
