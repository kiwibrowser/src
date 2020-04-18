// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_EXTENDABLE_MESSAGE_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_EXTENDABLE_MESSAGE_EVENT_H_

#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/modules/serviceworkers/extendable_event.h"
#include "third_party/blink/renderer/modules/serviceworkers/extendable_message_event_init.h"

namespace blink {

class MODULES_EXPORT ExtendableMessageEvent final : public ExtendableEvent {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static ExtendableMessageEvent* Create(
      const AtomicString& type,
      const ExtendableMessageEventInit& initializer);
  static ExtendableMessageEvent* Create(
      const AtomicString& type,
      const ExtendableMessageEventInit& initializer,
      WaitUntilObserver*);
  static ExtendableMessageEvent* Create(
      scoped_refptr<SerializedScriptValue> data,
      const String& origin,
      MessagePortArray* ports,
      WaitUntilObserver*);
  static ExtendableMessageEvent* Create(
      scoped_refptr<SerializedScriptValue> data,
      const String& origin,
      MessagePortArray* ports,
      ServiceWorkerClient* source,
      WaitUntilObserver*);
  static ExtendableMessageEvent* Create(
      scoped_refptr<SerializedScriptValue> data,
      const String& origin,
      MessagePortArray* ports,
      ServiceWorker* source,
      WaitUntilObserver*);

  SerializedScriptValue* SerializedData() const {
    return serialized_data_.get();
  }
  void SetSerializedData(scoped_refptr<SerializedScriptValue> serialized_data) {
    serialized_data_ = std::move(serialized_data);
  }
  const String& origin() const { return origin_; }
  const String& lastEventId() const { return last_event_id_; }
  MessagePortArray ports() const;
  void source(ClientOrServiceWorkerOrMessagePort& result) const;

  const AtomicString& InterfaceName() const override;

  void Trace(blink::Visitor*) override;

 private:
  ExtendableMessageEvent(const AtomicString& type,
                         const ExtendableMessageEventInit& initializer);
  ExtendableMessageEvent(const AtomicString& type,
                         const ExtendableMessageEventInit& initializer,
                         WaitUntilObserver*);
  ExtendableMessageEvent(scoped_refptr<SerializedScriptValue> data,
                         const String& origin,
                         MessagePortArray* ports,
                         WaitUntilObserver*);

  scoped_refptr<SerializedScriptValue> serialized_data_;
  String origin_;
  String last_event_id_;
  Member<ServiceWorkerClient> source_as_client_;
  Member<ServiceWorker> source_as_service_worker_;
  Member<MessagePort> source_as_message_port_;
  Member<MessagePortArray> ports_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_EXTENDABLE_MESSAGE_EVENT_H_
