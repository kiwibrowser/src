// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_CONSOLE_MESSAGE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_CONSOLE_MESSAGE_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/inspector/console_types.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class DocumentLoader;
class LocalFrame;
class SourceLocation;
class WorkerThread;

class CORE_EXPORT ConsoleMessage final
    : public GarbageCollectedFinalized<ConsoleMessage> {
 public:
  // Location must be non-null.
  static ConsoleMessage* Create(MessageSource,
                                MessageLevel,
                                const String& message,
                                std::unique_ptr<SourceLocation>);

  // Shortcut when location is unknown. Captures current location.
  static ConsoleMessage* Create(MessageSource,
                                MessageLevel,
                                const String& message);

  // This method captures current location if available.
  static ConsoleMessage* CreateForRequest(MessageSource,
                                          MessageLevel,
                                          const String& message,
                                          const String& url,
                                          DocumentLoader*,
                                          unsigned long request_identifier);

  // This creates message from WorkerMessageSource.
  static ConsoleMessage* CreateFromWorker(MessageLevel,
                                          const String& message,
                                          std::unique_ptr<SourceLocation>,
                                          WorkerThread*);

  ~ConsoleMessage();

  SourceLocation* Location() const;
  const String& RequestIdentifier() const;
  double Timestamp() const;
  MessageSource Source() const;
  MessageLevel Level() const;
  const String& Message() const;
  const String& WorkerId() const;
  LocalFrame* Frame() const;
  Vector<DOMNodeId>& Nodes();
  void SetNodes(LocalFrame*, Vector<DOMNodeId> nodes);

  void Trace(blink::Visitor*);

 private:
  ConsoleMessage(MessageSource,
                 MessageLevel,
                 const String& message,
                 std::unique_ptr<SourceLocation>);

  MessageSource source_;
  MessageLevel level_;
  String message_;
  std::unique_ptr<SourceLocation> location_;
  String request_identifier_;
  double timestamp_;
  String worker_id_;
  WeakMember<LocalFrame> frame_;
  Vector<DOMNodeId> nodes_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_CONSOLE_MESSAGE_H_
