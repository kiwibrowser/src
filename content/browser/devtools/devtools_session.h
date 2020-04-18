// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_SESSION_H_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_SESSION_H_

#include <map>

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/values.h"
#include "content/browser/devtools/devtools_agent_host_impl.h"
#include "content/browser/devtools/protocol/devtools_domain_handler.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "third_party/blink/public/web/devtools_agent.mojom.h"

namespace content {

class DevToolsAgentHostClient;
class RenderFrameHostImpl;

class DevToolsSession : public protocol::FrontendChannel,
                        public blink::mojom::DevToolsSessionHost {
 public:
  DevToolsSession(DevToolsAgentHostImpl* agent_host,
                  DevToolsAgentHostClient* client,
                  bool restricted);
  ~DevToolsSession() override;

  bool restricted() { return restricted_; }
  DevToolsAgentHostClient* client() { return client_; };

  // Browser-only sessions do not talk to mojom::DevToolsAgent, but instead
  // handle all protocol messages locally in the browser process.
  void SetBrowserOnly(bool browser_only);

  void AddHandler(std::unique_ptr<protocol::DevToolsDomainHandler> handler);
  // TODO(dgozman): maybe combine this with AttachToAgent?
  void SetRenderer(int process_host_id, RenderFrameHostImpl* frame_host);

  void AttachToAgent(const blink::mojom::DevToolsAgentAssociatedPtr& agent);
  void DispatchProtocolMessage(const std::string& message);
  void SuspendSendingMessagesToAgent();
  void ResumeSendingMessagesToAgent();

  template <typename Handler>
  static std::vector<Handler*> HandlersForAgentHost(
      DevToolsAgentHostImpl* agent_host,
      const std::string& name) {
    std::vector<Handler*> result;
    if (agent_host->sessions().empty())
      return result;
    for (DevToolsSession* session : agent_host->sessions()) {
      auto it = session->handlers_.find(name);
      if (it != session->handlers_.end())
        result.push_back(static_cast<Handler*>(it->second.get()));
    }
    return result;
  }

 private:
  void SendResponse(std::unique_ptr<base::DictionaryValue> response);
  void MojoConnectionDestroyed();
  void DispatchProtocolMessageToAgent(int call_id,
                                      const std::string& method,
                                      const std::string& message);

  // protocol::FrontendChannel implementation.
  void sendProtocolResponse(
      int call_id,
      std::unique_ptr<protocol::Serializable> message) override;
  void sendProtocolNotification(
      std::unique_ptr<protocol::Serializable> message) override;
  void flushProtocolNotifications() override;

  // blink::mojom::DevToolsSessionHost implementation.
  void DispatchProtocolResponse(
      const std::string& message,
      int call_id,
      const base::Optional<std::string>& state) override;
  void DispatchProtocolNotification(
      const std::string& message,
      const base::Optional<std::string>& state) override;

  mojo::AssociatedBinding<blink::mojom::DevToolsSessionHost> binding_;
  blink::mojom::DevToolsSessionAssociatedPtr session_ptr_;
  blink::mojom::DevToolsSessionPtr io_session_ptr_;
  DevToolsAgentHostImpl* agent_host_;
  DevToolsAgentHostClient* client_;
  bool restricted_;
  bool browser_only_ = false;
  base::flat_map<std::string, std::unique_ptr<protocol::DevToolsDomainHandler>>
      handlers_;
  int process_host_id_;
  RenderFrameHostImpl* host_;
  std::unique_ptr<protocol::UberDispatcher> dispatcher_;

  // These messages were queued after suspending, not sent to the agent,
  // and will be sent after resuming.
  struct SuspendedMessage {
    int call_id;
    std::string method;
    std::string message;
  };
  std::vector<SuspendedMessage> suspended_messages_;
  bool suspended_sending_messages_to_agent_ = false;

  // These messages have been sent to agent, but did not get a response yet.
  struct WaitingMessage {
    std::string method;
    std::string message;
  };
  std::map<int, WaitingMessage> waiting_for_response_messages_;

  // |state_cookie_| always corresponds to a state before
  // any of the waiting for response messages have been handled.
  // Note that |state_cookie_| is not present only before first attach.
  base::Optional<std::string> state_cookie_;

  base::WeakPtrFactory<DevToolsSession> weak_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_SESSION_H_
