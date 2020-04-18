// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_AGENT_HOST_IMPL_H_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_AGENT_HOST_IMPL_H_

#include <stdint.h>

#include <string>

#include "base/compiler_specific.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "content/browser/devtools/devtools_io_context.h"
#include "content/common/content_export.h"
#include "content/public/browser/certificate_request_result_type.h"
#include "content/public/browser/devtools_agent_host.h"
#include "third_party/blink/public/web/devtools_agent.mojom.h"

namespace content {

class BrowserContext;
class DevToolsSession;

// Describes interface for managing devtools agents from the browser process.
class CONTENT_EXPORT DevToolsAgentHostImpl : public DevToolsAgentHost {
 public:
  // Asks any interested agents to handle the given certificate error. Returns
  // |true| if the error was handled, |false| otherwise.
  using CertErrorCallback =
      base::RepeatingCallback<void(content::CertificateRequestResultType)>;
  static bool HandleCertificateError(WebContents* web_contents,
                                     int cert_error,
                                     const GURL& request_url,
                                     CertErrorCallback callback);

  // DevToolsAgentHost implementation.
  void AttachClient(DevToolsAgentHostClient* client) override;
  bool AttachRestrictedClient(DevToolsAgentHostClient* client) override;
  bool DetachClient(DevToolsAgentHostClient* client) override;
  bool DispatchProtocolMessage(DevToolsAgentHostClient* client,
                               const std::string& message) override;
  bool IsAttached() override;
  void InspectElement(RenderFrameHost* frame_host, int x, int y) override;
  std::string GetId() override;
  std::string GetParentId() override;
  std::string GetOpenerId() override;
  std::string GetDescription() override;
  GURL GetFaviconURL() override;
  std::string GetFrontendURL() override;
  base::TimeTicks GetLastActivityTime() override;
  BrowserContext* GetBrowserContext() override;
  WebContents* GetWebContents() override;
  void DisconnectWebContents() override;
  void ConnectWebContents(WebContents* wc) override;

  bool Inspect();

 protected:
  DevToolsAgentHostImpl(const std::string& id);
  ~DevToolsAgentHostImpl() override;

  static bool ShouldForceCreation();

  // Returning |false| will block the attach.
  virtual bool AttachSession(DevToolsSession* session);
  virtual void DetachSession(DevToolsSession* session);
  virtual void DispatchProtocolMessage(DevToolsSession* session,
                                       const std::string& message);

  void NotifyCreated();
  void NotifyNavigated();
  void ForceDetachAllSessions();
  void ForceDetachRestrictedSessions();
  DevToolsIOContext* GetIOContext() { return &io_context_; }

  base::flat_set<DevToolsSession*>& sessions() { return sessions_; }

 private:
  friend class DevToolsAgentHost; // for static methods
  friend class DevToolsSession;
  bool InnerAttachClient(DevToolsAgentHostClient* client, bool restricted);
  void InnerDetachClient(DevToolsAgentHostClient* client);
  void NotifyAttached();
  void NotifyDetached();
  void NotifyDestroyed();
  DevToolsSession* SessionByClient(DevToolsAgentHostClient* client);

  const std::string id_;
  base::flat_set<DevToolsSession*> sessions_;
  base::flat_map<DevToolsAgentHostClient*, std::unique_ptr<DevToolsSession>>
      session_by_client_;
  DevToolsIOContext io_context_;
  static int s_force_creation_count_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_AGENT_HOST_IMPL_H_
