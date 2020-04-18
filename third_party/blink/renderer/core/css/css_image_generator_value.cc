/*
 * Copyright (C) 2008 Apple Inc.  All rights reserved.
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

#include "third_party/blink/renderer/core/css/css_image_generator_value.h"

#include "third_party/blink/renderer/core/css/css_crossfade_value.h"
#include "third_party/blink/renderer/core/css/css_gradient_value.h"
#include "third_party/blink/renderer/core/css/css_paint_value.h"
#include "third_party/blink/renderer/platform/graphics/image.h"

namespace blink {

using cssvalue::ToCSSCrossfadeValue;
using cssvalue::ToCSSConicGradientValue;
using cssvalue::ToCSSLinearGradientValue;
using cssvalue::ToCSSRadialGradientValue;

Image* GeneratedImageCache::GetImage(const FloatSize& size) const {
  if (size.IsEmpty())
    return nullptr;

  DCHECK(sizes_.find(size) != sizes_.end());
  GeneratedImageMap::const_iterator image_iter = images_.find(size);
  if (image_iter == images_.end())
    return nullptr;
  return image_iter->second.get();
}

void GeneratedImageCache::PutImage(const FloatSize& size,
                                   scoped_refptr<Image> image) {
  DCHECK(!size.IsEmpty());
  images_.insert(
      std::pair<FloatSize, scoped_refptr<Image>>(size, std::move(image)));
}

void GeneratedImageCache::AddSize(const FloatSize& size) {
  DCHECK(!size.IsEmpty());
  ImageSizeCountMap::iterator size_entry = sizes_.find(size);
  if (size_entry == sizes_.end())
    sizes_.insert(std::pair<FloatSize, unsigned>(size, 1));
  else
    size_entry->second++;
}

void GeneratedImageCache::RemoveSize(const FloatSize& size) {
  DCHECK(!size.IsEmpty());
  SECURITY_DCHECK(sizes_.find(size) != sizes_.end());
  unsigned& count = sizes_[size];
  count--;
  if (count == 0) {
    DCHECK(images_.find(size) != images_.end());
    sizes_.erase(sizes_.find(size));
    images_.erase(images_.find(size));
  }
}

CSSImageGeneratorValue::CSSImageGeneratorValue(ClassType class_type)
    : CSSValue(class_type) {}

CSSImageGeneratorValue::~CSSImageGeneratorValue() = default;

void CSSImageGeneratorValue::AddClient(const ImageResourceObserver* client) {
  DCHECK(client);
  if (clients_.IsEmpty()) {
    DCHECK(!keep_alive_);
    keep_alive_ = this;
  }

  SizeAndCount& size_count =
      clients_.insert(client, SizeAndCount()).stored_value->value;
  size_count.count++;
}

CSSImageGeneratorValue* CSSImageGeneratorValue::ValueWithURLsMadeAbsolute() {
  if (IsCrossfadeValue())
    return ToCSSCrossfadeValue(this)->ValueWithURLsMadeAbsolute();
  return this;
}

void CSSImageGeneratorValue::RemoveClient(const ImageResourceObserver* client) {
  DCHECK(client);
  ClientSizeCountMap::iterator it = clients_.find(client);
  SECURITY_DCHECK(it != clients_.end());

  SizeAndCount& size_count = it->value;
  if (!size_count.size.IsEmpty()) {
    cached_images_.RemoveSize(size_count.size);
    size_count.size = FloatSize();
  }

  if (!--size_count.count)
    clients_.erase(client);

  if (clients_.IsEmpty()) {
    DCHECK(keep_alive_);
    keep_alive_.Clear();
  }
}

Image* CSSImageGeneratorValue::GetImage(const ImageResourceObserver* client,
                                        const FloatSize& size) const {
  ClientSizeCountMap::iterator it = clients_.find(client);
  if (it != clients_.end()) {
    DCHECK(keep_alive_);
    SizeAndCount& size_count = it->value;
    if (size_count.size != size) {
      if (!size_count.size.IsEmpty()) {
        cached_images_.RemoveSize(size_count.size);
        size_count.size = FloatSize();
      }

      if (!size.IsEmpty()) {
        cached_images_.AddSize(size);
        size_count.size = size;
      }
    }
  }
  return cached_images_.GetImage(size);
}

void CSSImageGeneratorValue::PutImage(const FloatSize& size,
                                      scoped_refptr<Image> image) const {
  cached_images_.PutImage(size, std::move(image));
}

scoped_refptr<Image> CSSImageGeneratorValue::GetImage(
    const ImageResourceObserver& client,
    const Document& document,
    const ComputedStyle& style,
    const FloatSize& target_size) {
  switch (GetClassType()) {
    case kCrossfadeClass:
      return ToCSSCrossfadeValue(this)->GetImage(client, document, style,
                                                 target_size);
    case kLinearGradientClass:
      return ToCSSLinearGradientValue(this)->GetImage(client, document, style,
                                                      target_size);
    case kPaintClass:
      return ToCSSPaintValue(this)->GetImage(client, document, style,
                                             target_size);
    case kRadialGradientClass:
      return ToCSSRadialGradientValue(this)->GetImage(client, document, style,
                                                      target_size);
    case kConicGradientClass:
      return ToCSSConicGradientValue(this)->GetImage(client, document, style,
                                                     target_size);
    default:
      NOTREACHED();
  }
  return nullptr;
}

bool CSSImageGeneratorValue::IsFixedSize() const {
  switch (GetClassType()) {
    case kCrossfadeClass:
      return ToCSSCrossfadeValue(this)->IsFixedSize();
    case kLinearGradientClass:
      return ToCSSLinearGradientValue(this)->IsFixedSize();
    case kPaintClass:
      return ToCSSPaintValue(this)->IsFixedSize();
    case kRadialGradientClass:
      return ToCSSRadialGradientValue(this)->IsFixedSize();
    case kConicGradientClass:
      return ToCSSConicGradientValue(this)->IsFixedSize();
    default:
      NOTREACHED();
  }
  return false;
}

FloatSize CSSImageGeneratorValue::FixedSize(
    const Document& document,
    const FloatSize& default_object_size) {
  switch (GetClassType()) {
    case kCrossfadeClass:
      return ToCSSCrossfadeValue(this)->FixedSize(document,
                                                  default_object_size);
    case kLinearGradientClass:
      return ToCSSLinearGradientValue(this)->FixedSize(document);
    case kPaintClass:
      return ToCSSPaintValue(this)->FixedSize(document);
    case kRadialGradientClass:
      return ToCSSRadialGradientValue(this)->FixedSize(document);
    case kConicGradientClass:
      return ToCSSConicGradientValue(this)->FixedSize(document);
    default:
      NOTREACHED();
  }
  return FloatSize();
}

bool CSSImageGeneratorValue::IsPending() const {
  switch (GetClassType()) {
    case kCrossfadeClass:
      return ToCSSCrossfadeValue(this)->IsPending();
    case kLinearGradientClass:
      return ToCSSLinearGradientValue(this)->IsPending();
    case kPaintClass:
      return ToCSSPaintValue(this)->IsPending();
    case kRadialGradientClass:
      return ToCSSRadialGradientValue(this)->IsPending();
    case kConicGradientClass:
      return ToCSSConicGradientValue(this)->IsPending();
    default:
      NOTREACHED();
  }
  return false;
}

bool CSSImageGeneratorValue::KnownToBeOpaque(const Document& document,
                                             const ComputedStyle& style) const {
  switch (GetClassType()) {
    case kCrossfadeClass:
      return ToCSSCrossfadeValue(this)->KnownToBeOpaque(document, style);
    case kLinearGradientClass:
      return ToCSSLinearGradientValue(this)->KnownToBeOpaque(document, style);
    case kPaintClass:
      return ToCSSPaintValue(this)->KnownToBeOpaque(document, style);
    case kRadialGradientClass:
      return ToCSSRadialGradientValue(this)->KnownToBeOpaque(document, style);
    case kConicGradientClass:
      return ToCSSConicGradientValue(this)->KnownToBeOpaque(document, style);
    default:
      NOTREACHED();
  }
  return false;
}

void CSSImageGeneratorValue::LoadSubimages(const Document& document) {
  switch (GetClassType()) {
    case kCrossfadeClass:
      ToCSSCrossfadeValue(this)->LoadSubimages(document);
      break;
    case kLinearGradientClass:
      ToCSSLinearGradientValue(this)->LoadSubimages(document);
      break;
    case kPaintClass:
      ToCSSPaintValue(this)->LoadSubimages(document);
      break;
    case kRadialGradientClass:
      ToCSSRadialGradientValue(this)->LoadSubimages(document);
      break;
    case kConicGradientClass:
      ToCSSConicGradientValue(this)->LoadSubimages(document);
      break;
    default:
      NOTREACHED();
  }
}

}  // namespace blink
