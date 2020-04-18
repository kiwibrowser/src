// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/web_http_header_map.h"

#include <memory>
#include <string>
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/platform/network/http_header_map.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

class WebHTTPHeaderMap::WebHTTPHeaderMapImpl {
 public:
  explicit WebHTTPHeaderMapImpl(const HTTPHeaderMap& map) : map_(map){};

  explicit WebHTTPHeaderMapImpl(const net::HttpRequestHeaders* headers) {
    for (net::HttpRequestHeaders::Iterator it(*headers); it.GetNext();) {
      map_.Add(
          WTF::AtomicString::FromUTF8(it.name().c_str(), it.name().length()),
          WTF::AtomicString::FromUTF8(it.value().c_str(), it.value().length()));
    }
  };

  explicit WebHTTPHeaderMapImpl(const net::HttpResponseHeaders* headers) {
    size_t iter = 0;
    std::string name;
    std::string value;

    while (headers->EnumerateHeaderLines(&iter, &name, &value)) {
      WTF::AtomicString atomic_name =
          WTF::AtomicString::FromUTF8(name.c_str(), name.length());
      WTF::AtomicString atomic_value =
          WTF::AtomicString::FromUTF8(value.c_str(), value.length());

      if (map_.Contains(atomic_name))
        map_.Set(atomic_name, map_.Get(atomic_name) + "," + atomic_value);
      else
        map_.Add(atomic_name, atomic_value);
    }
  };

  const HTTPHeaderMap& map() const { return map_; };

 private:
  HTTPHeaderMap map_;
};

WebHTTPHeaderMap::~WebHTTPHeaderMap() = default;
;

WebHTTPHeaderMap::WebHTTPHeaderMap(const HTTPHeaderMap& map) {
  implementation_ = std::make_unique<WebHTTPHeaderMapImpl>(map);
}

WebHTTPHeaderMap::WebHTTPHeaderMap(const net::HttpResponseHeaders* headers) {
  implementation_ = std::make_unique<WebHTTPHeaderMapImpl>(headers);
}

WebHTTPHeaderMap::WebHTTPHeaderMap(const net::HttpRequestHeaders* headers) {
  implementation_ = std::make_unique<WebHTTPHeaderMapImpl>(headers);
}

const HTTPHeaderMap& WebHTTPHeaderMap::GetHTTPHeaderMap() const {
  return implementation_->map();
}

WebString WebHTTPHeaderMap::Get(const WebString& name) const {
  return implementation_->map().Get(name).GetString();
}

}  // namespace blink
