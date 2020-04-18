// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/referrer_script_info.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "v8/include/v8.h"

namespace blink {

namespace {

enum HostDefinedOptionsIndex : size_t {
  kBaseURL,
  kCredentialsMode,
  kNonce,
  kParserState,
  kLength
};

}  // namespace

ReferrerScriptInfo ReferrerScriptInfo::FromV8HostDefinedOptions(
    v8::Local<v8::Context> context,
    v8::Local<v8::PrimitiveArray> host_defined_options) {
  if (host_defined_options.IsEmpty() || !host_defined_options->Length()) {
    return ReferrerScriptInfo();
  }

  v8::Local<v8::Primitive> base_url_value = host_defined_options->Get(kBaseURL);
  String base_url_string =
      ToCoreStringWithNullCheck(v8::Local<v8::String>::Cast(base_url_value));
  KURL base_url = base_url_string.IsEmpty() ? KURL() : KURL(base_url_string);
  DCHECK(base_url.IsNull() || base_url.IsValid());

  v8::Local<v8::Primitive> credentials_mode_value =
      host_defined_options->Get(kCredentialsMode);
  auto credentials_mode = static_cast<network::mojom::FetchCredentialsMode>(
      credentials_mode_value->IntegerValue(context).ToChecked());

  v8::Local<v8::Primitive> nonce_value = host_defined_options->Get(kNonce);
  String nonce =
      ToCoreStringWithNullCheck(v8::Local<v8::String>::Cast(nonce_value));

  v8::Local<v8::Primitive> parser_state_value =
      host_defined_options->Get(kParserState);
  ParserDisposition parser_state = static_cast<ParserDisposition>(
      parser_state_value->IntegerValue(context).ToChecked());

  return ReferrerScriptInfo(base_url, credentials_mode, nonce, parser_state);
}

v8::Local<v8::PrimitiveArray> ReferrerScriptInfo::ToV8HostDefinedOptions(
    v8::Isolate* isolate) const {
  if (IsDefaultValue())
    return v8::Local<v8::PrimitiveArray>();

  v8::Local<v8::PrimitiveArray> host_defined_options =
      v8::PrimitiveArray::New(isolate, HostDefinedOptionsIndex::kLength);

  v8::Local<v8::Primitive> base_url_value =
      V8String(isolate, base_url_.GetString());
  host_defined_options->Set(HostDefinedOptionsIndex::kBaseURL, base_url_value);

  v8::Local<v8::Primitive> credentials_mode_value =
      v8::Integer::NewFromUnsigned(isolate,
                                   static_cast<uint32_t>(credentials_mode_));
  host_defined_options->Set(HostDefinedOptionsIndex::kCredentialsMode,
                            credentials_mode_value);

  v8::Local<v8::Primitive> nonce_value = V8String(isolate, nonce_);
  host_defined_options->Set(HostDefinedOptionsIndex::kNonce, nonce_value);

  v8::Local<v8::Primitive> parser_state_value = v8::Integer::NewFromUnsigned(
      isolate, static_cast<uint32_t>(parser_state_));
  host_defined_options->Set(HostDefinedOptionsIndex::kParserState,
                            parser_state_value);

  return host_defined_options;
}

}  // namespace blink
