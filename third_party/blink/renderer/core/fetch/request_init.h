// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_REQUEST_INIT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_REQUEST_INIT_H_

#include "base/memory/scoped_refptr.h"
#include "base/optional.h"
#include "third_party/blink/renderer/bindings/core/v8/byte_string_sequence_sequence_or_byte_string_byte_string_record.h"
#include "third_party/blink/renderer/bindings/core/v8/native_value_traits.h"
#include "third_party/blink/renderer/core/fetch/headers.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/weborigin/referrer.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class AbortSignal;
class BytesConsumer;
class Dictionary;
class ExecutionContext;
class ExceptionState;

// FIXME: Use IDL dictionary instead of this class.
class RequestInit {
  STACK_ALLOCATED();

 public:
  RequestInit(ExecutionContext*, const Dictionary&, ExceptionState&);

  const String& Method() const { return method_; }
  const HeadersInit& GetHeaders() const { return headers_; }
  const String& ContentType() const { return content_type_; }
  BytesConsumer* GetBody() { return body_; }
  const Referrer& GetReferrer() const { return referrer_; }
  const String& Mode() const { return mode_; }
  const String& Credentials() const { return credentials_; }
  const String& CacheMode() const { return cache_; }
  const String& Redirect() const { return redirect_; }
  const String& Integrity() const { return integrity_; }
  const base::Optional<bool>& Keepalive() const { return keepalive_; }
  base::Optional<AbortSignal*> Signal() const;
  bool AreAnyMembersSet() const { return are_any_members_set_; }

 private:
  // These are defined here to avoid JUMBO ambiguity.
  class GetterHelper;
  struct IDLPassThrough;
  friend struct NativeValueTraits<IDLPassThrough>;
  friend struct NativeValueTraitsBase<IDLPassThrough>;

  void CheckEnumValues(const base::Optional<String>& referrer_string,
                       const base::Optional<String>& referrer_policy_string,
                       ExceptionState&);
  void SetUpBody(ExecutionContext*,
                 v8::Isolate*,
                 v8::Local<v8::Value> v8_body,
                 ExceptionState&);

  String method_;
  HeadersInit headers_;
  String content_type_;
  Member<BytesConsumer> body_;
  Referrer referrer_;
  String mode_;
  String credentials_;
  String cache_;
  String redirect_;
  String integrity_;
  base::Optional<bool> keepalive_;
  base::Optional<Member<AbortSignal>> signal_;
  // True if any members in RequestInit are set and hence the referrer member
  // should be used in the Request constructor.
  bool are_any_members_set_ = false;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_REQUEST_INIT_H_
