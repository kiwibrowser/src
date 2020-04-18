// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/cssom/css_url_image_value.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/css/css_image_value.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/loader/resource/image_resource_content.h"
#include "third_party/blink/renderer/core/style/style_image.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

CSSURLImageValue* CSSURLImageValue::Create(ScriptState* script_state,
                                           const AtomicString& url,
                                           ExceptionState& exception_state) {
  const auto* execution_context = ExecutionContext::From(script_state);
  DCHECK(execution_context);
  KURL parsed_url = execution_context->CompleteURL(url);
  if (!parsed_url.IsValid()) {
    exception_state.ThrowTypeError("Failed to parse URL from " + url);
    return nullptr;
  }
  // Use absolute URL for CSSImageValue but keep relative URL for
  // getter and serialization.
  return new CSSURLImageValue(
      *CSSImageValue::Create(url, parsed_url, Referrer()));
}

CSSURLImageValue* CSSURLImageValue::FromCSSValue(const CSSImageValue& value) {
  return new CSSURLImageValue(value);
}

const String& CSSURLImageValue::url() const {
  return value_->RelativeUrl();
}

base::Optional<IntSize> CSSURLImageValue::IntrinsicSize() const {
  if (Status() != ResourceStatus::kCached)
    return base::nullopt;

  DCHECK(!value_->IsCachePending());
  ImageResourceContent* resource_content = value_->CachedImage()->CachedImage();
  return resource_content
             ? resource_content->IntrinsicSize(kDoNotRespectImageOrientation)
             : IntSize(0, 0);
}

ResourceStatus CSSURLImageValue::Status() const {
  if (value_->IsCachePending())
    return ResourceStatus::kNotStarted;
  return value_->CachedImage()->CachedImage()->GetContentStatus();
}

scoped_refptr<Image> CSSURLImageValue::GetSourceImageForCanvas(
    SourceImageStatus*,
    AccelerationHint,
    const FloatSize&) {
  return GetImage();
}

scoped_refptr<Image> CSSURLImageValue::GetImage() const {
  if (value_->IsCachePending())
    return nullptr;
  // cachedImage can be null if image is StyleInvalidImage
  ImageResourceContent* cached_image = value_->CachedImage()->CachedImage();
  if (cached_image) {
    // getImage() returns the nullImage() if the image is not available yet
    return cached_image->GetImage()->ImageForDefaultFrame();
  }
  return nullptr;
}

bool CSSURLImageValue::IsAccelerated() const {
  return GetImage() && GetImage()->IsTextureBacked();
}

const CSSValue* CSSURLImageValue::ToCSSValue() const {
  return value_;
}

void CSSURLImageValue::Trace(blink::Visitor* visitor) {
  visitor->Trace(value_);
  CSSStyleImageValue::Trace(visitor);
}

}  // namespace blink
