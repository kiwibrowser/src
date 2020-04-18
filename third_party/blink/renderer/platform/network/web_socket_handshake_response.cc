/*
 * Copyright (C) 2010 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/network/web_socket_handshake_response.h"

#include "third_party/blink/renderer/platform/network/web_socket_handshake_request.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

WebSocketHandshakeResponse::WebSocketHandshakeResponse() = default;

WebSocketHandshakeResponse::~WebSocketHandshakeResponse() = default;

int WebSocketHandshakeResponse::StatusCode() const {
  return status_code_;
}

void WebSocketHandshakeResponse::SetStatusCode(int status_code) {
  DCHECK_GE(status_code, 100);
  DCHECK_LT(status_code, 600);
  status_code_ = status_code;
}

const String& WebSocketHandshakeResponse::StatusText() const {
  return status_text_;
}

void WebSocketHandshakeResponse::SetStatusText(const String& status_text) {
  status_text_ = status_text;
}

const HTTPHeaderMap& WebSocketHandshakeResponse::HeaderFields() const {
  return header_fields_;
}

void WebSocketHandshakeResponse::AddHeaderField(const AtomicString& name,
                                                const AtomicString& value) {
  WebSocketHandshakeRequest::AddAndMergeHeader(&header_fields_, name, value);
}

}  // namespace blink
