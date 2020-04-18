// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/inspector/inspector_session.h"

#include "third_party/blink/renderer/bindings/core/v8/script_controller.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/inspector/inspector_base_agent.h"
#include "third_party/blink/renderer/core/inspector/protocol/Protocol.h"
#include "third_party/blink/renderer/core/inspector/v8_inspector_string.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"

namespace blink {

namespace {
const char kV8StateKey[] = "v8";
}

InspectorSession::InspectorSession(Client* client,
                                   CoreProbeSink* instrumenting_agents,
                                   int session_id,
                                   v8_inspector::V8Inspector* inspector,
                                   int context_group_id,
                                   const String& reattach_state)
    : client_(client),
      v8_session_(nullptr),
      session_id_(session_id),
      disposed_(false),
      instrumenting_agents_(instrumenting_agents),
      inspector_backend_dispatcher_(new protocol::UberDispatcher(this)) {
  String v8_state;
  if (!reattach_state.IsNull()) {
    std::unique_ptr<protocol::Value> state =
        protocol::StringUtil::parseJSON(reattach_state);
    if (state)
      state_ = protocol::DictionaryValue::cast(std::move(state));
    if (!state_)
      state_ = protocol::DictionaryValue::create();
    state_->getString(kV8StateKey, &v8_state);
  } else {
    state_ = protocol::DictionaryValue::create();
  }
  v8_session_ = inspector->connect(context_group_id, this,
                                   ToV8InspectorStringView(v8_state));
}

InspectorSession::~InspectorSession() {
  DCHECK(disposed_);
}

void InspectorSession::Append(InspectorAgent* agent) {
  agents_.push_back(agent);
  agent->Init(instrumenting_agents_.Get(), inspector_backend_dispatcher_.get(),
              state_.get());
}

void InspectorSession::Restore() {
  DCHECK(!disposed_);
  for (size_t i = 0; i < agents_.size(); i++)
    agents_[i]->Restore();
}

void InspectorSession::Dispose() {
  DCHECK(!disposed_);
  disposed_ = true;
  inspector_backend_dispatcher_.reset();
  for (size_t i = agents_.size(); i > 0; i--)
    agents_[i - 1]->Dispose();
  agents_.clear();
  v8_session_.reset();
}

void InspectorSession::DispatchProtocolMessage(const String& method,
                                               const String& message) {
  DCHECK(!disposed_);
  if (v8_inspector::V8InspectorSession::canDispatchMethod(
          ToV8InspectorStringView(method))) {
    v8_session_->dispatchProtocolMessage(ToV8InspectorStringView(message));
  } else {
    inspector_backend_dispatcher_->dispatch(
        protocol::StringUtil::parseJSON(message));
  }
}

void InspectorSession::DispatchProtocolMessage(const String& message) {
  DCHECK(!disposed_);
  String method;
  std::unique_ptr<protocol::DictionaryValue> parsedMessage;
  if (!inspector_backend_dispatcher_->getCommandName(message, &method,
                                                     &parsedMessage))
    return;
  if (v8_inspector::V8InspectorSession::canDispatchMethod(
          ToV8InspectorStringView(method))) {
    v8_session_->dispatchProtocolMessage(ToV8InspectorStringView(message));
  } else {
    inspector_backend_dispatcher_->dispatch(std::move(parsedMessage));
  }
}

void InspectorSession::DidCommitLoadForLocalFrame(LocalFrame* frame) {
  for (size_t i = 0; i < agents_.size(); i++)
    agents_[i]->DidCommitLoadForLocalFrame(frame);
}

void InspectorSession::sendProtocolResponse(
    int call_id,
    std::unique_ptr<protocol::Serializable> message) {
  SendProtocolResponse(call_id, message->serialize());
}

void InspectorSession::sendResponse(
    int call_id,
    std::unique_ptr<v8_inspector::StringBuffer> message) {
  // We can potentially avoid copies if WebString would convert to utf8 right
  // from StringView, but it uses StringImpl itself, so we don't create any
  // extra copies here.
  SendProtocolResponse(call_id, ToCoreString(message->string()));
}

void InspectorSession::SendProtocolResponse(int call_id,
                                            const String& message) {
  if (disposed_)
    return;
  flushProtocolNotifications();
  client_->SendProtocolResponse(session_id_, call_id, message,
                                GetStateToSend());
}

String InspectorSession::GetStateToSend() {
  state_->setString(kV8StateKey, ToCoreString(v8_session_->stateJSON()));
  String state_to_send = state_->serialize();
  if (state_to_send == last_sent_state_)
    state_to_send = String();
  else
    last_sent_state_ = state_to_send;
  return state_to_send;
}

class InspectorSession::Notification {
 public:
  static std::unique_ptr<Notification> CreateForBlink(
      std::unique_ptr<protocol::Serializable> notification) {
    return std::unique_ptr<Notification>(
        new Notification(std::move(notification)));
  }

  static std::unique_ptr<Notification> CreateForV8(
      std::unique_ptr<v8_inspector::StringBuffer> notification) {
    return std::unique_ptr<Notification>(
        new Notification(std::move(notification)));
  }

  String Serialize() {
    if (blink_notification_) {
      serialized_ = blink_notification_->serialize();
      blink_notification_.reset();
    } else if (v8_notification_) {
      serialized_ = ToCoreString(v8_notification_->string());
      v8_notification_.reset();
    }
    return serialized_;
  }

 private:
  explicit Notification(std::unique_ptr<protocol::Serializable> notification)
      : blink_notification_(std::move(notification)) {}

  explicit Notification(
      std::unique_ptr<v8_inspector::StringBuffer> notification)
      : v8_notification_(std::move(notification)) {}

  std::unique_ptr<protocol::Serializable> blink_notification_;
  std::unique_ptr<v8_inspector::StringBuffer> v8_notification_;
  String serialized_;
};

void InspectorSession::sendProtocolNotification(
    std::unique_ptr<protocol::Serializable> notification) {
  if (disposed_)
    return;
  notification_queue_.push_back(
      Notification::CreateForBlink(std::move(notification)));
}

void InspectorSession::sendNotification(
    std::unique_ptr<v8_inspector::StringBuffer> notification) {
  if (disposed_)
    return;
  notification_queue_.push_back(
      Notification::CreateForV8(std::move(notification)));
}

void InspectorSession::flushProtocolNotifications() {
  if (disposed_)
    return;
  for (size_t i = 0; i < agents_.size(); i++)
    agents_[i]->FlushPendingProtocolNotifications();
  if (!notification_queue_.size())
    return;
  String state_to_send = GetStateToSend();
  for (size_t i = 0; i < notification_queue_.size(); ++i) {
    client_->SendProtocolNotification(
        session_id_, notification_queue_[i]->Serialize(), state_to_send);
    // Only send state once in this series of serialized updates.
    state_to_send = String();
  }
  notification_queue_.clear();
}

void InspectorSession::Trace(blink::Visitor* visitor) {
  visitor->Trace(instrumenting_agents_);
  visitor->Trace(agents_);
}

}  // namespace blink
