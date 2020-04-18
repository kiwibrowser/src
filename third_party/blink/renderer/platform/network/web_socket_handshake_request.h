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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_NETWORK_WEB_SOCKET_HANDSHAKE_REQUEST_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_NETWORK_WEB_SOCKET_HANDSHAKE_REQUEST_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/network/http_header_map.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class HTTPHeaderMap;

class PLATFORM_EXPORT WebSocketHandshakeRequest final
    : public RefCounted<WebSocketHandshakeRequest> {
 public:
  static scoped_refptr<WebSocketHandshakeRequest> Create(const KURL& url) {
    return base::AdoptRef(new WebSocketHandshakeRequest(url));
  }
  static scoped_refptr<WebSocketHandshakeRequest> Create() {
    return base::AdoptRef(new WebSocketHandshakeRequest);
  }
  static scoped_refptr<WebSocketHandshakeRequest> Create(
      const WebSocketHandshakeRequest& request) {
    return base::AdoptRef(new WebSocketHandshakeRequest(request));
  }
  virtual ~WebSocketHandshakeRequest();

  void AddAndMergeHeader(const AtomicString& name, const AtomicString& value) {
    AddAndMergeHeader(&header_fields_, name, value);
  }

  // Merges the existing value with |value| in |map| if |map| already has
  // |name|.  Associates |value| with |name| in |map| otherwise.
  // This function builds data for inspector.
  static void AddAndMergeHeader(HTTPHeaderMap* /* map */,
                                const AtomicString& name,
                                const AtomicString& value);

  void AddHeaderField(const AtomicString& name, const AtomicString& value) {
    header_fields_.Add(name, value);
  }

  KURL Url() const { return url_; }
  void SetURL(const KURL& url) { url_ = url; }
  const HTTPHeaderMap& HeaderFields() const { return header_fields_; }
  const String& HeadersText() const { return headers_text_; }
  void SetHeadersText(const String& text) { headers_text_ = text; }

 private:
  WebSocketHandshakeRequest(const KURL&);
  WebSocketHandshakeRequest();
  WebSocketHandshakeRequest(const WebSocketHandshakeRequest&);

  KURL url_;
  HTTPHeaderMap header_fields_;
  String headers_text_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_NETWORK_WEB_SOCKET_HANDSHAKE_REQUEST_H_
