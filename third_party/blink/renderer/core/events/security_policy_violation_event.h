/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_SECURITY_POLICY_VIOLATION_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_SECURITY_POLICY_VIOLATION_EVENT_H_

#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/events/security_policy_violation_event_init.h"
#include "third_party/blink/renderer/platform/network/content_security_policy_parsers.h"

namespace blink {

class SecurityPolicyViolationEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static SecurityPolicyViolationEvent* Create(
      const AtomicString& type,
      const SecurityPolicyViolationEventInit& initializer) {
    return new SecurityPolicyViolationEvent(type, initializer);
  }

  const String& documentURI() const { return document_uri_; }
  const String& referrer() const { return referrer_; }
  const String& blockedURI() const { return blocked_uri_; }
  const String& violatedDirective() const { return violated_directive_; }
  const String& effectiveDirective() const { return effective_directive_; }
  const String& originalPolicy() const { return original_policy_; }
  const String& disposition() const;
  const String& sourceFile() const { return source_file_; }
  const String& sample() const { return sample_; }
  int lineNumber() const { return line_number_; }
  int columnNumber() const { return column_number_; }
  uint16_t statusCode() const { return status_code_; }

  const AtomicString& InterfaceName() const override {
    return EventNames::SecurityPolicyViolationEvent;
  }

  void Trace(blink::Visitor* visitor) override { Event::Trace(visitor); }

 private:
  SecurityPolicyViolationEvent(
      const AtomicString& type,
      const SecurityPolicyViolationEventInit& initializer);

  String document_uri_;
  String referrer_;
  String blocked_uri_;
  String violated_directive_;
  String effective_directive_;
  String original_policy_;
  ContentSecurityPolicyHeaderType disposition_;
  String source_file_;
  String sample_;
  int line_number_;
  int column_number_;
  int status_code_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_SECURITY_POLICY_VIOLATION_EVENT_H_
