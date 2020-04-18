// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BROADCASTCHANNEL_BROADCAST_CHANNEL_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BROADCASTCHANNEL_BROADCAST_CHANNEL_H_

#include "mojo/public/cpp/bindings/associated_binding.h"
#include "third_party/blink/public/platform/modules/broadcastchannel/broadcast_channel.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

class ScriptValue;

class BroadcastChannel final : public EventTargetWithInlineData,
                               public ActiveScriptWrappable<BroadcastChannel>,
                               public ContextLifecycleObserver,
                               public mojom::blink::BroadcastChannelClient {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(BroadcastChannel);
  USING_PRE_FINALIZER(BroadcastChannel, Dispose);
  WTF_MAKE_NONCOPYABLE(BroadcastChannel);

 public:
  static BroadcastChannel* Create(ExecutionContext*,
                                  const String& name,
                                  ExceptionState&);
  ~BroadcastChannel() override;
  void Dispose();

  // IDL
  String name() const { return name_; }
  void postMessage(const ScriptValue&, ExceptionState&);
  void close();
  DEFINE_ATTRIBUTE_EVENT_LISTENER(message);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(messageerror);

  // EventTarget:
  const AtomicString& InterfaceName() const override;
  ExecutionContext* GetExecutionContext() const override {
    return ContextLifecycleObserver::GetExecutionContext();
  }

  // ScriptWrappable:
  bool HasPendingActivity() const override;

  // ContextLifecycleObserver:
  void ContextDestroyed(ExecutionContext*) override;

  void Trace(blink::Visitor*) override;

 private:
  BroadcastChannel(ExecutionContext*, const String& name);

  // mojom::blink::BroadcastChannelClient:
  void OnMessage(BlinkCloneableMessage) override;

  // Called when the mojo binding disconnects.
  void OnError();

  scoped_refptr<const SecurityOrigin> origin_;
  String name_;

  mojo::AssociatedBinding<mojom::blink::BroadcastChannelClient> binding_;
  mojom::blink::BroadcastChannelClientAssociatedPtr remote_client_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BROADCASTCHANNEL_BROADCAST_CHANNEL_H_
