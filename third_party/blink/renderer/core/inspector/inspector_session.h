// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_SESSION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_SESSION_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/inspector/protocol/Forward.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "v8/include/v8-inspector-protocol.h"

namespace blink {

class InspectorAgent;
class CoreProbeSink;
class LocalFrame;

class CORE_EXPORT InspectorSession
    : public GarbageCollectedFinalized<InspectorSession>,
      public protocol::FrontendChannel,
      public v8_inspector::V8Inspector::Channel {
 public:
  class Client {
   public:
    virtual void SendProtocolResponse(int session_id,
                                      int call_id,
                                      const String& response,
                                      const String& state) = 0;
    virtual void SendProtocolNotification(int session_id,
                                          const String& message,
                                          const String& state) = 0;
    virtual ~Client() = default;
  };

  InspectorSession(Client*,
                   CoreProbeSink*,
                   int session_id,
                   v8_inspector::V8Inspector*,
                   int context_group_id,
                   const String& reattach_state);
  ~InspectorSession() override;
  // TODO(dgozman): remove session id once WokrerInspectorController
  // does not use it anymore.
  int SessionId() { return session_id_; }
  v8_inspector::V8InspectorSession* V8Session() { return v8_session_.get(); }

  void Append(InspectorAgent*);
  void Restore();
  void Dispose();
  void DidCommitLoadForLocalFrame(LocalFrame*);
  void DispatchProtocolMessage(const String& method, const String& message);
  void DispatchProtocolMessage(const String& message);
  void flushProtocolNotifications() override;

  void Trace(blink::Visitor*);

 private:
  // protocol::FrontendChannel implementation.
  void sendProtocolResponse(
      int call_id,
      std::unique_ptr<protocol::Serializable> message) override;
  void sendProtocolNotification(
      std::unique_ptr<protocol::Serializable> message) override;

  // v8_inspector::V8Inspector::Channel implementation.
  void sendResponse(
      int call_id,
      std::unique_ptr<v8_inspector::StringBuffer> message) override;
  void sendNotification(
      std::unique_ptr<v8_inspector::StringBuffer> message) override;

  void SendProtocolResponse(int call_id, const String& message);

  String GetStateToSend();

  Client* client_;
  std::unique_ptr<v8_inspector::V8InspectorSession> v8_session_;
  int session_id_;
  bool disposed_;
  Member<CoreProbeSink> instrumenting_agents_;
  std::unique_ptr<protocol::UberDispatcher> inspector_backend_dispatcher_;
  std::unique_ptr<protocol::DictionaryValue> state_;
  HeapVector<Member<InspectorAgent>> agents_;
  class Notification;
  Vector<std::unique_ptr<Notification>> notification_queue_;
  String last_sent_state_;

  DISALLOW_COPY_AND_ASSIGN(InspectorSession);
};

}  // namespace blink

#endif  // !defined(InspectorSession_h)
