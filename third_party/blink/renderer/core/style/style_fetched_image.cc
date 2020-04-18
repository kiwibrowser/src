/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "third_party/blink/renderer/core/style/style_fetched_image.h"

#include "third_party/blink/renderer/core/css/css_image_value.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/loader/resource/image_resource_content.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/core/svg/graphics/svg_image.h"
#include "third_party/blink/renderer/core/svg/graphics/svg_image_for_container.h"
#include "third_party/blink/renderer/platform/geometry/layout_size.h"

namespace blink {

StyleFetchedImage::StyleFetchedImage(const Document& document,
                                     FetchParameters& params)
    : document_(&document), url_(params.Url()) {
  is_image_resource_ = true;
  image_ = ImageResourceContent::Fetch(params, document_->Fetcher());
  image_->AddObserver(this);
  // ResourceFetcher is not determined from StyleFetchedImage and it is
  // impossible to send a request for refetching.
  image_->SetNotRefetchableDataFromDiskCache();
}

StyleFetchedImage::~StyleFetchedImage() = default;

void StyleFetchedImage::Dispose() {
  image_->RemoveObserver(this);
  image_ = nullptr;
}

WrappedImagePtr StyleFetchedImage::Data() const {
  return image_.Get();
}

ImageResourceContent* StyleFetchedImage::CachedImage() const {
  return image_.Get();
}

CSSValue* StyleFetchedImage::CssValue() const {
  return CSSImageValue::Create(url_, const_cast<StyleFetchedImage*>(this));
}

CSSValue* StyleFetchedImage::ComputedCSSValue() const {
  return CssValue();
}

bool StyleFetchedImage::CanRender() const {
  return !image_->ErrorOccurred() && !image_->GetImage()->IsNull();
}

bool StyleFetchedImage::IsLoaded() const {
  return image_->IsLoaded();
}

bool StyleFetchedImage::ErrorOccurred() const {
  return image_->ErrorOccurred();
}

FloatSize StyleFetchedImage::ImageSize(
    const Document&,
    float multiplier,
    const LayoutSize& default_object_size) const {
  Image* image = image_->GetImage();
  if (image->IsSVGImage()) {
    return ImageSizeForSVGImage(ToSVGImage(image), multiplier,
                                default_object_size);
  }
  // Image orientation should only be respected for content images,
  // not decorative images such as StyleImage (backgrounds,
  // border-image, etc.)
  //
  // https://drafts.csswg.org/css-images-3/#the-image-orientation
  FloatSize size(image->Size());
  return ApplyZoom(size, multiplier);
}

bool StyleFetchedImage::ImageHasRelativeSize() const {
  return image_->GetImage()->HasRelativeSize();
}

bool StyleFetchedImage::UsesImageContainerSize() const {
  return image_->GetImage()->UsesContainerSize();
}

void StyleFetchedImage::AddClient(ImageResourceObserver* observer) {
  image_->AddObserver(observer);
}

void StyleFetchedImage::RemoveClient(ImageResourceObserver* observer) {
  image_->RemoveObserver(observer);
}

void StyleFetchedImage::ImageNotifyFinished(ImageResourceContent*) {
  if (image_ && image_->HasImage()) {
    Image& image = *image_->GetImage();

    if (document_ && image.IsSVGImage())
      ToSVGImage(image).UpdateUseCounters(*document_);
  }

  // Oilpan: do not prolong the Document's lifetime.
  document_.Clear();
}

scoped_refptr<Image> StyleFetchedImage::GetImage(
    const ImageResourceObserver&,
    const Document&,
    const ComputedStyle& style,
    const FloatSize& target_size) const {
  Image* image = image_->GetImage();
  if (!image->IsSVGImage())
    return image;
  return SVGImageForContainer::Create(ToSVGImage(image), target_size,
                                      style.EffectiveZoom(), url_);
}

bool StyleFetchedImage::KnownToBeOpaque(const Document&,
                                        const ComputedStyle&) const {
  return image_->GetImage()->CurrentFrameKnownToBeOpaque();
}

void StyleFetchedImage::Trace(blink::Visitor* visitor) {
  visitor->Trace(image_);
  visitor->Trace(document_);
  StyleImage::Trace(visitor);
}

}  // namespace blink
