// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/protocol/target_handler.h"

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "content/browser/devtools/devtools_manager.h"
#include "content/browser/devtools/devtools_session.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
namespace protocol {

namespace {

static const char kMethod[] = "method";
static const char kResumeMethod[] = "Runtime.runIfWaitingForDebugger";

std::unique_ptr<Target::TargetInfo> CreateInfo(DevToolsAgentHost* host) {
  std::unique_ptr<Target::TargetInfo> target_info =
      Target::TargetInfo::Create()
          .SetTargetId(host->GetId())
          .SetTitle(host->GetTitle())
          .SetUrl(host->GetURL().spec())
          .SetType(host->GetType())
          .SetAttached(host->IsAttached())
          .Build();
  if (!host->GetOpenerId().empty())
    target_info->SetOpenerId(host->GetOpenerId());
  if (host->GetBrowserContext())
    target_info->SetBrowserContextId(host->GetBrowserContext()->UniqueId());
  return target_info;
}

}  // namespace

// Throttle is owned externally by the navigation subsystem.
class TargetHandler::Throttle : public content::NavigationThrottle {
 public:
  Throttle(base::WeakPtr<protocol::TargetHandler> target_handler,
           content::NavigationHandle* navigation_handle);
  ~Throttle() override;
  void Clear();
  // content::NavigationThrottle implementation:
  NavigationThrottle::ThrottleCheckResult WillProcessResponse() override;
  const char* GetNameForLogging() override;

 private:
  void CleanupPointers();

  base::WeakPtr<protocol::TargetHandler> target_handler_;
  scoped_refptr<DevToolsAgentHost> agent_host_;

  DISALLOW_COPY_AND_ASSIGN(Throttle);
};

class TargetHandler::Session : public DevToolsAgentHostClient {
 public:
  static std::string Attach(TargetHandler* handler,
                            DevToolsAgentHost* agent_host,
                            bool waiting_for_debugger) {
    std::string id = base::StringPrintf("%s:%d", agent_host->GetId().c_str(),
                                        ++handler->last_session_id_);
    Session* session = new Session(handler, agent_host, id);
    handler->attached_sessions_[id].reset(session);
    agent_host->AttachClient(session);
    handler->frontend_->AttachedToTarget(id, CreateInfo(agent_host),
                                         waiting_for_debugger);
    return id;
  }

  ~Session() override {
    if (agent_host_)
      agent_host_->DetachClient(this);
  }

  void Detach(bool host_closed) {
    handler_->frontend_->DetachedFromTarget(id_, agent_host_->GetId());
    if (host_closed)
      handler_->auto_attacher_.AgentHostClosed(agent_host_.get());
    else
      agent_host_->DetachClient(this);
    handler_->auto_attached_sessions_.erase(agent_host_.get());
    agent_host_ = nullptr;
    handler_->attached_sessions_.erase(id_);
  }

  void SetThrottle(Throttle* throttle) { throttle_ = throttle; }

  void SendMessageToAgentHost(const std::string& message) {
    if (throttle_) {
      bool resuming = false;
      std::unique_ptr<base::Value> value = base::JSONReader::Read(message);
      if (value && value->is_dict()) {
        base::Value* method = value->FindKey(kMethod);
        resuming = method && method->is_string() &&
                   method->GetString() == kResumeMethod;
      }
      if (resuming)
        throttle_->Clear();
    }

    agent_host_->DispatchProtocolMessage(this, message);
  }

  bool IsAttachedTo(const std::string& target_id) {
    return agent_host_->GetId() == target_id;
  }

 private:
  Session(TargetHandler* handler,
          DevToolsAgentHost* agent_host,
          const std::string& id)
      : handler_(handler), agent_host_(agent_host), id_(id) {}

  // DevToolsAgentHostClient implementation.
  void DispatchProtocolMessage(DevToolsAgentHost* agent_host,
                               const std::string& message) override {
    DCHECK(agent_host == agent_host_.get());
    handler_->frontend_->ReceivedMessageFromTarget(id_, message,
                                                   agent_host_->GetId());
  }

  void AgentHostClosed(DevToolsAgentHost* agent_host) override {
    DCHECK(agent_host == agent_host_.get());
    Detach(true);
  }

  TargetHandler* handler_;
  scoped_refptr<DevToolsAgentHost> agent_host_;
  std::string id_;
  Throttle* throttle_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(Session);
};

TargetHandler::Throttle::Throttle(
    base::WeakPtr<protocol::TargetHandler> target_handler,
    content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle),
      target_handler_(target_handler) {
  target_handler->throttles_.insert(this);
}

TargetHandler::Throttle::~Throttle() {
  CleanupPointers();
}

void TargetHandler::Throttle::CleanupPointers() {
  if (target_handler_ && agent_host_) {
    auto it = target_handler_->auto_attached_sessions_.find(agent_host_.get());
    if (it != target_handler_->auto_attached_sessions_.end())
      it->second->SetThrottle(nullptr);
  }
  if (target_handler_) {
    target_handler_->throttles_.erase(this);
    target_handler_ = nullptr;
  }
}

NavigationThrottle::ThrottleCheckResult
TargetHandler::Throttle::WillProcessResponse() {
  if (!target_handler_)
    return PROCEED;
  agent_host_ = target_handler_->auto_attacher_.AutoAttachToFrame(
      static_cast<NavigationHandleImpl*>(navigation_handle()));
  if (!agent_host_.get())
    return PROCEED;
  target_handler_->auto_attached_sessions_[agent_host_.get()]->SetThrottle(
      this);
  return DEFER;
}

const char* TargetHandler::Throttle::GetNameForLogging() {
  return "DevToolsTargetNavigationThrottle";
}

void TargetHandler::Throttle::Clear() {
  CleanupPointers();
  if (agent_host_) {
    agent_host_ = nullptr;
    Resume();
  }
}

TargetHandler::TargetHandler(bool browser_only)
    : DevToolsDomainHandler(Target::Metainfo::domainName),
      auto_attacher_(
          base::Bind(&TargetHandler::AutoAttach, base::Unretained(this)),
          base::Bind(&TargetHandler::AutoDetach, base::Unretained(this))),
      discover_(false),
      browser_only_(browser_only),
      weak_factory_(this) {}

TargetHandler::~TargetHandler() {
}

// static
std::vector<TargetHandler*> TargetHandler::ForAgentHost(
    DevToolsAgentHostImpl* host) {
  return DevToolsSession::HandlersForAgentHost<TargetHandler>(
      host, Target::Metainfo::domainName);
}

void TargetHandler::Wire(UberDispatcher* dispatcher) {
  frontend_.reset(new Target::Frontend(dispatcher->channel()));
  Target::Dispatcher::wire(dispatcher, this);
}

void TargetHandler::SetRenderer(int process_host_id,
                                RenderFrameHostImpl* frame_host) {
  auto_attacher_.SetRenderFrameHost(frame_host);
}

Response TargetHandler::Disable() {
  SetAutoAttach(false, false);
  SetDiscoverTargets(false);
  auto_attached_sessions_.clear();
  attached_sessions_.clear();
  return Response::OK();
}

void TargetHandler::DidCommitNavigation() {
  auto_attacher_.UpdateServiceWorkers();
}

std::unique_ptr<NavigationThrottle> TargetHandler::CreateThrottleForNavigation(
    NavigationHandle* navigation_handle) {
  if (!auto_attacher_.ShouldThrottleFramesNavigation())
    return nullptr;
  return std::make_unique<Throttle>(weak_factory_.GetWeakPtr(),
                                    navigation_handle);
}

void TargetHandler::ClearThrottles() {
  base::flat_set<Throttle*> copy(throttles_);
  for (Throttle* throttle : copy)
    throttle->Clear();
  throttles_.clear();
}

void TargetHandler::AutoAttach(DevToolsAgentHost* host,
                               bool waiting_for_debugger) {
  std::string session_id = Session::Attach(this, host, waiting_for_debugger);
  auto_attached_sessions_[host] = attached_sessions_[session_id].get();
}

void TargetHandler::AutoDetach(DevToolsAgentHost* host) {
  auto it = auto_attached_sessions_.find(host);
  if (it == auto_attached_sessions_.end())
    return;
  it->second->Detach(false);
}

Response TargetHandler::FindSession(Maybe<std::string> session_id,
                                    Maybe<std::string> target_id,
                                    Session** session,
                                    bool fall_through) {
  *session = nullptr;
  fall_through &= !browser_only_;
  if (session_id.isJust()) {
    auto it = attached_sessions_.find(session_id.fromJust());
    if (it == attached_sessions_.end()) {
      if (fall_through)
        return Response::FallThrough();
      return Response::InvalidParams("No session with given id");
    }
    *session = it->second.get();
    return Response::OK();
  }
  if (target_id.isJust()) {
    for (auto& it : attached_sessions_) {
      if (it.second->IsAttachedTo(target_id.fromJust())) {
        if (*session)
          return Response::Error("Multiple sessions attached, specify id.");
        *session = it.second.get();
      }
    }
    if (!*session) {
      if (fall_through)
        return Response::FallThrough();
      return Response::InvalidParams("No session for given target id");
    }
    return Response::OK();
  }
  if (fall_through)
    return Response::FallThrough();
  return Response::InvalidParams("Session id must be specified");
}

// ----------------- Protocol ----------------------

Response TargetHandler::SetDiscoverTargets(bool discover) {
  if (discover_ == discover)
    return Response::OK();
  discover_ = discover;
  if (discover_) {
    DevToolsAgentHost::AddObserver(this);
  } else {
    DevToolsAgentHost::RemoveObserver(this);
    reported_hosts_.clear();
  }
  return Response::OK();
}

Response TargetHandler::SetAutoAttach(
    bool auto_attach, bool wait_for_debugger_on_start) {
  auto_attacher_.SetAutoAttach(auto_attach, wait_for_debugger_on_start);
  if (!auto_attacher_.ShouldThrottleFramesNavigation())
    ClearThrottles();
  return browser_only_ ? Response::OK() : Response::FallThrough();
}

Response TargetHandler::SetRemoteLocations(
    std::unique_ptr<protocol::Array<Target::RemoteLocation>>) {
  return Response::Error("Not supported");
}

Response TargetHandler::AttachToTarget(const std::string& target_id,
                                       std::string* out_session_id) {
  // TODO(dgozman): only allow reported hosts.
  scoped_refptr<DevToolsAgentHost> agent_host =
      DevToolsAgentHost::GetForId(target_id);
  if (!agent_host)
    return Response::InvalidParams("No target with given id found");
  *out_session_id = Session::Attach(this, agent_host.get(), false);
  return Response::OK();
}

Response TargetHandler::DetachFromTarget(Maybe<std::string> session_id,
                                         Maybe<std::string> target_id) {
  Session* session = nullptr;
  Response response =
      FindSession(std::move(session_id), std::move(target_id), &session, false);
  if (!response.isSuccess())
    return response;
  session->Detach(false);
  return Response::OK();
}

Response TargetHandler::SendMessageToTarget(const std::string& message,
                                            Maybe<std::string> session_id,
                                            Maybe<std::string> target_id) {
  Session* session = nullptr;
  Response response =
      FindSession(std::move(session_id), std::move(target_id), &session, true);
  if (!response.isSuccess())
    return response;
  session->SendMessageToAgentHost(message);
  return Response::OK();
}

Response TargetHandler::GetTargetInfo(
    const std::string& target_id,
    std::unique_ptr<Target::TargetInfo>* target_info) {
  // TODO(dgozman): only allow reported hosts.
  scoped_refptr<DevToolsAgentHost> agent_host(
      DevToolsAgentHost::GetForId(target_id));
  if (!agent_host)
    return Response::InvalidParams("No target with given id found");
  *target_info = CreateInfo(agent_host.get());
  return Response::OK();
}

Response TargetHandler::ActivateTarget(const std::string& target_id) {
  // TODO(dgozman): only allow reported hosts.
  scoped_refptr<DevToolsAgentHost> agent_host(
      DevToolsAgentHost::GetForId(target_id));
  if (!agent_host)
    return Response::InvalidParams("No target with given id found");
  agent_host->Activate();
  return Response::OK();
}

Response TargetHandler::CloseTarget(const std::string& target_id,
                                    bool* out_success) {
  scoped_refptr<DevToolsAgentHost> agent_host =
      DevToolsAgentHost::GetForId(target_id);
  if (!agent_host)
    return Response::InvalidParams("No target with given id found");
  *out_success = agent_host->Close();
  return Response::OK();
}

Response TargetHandler::CreateBrowserContext(std::string* out_context_id) {
  return Response::Error("Not supported");
}

Response TargetHandler::DisposeBrowserContext(const std::string& context_id) {
  return Response::Error("Not supported");
}

Response TargetHandler::GetBrowserContexts(
    std::unique_ptr<protocol::Array<String>>* browser_context_ids) {
  return Response::Error("Not supported");
}

Response TargetHandler::CreateTarget(const std::string& url,
                                     Maybe<int> width,
                                     Maybe<int> height,
                                     Maybe<std::string> context_id,
                                     Maybe<bool> enable_begin_frame_control,
                                     std::string* out_target_id) {
  DevToolsManagerDelegate* delegate =
      DevToolsManager::GetInstance()->delegate();
  if (!delegate)
    return Response::Error("Not supported");
  scoped_refptr<content::DevToolsAgentHost> agent_host =
      delegate->CreateNewTarget(GURL(url));
  if (!agent_host)
    return Response::Error("Not supported");
  *out_target_id = agent_host->GetId();
  return Response::OK();
}

Response TargetHandler::GetTargets(
    std::unique_ptr<protocol::Array<Target::TargetInfo>>* target_infos) {
  *target_infos = protocol::Array<Target::TargetInfo>::create();
  for (const auto& host : DevToolsAgentHost::GetOrCreateAll())
    (*target_infos)->addItem(CreateInfo(host.get()));
  return Response::OK();
}

// -------------- DevToolsAgentHostObserver -----------------

bool TargetHandler::ShouldForceDevToolsAgentHostCreation() {
  return true;
}

void TargetHandler::DevToolsAgentHostCreated(DevToolsAgentHost* host) {
  // If we start discovering late, all existing agent hosts will be reported,
  // but we could have already attached to some.
  if (reported_hosts_.find(host) != reported_hosts_.end())
    return;
  frontend_->TargetCreated(CreateInfo(host));
  reported_hosts_.insert(host);
}

void TargetHandler::DevToolsAgentHostNavigated(DevToolsAgentHost* host) {
  if (reported_hosts_.find(host) == reported_hosts_.end())
    return;
  frontend_->TargetInfoChanged(CreateInfo(host));
}

void TargetHandler::DevToolsAgentHostDestroyed(DevToolsAgentHost* host) {
  if (reported_hosts_.find(host) == reported_hosts_.end())
    return;
  frontend_->TargetDestroyed(host->GetId());
  reported_hosts_.erase(host);
}

void TargetHandler::DevToolsAgentHostAttached(DevToolsAgentHost* host) {
  if (reported_hosts_.find(host) == reported_hosts_.end())
    return;
  frontend_->TargetInfoChanged(CreateInfo(host));
}

void TargetHandler::DevToolsAgentHostDetached(DevToolsAgentHost* host) {
  if (reported_hosts_.find(host) == reported_hosts_.end())
    return;
  frontend_->TargetInfoChanged(CreateInfo(host));
}

}  // namespace protocol
}  // namespace content
