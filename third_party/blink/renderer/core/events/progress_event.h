/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_PROGRESS_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_PROGRESS_EVENT_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/events/progress_event_init.h"

namespace blink {

class CORE_EXPORT ProgressEvent : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static ProgressEvent* Create() { return new ProgressEvent; }
  static ProgressEvent* Create(const AtomicString& type,
                               bool length_computable,
                               unsigned long long loaded,
                               unsigned long long total) {
    return new ProgressEvent(type, length_computable, loaded, total);
  }
  static ProgressEvent* Create(const AtomicString& type,
                               const ProgressEventInit& initializer) {
    return new ProgressEvent(type, initializer);
  }

  bool lengthComputable() const { return length_computable_; }
  unsigned long long loaded() const { return loaded_; }
  unsigned long long total() const { return total_; }

  const AtomicString& InterfaceName() const override;

  void Trace(blink::Visitor*) override;

 protected:
  ProgressEvent();
  ProgressEvent(const AtomicString& type,
                bool length_computable,
                unsigned long long loaded,
                unsigned long long total);
  ProgressEvent(const AtomicString&, const ProgressEventInit&);

 private:
  bool length_computable_;
  unsigned long long loaded_;
  unsigned long long total_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_PROGRESS_EVENT_H_
