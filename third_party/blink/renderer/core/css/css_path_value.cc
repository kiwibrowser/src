// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/css_path_value.h"

#include <memory>
#include "third_party/blink/renderer/core/style/style_path.h"
#include "third_party/blink/renderer/core/svg/svg_path_utilities.h"

namespace blink {

namespace cssvalue {

CSSPathValue* CSSPathValue::Create(scoped_refptr<StylePath> style_path) {
  return new CSSPathValue(std::move(style_path));
}

CSSPathValue* CSSPathValue::Create(
    std::unique_ptr<SVGPathByteStream> path_byte_stream) {
  return CSSPathValue::Create(StylePath::Create(std::move(path_byte_stream)));
}

CSSPathValue::CSSPathValue(scoped_refptr<StylePath> style_path)
    : CSSValue(kPathClass), style_path_(std::move(style_path)) {
  DCHECK(style_path_);
}

namespace {

CSSPathValue* CreatePathValue() {
  std::unique_ptr<SVGPathByteStream> path_byte_stream =
      SVGPathByteStream::Create();
  // Need to be registered as LSan ignored, as it will be reachable and
  // separately referred to by emptyPathValue() callers.
  LEAK_SANITIZER_IGNORE_OBJECT(path_byte_stream.get());
  return CSSPathValue::Create(std::move(path_byte_stream));
}

}  // namespace

CSSPathValue& CSSPathValue::EmptyPathValue() {
  DEFINE_STATIC_LOCAL(CSSPathValue, empty, (CreatePathValue()));
  return empty;
}

String CSSPathValue::CustomCSSText() const {
  return "path('" + BuildStringFromByteStream(ByteStream()) + "')";
}

bool CSSPathValue::Equals(const CSSPathValue& other) const {
  return ByteStream() == other.ByteStream();
}

void CSSPathValue::TraceAfterDispatch(blink::Visitor* visitor) {
  CSSValue::TraceAfterDispatch(visitor);
}

}  // namespace cssvalue
}  // namespace blink
