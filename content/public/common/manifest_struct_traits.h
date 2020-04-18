// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_MANIFEST_STRUCT_TRAITS_H_
#define CONTENT_PUBLIC_COMMON_MANIFEST_STRUCT_TRAITS_H_

#include "content/public/common/manifest.h"

#include "mojo/public/cpp/bindings/struct_traits.h"
#include "third_party/blink/public/platform/modules/manifest/manifest.mojom-shared.h"

namespace mojo {
namespace internal {

inline base::StringPiece16 TruncateString16(const base::string16& string) {
  return base::StringPiece16(string).substr(
      0, content::Manifest::kMaxIPCStringLength);
}

inline base::Optional<base::StringPiece16> TruncateNullableString16(
    const base::NullableString16& string) {
  if (string.is_null())
    return base::nullopt;

  return TruncateString16(string.string());
}

}  // namespace internal

template <>
struct StructTraits<blink::mojom::ManifestDataView, content::Manifest> {
  static bool IsNull(const content::Manifest& m) { return m.IsEmpty(); }

  static void SetToNull(content::Manifest* m) { *m = content::Manifest(); }

  static base::Optional<base::StringPiece16> name(const content::Manifest& m) {
    return internal::TruncateNullableString16(m.name);
  }

  static base::Optional<base::StringPiece16> short_name(
      const content::Manifest& m) {
    return internal::TruncateNullableString16(m.short_name);
  }

  static base::Optional<base::StringPiece16> gcm_sender_id(
      const content::Manifest& m) {
    return internal::TruncateNullableString16(m.gcm_sender_id);
  }

  static const GURL& start_url(const content::Manifest& m) {
    return m.start_url;
  }

  static const GURL& scope(const content::Manifest& m) { return m.scope; }

  static blink::WebDisplayMode display(const content::Manifest& m) {
    return m.display;
  }

  static blink::WebScreenOrientationLockType orientation(
      const content::Manifest& m) {
    return m.orientation;
  }

  static bool has_theme_color(const content::Manifest& m) {
    return m.theme_color.has_value();
  }

  static uint32_t theme_color(const content::Manifest& m) {
    return m.theme_color.value_or(0);
  }

  static bool has_background_color(const content::Manifest& m) {
    return m.background_color.has_value();
  }

  static uint32_t background_color(const content::Manifest& m) {
    return m.background_color.value_or(0);
  }

  static const GURL& splash_screen_url(const content::Manifest& m) {
    return m.splash_screen_url;
  }

  static const std::vector<content::Manifest::Icon>& icons(
      const content::Manifest& m) {
    return m.icons;
  }

  static const base::Optional<content::Manifest::ShareTarget>& share_target(
      const content::Manifest& m) {
    return m.share_target;
  }

  static const std::vector<content::Manifest::RelatedApplication>&
  related_applications(const content::Manifest& m) {
    return m.related_applications;
  }

  static bool prefer_related_applications(const content::Manifest& m) {
    return m.prefer_related_applications;
  }

  static bool Read(blink::mojom::ManifestDataView data, content::Manifest* out);
};

template <>
struct StructTraits<blink::mojom::ManifestIconDataView,
                    content::Manifest::Icon> {
  static const GURL& src(const content::Manifest::Icon& m) { return m.src; }

  static base::StringPiece16 type(const content::Manifest::Icon& m) {
    return internal::TruncateString16(m.type);
  }
  static const std::vector<gfx::Size>& sizes(const content::Manifest::Icon& m) {
    return m.sizes;
  }

  static const std::vector<content::Manifest::Icon::IconPurpose>& purpose(
      const content::Manifest::Icon& m) {
    return m.purpose;
  }

  static bool Read(blink::mojom::ManifestIconDataView data,
                   content::Manifest::Icon* out);
};

template <>
struct StructTraits<blink::mojom::ManifestRelatedApplicationDataView,
                    content::Manifest::RelatedApplication> {
  static base::Optional<base::StringPiece16> platform(
      const content::Manifest::RelatedApplication& m) {
    return internal::TruncateNullableString16(m.platform);
  }

  static const GURL& url(const content::Manifest::RelatedApplication& m) {
    return m.url;
  }

  static base::Optional<base::StringPiece16> id(
      const content::Manifest::RelatedApplication& m) {
    return internal::TruncateNullableString16(m.id);
  }

  static bool Read(blink::mojom::ManifestRelatedApplicationDataView data,
                   content::Manifest::RelatedApplication* out);
};

template <>
struct StructTraits<blink::mojom::ManifestShareTargetDataView,
                    content::Manifest::ShareTarget> {
  static const GURL& url_template(const content::Manifest::ShareTarget& m) {
    return m.url_template;
  }
  static bool Read(blink::mojom::ManifestShareTargetDataView data,
                   content::Manifest::ShareTarget* out);
};

template <>
struct EnumTraits<blink::mojom::ManifestIcon_Purpose,
                  content::Manifest::Icon::IconPurpose> {
  static blink::mojom::ManifestIcon_Purpose ToMojom(
      content::Manifest::Icon::IconPurpose purpose) {
    switch (purpose) {
      case content::Manifest::Icon::ANY:
        return blink::mojom::ManifestIcon_Purpose::ANY;
      case content::Manifest::Icon::BADGE:
        return blink::mojom::ManifestIcon_Purpose::BADGE;
    }
    NOTREACHED();
    return blink::mojom::ManifestIcon_Purpose::ANY;
  }
  static bool FromMojom(blink::mojom::ManifestIcon_Purpose input,
                        content::Manifest::Icon::IconPurpose* out) {
    switch (input) {
      case blink::mojom::ManifestIcon_Purpose::ANY:
        *out = content::Manifest::Icon::ANY;
        return true;
      case blink::mojom::ManifestIcon_Purpose::BADGE:
        *out = content::Manifest::Icon::BADGE;
        return true;
    }

    return false;
  }
};

}  // namespace mojo

#endif  // CONTENT_PUBLIC_COMMON_MANIFEST_STRUCT_TRAITS_H_
