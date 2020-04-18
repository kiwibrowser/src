/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008, 2012 Apple Inc. All rights reserved.
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
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_IMAGE_VALUE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_IMAGE_VALUE_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/css/css_value.h"
#include "third_party/blink/renderer/platform/cross_origin_attribute_value.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"
#include "third_party/blink/renderer/platform/weborigin/referrer.h"

namespace blink {

class Document;
class KURL;
class StyleImage;
class ComputedStyle;

class CORE_EXPORT CSSImageValue : public CSSValue {
 public:
  static CSSImageValue* Create(const KURL& url, StyleImage* image = nullptr) {
    return Create(url.GetString(), url, Referrer(), image);
  }
  static CSSImageValue* Create(const AtomicString& relative_url,
                               const KURL& absolute_url,
                               StyleImage* image = nullptr) {
    return Create(relative_url, absolute_url, Referrer(), image);
  }
  static CSSImageValue* Create(const String& raw_value,
                               const KURL& url,
                               const Referrer& referrer,
                               StyleImage* image = nullptr) {
    return Create(AtomicString(raw_value), url, referrer, image);
  }
  static CSSImageValue* Create(const AtomicString& raw_value,
                               const KURL& url,
                               const Referrer& referrer,
                               StyleImage* image = nullptr) {
    return new CSSImageValue(raw_value, url, referrer, image);
  }
  static CSSImageValue* Create(const AtomicString& absolute_url) {
    return new CSSImageValue(absolute_url);
  }
  ~CSSImageValue();

  bool IsCachePending() const { return !cached_image_; }
  StyleImage* CachedImage() const {
    DCHECK(!IsCachePending());
    return cached_image_.Get();
  }
  StyleImage* CacheImage(
      const Document&,
      FetchParameters::PlaceholderImageRequestType,
      CrossOriginAttributeValue = kCrossOriginAttributeNotSet);

  const String& Url() const { return absolute_url_; }
  const String& RelativeUrl() const { return relative_url_; }

  const Referrer& GetReferrer() const { return referrer_; }

  void ReResolveURL(const Document&) const;

  String CustomCSSText() const;

  bool HasFailedOrCanceledSubresources() const;

  bool Equals(const CSSImageValue&) const;

  bool KnownToBeOpaque(const Document&, const ComputedStyle&) const;

  CSSImageValue* ValueWithURLMadeAbsolute() const {
    return Create(KURL(absolute_url_), cached_image_.Get());
  }

  CSSImageValue* Clone() const {
    return Create(relative_url_, KURL(absolute_url_), cached_image_.Get());
  }

  void SetInitiator(const AtomicString& name) { initiator_name_ = name; }

  void TraceAfterDispatch(blink::Visitor*);
  void RestoreCachedResourceIfNeeded(const Document&) const;

 private:
  CSSImageValue(const AtomicString& raw_value,
                const KURL&,
                const Referrer&,
                StyleImage*);
  CSSImageValue(const AtomicString& absolute_url);

  AtomicString relative_url_;
  Referrer referrer_;
  AtomicString initiator_name_;

  // Cached image data.
  mutable AtomicString absolute_url_;
  mutable Member<StyleImage> cached_image_;
};

DEFINE_CSS_VALUE_TYPE_CASTS(CSSImageValue, IsImageValue());

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_IMAGE_VALUE_H_
