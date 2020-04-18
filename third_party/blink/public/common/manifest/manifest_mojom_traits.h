// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_MANIFEST_MANIFEST_MOJOM_TRAITS_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_MANIFEST_MANIFEST_MOJOM_TRAITS_H_

#include "third_party/blink/public/common/manifest/manifest.h"

#include "mojo/public/cpp/bindings/struct_traits.h"
#include "third_party/blink/common/common_export.h"
#include "third_party/blink/public/mojom/manifest/manifest.mojom.h"

namespace mojo {
namespace internal {

inline base::StringPiece16 TruncateString16(const base::string16& string) {
  // We restrict the maximum length for all the strings inside the Manifest
  // when it is sent over Mojo. The renderer process truncates the strings
  // before sending the Manifest and the browser process validates that.
  return base::StringPiece16(string).substr(0, 4 * 1024);
}

inline base::Optional<base::StringPiece16> TruncateNullableString16(
    const base::NullableString16& string) {
  if (string.is_null())
    return base::nullopt;

  return TruncateString16(string.string());
}

}  // namespace internal

template <>
struct BLINK_COMMON_EXPORT
    StructTraits<blink::mojom::ManifestDataView, ::blink::Manifest> {
  static bool IsNull(const ::blink::Manifest& manifest) {
    return manifest.IsEmpty();
  }

  static void SetToNull(::blink::Manifest* manifest) {
    *manifest = ::blink::Manifest();
  }

  static base::Optional<base::StringPiece16> name(
      const ::blink::Manifest& manifest) {
    return internal::TruncateNullableString16(manifest.name);
  }

  static base::Optional<base::StringPiece16> short_name(
      const ::blink::Manifest& manifest) {
    return internal::TruncateNullableString16(manifest.short_name);
  }

  static base::Optional<base::StringPiece16> gcm_sender_id(
      const ::blink::Manifest& manifest) {
    return internal::TruncateNullableString16(manifest.gcm_sender_id);
  }

  static const GURL& start_url(const ::blink::Manifest& manifest) {
    return manifest.start_url;
  }

  static const GURL& scope(const ::blink::Manifest& manifest) {
    return manifest.scope;
  }

  static blink::WebDisplayMode display(const ::blink::Manifest& manifest) {
    return manifest.display;
  }

  static blink::WebScreenOrientationLockType orientation(
      const ::blink::Manifest& manifest) {
    return manifest.orientation;
  }

  static bool has_theme_color(const ::blink::Manifest& m) {
    return m.theme_color.has_value();
  }

  static uint32_t theme_color(const ::blink::Manifest& m) {
    return m.theme_color.value_or(0);
  }

  static bool has_background_color(const ::blink::Manifest& m) {
    return m.background_color.has_value();
  }

  static uint32_t background_color(const ::blink::Manifest& m) {
    return m.background_color.value_or(0);
  }

  static const GURL& splash_screen_url(const ::blink::Manifest& manifest) {
    return manifest.splash_screen_url;
  }

  static const std::vector<::blink::Manifest::Icon>& icons(
      const ::blink::Manifest& manifest) {
    return manifest.icons;
  }

  static const base::Optional<::blink::Manifest::ShareTarget>& share_target(
      const ::blink::Manifest& manifest) {
    return manifest.share_target;
  }

  static const std::vector<::blink::Manifest::RelatedApplication>&
  related_applications(const ::blink::Manifest& manifest) {
    return manifest.related_applications;
  }

  static bool prefer_related_applications(const ::blink::Manifest& manifest) {
    return manifest.prefer_related_applications;
  }

  static bool Read(blink::mojom::ManifestDataView data, ::blink::Manifest* out);
};

template <>
struct BLINK_COMMON_EXPORT
    StructTraits<blink::mojom::ManifestIconDataView, ::blink::Manifest::Icon> {
  static const GURL& src(const ::blink::Manifest::Icon& icon) {
    return icon.src;
  }

  static base::StringPiece16 type(const ::blink::Manifest::Icon& icon) {
    return internal::TruncateString16(icon.type);
  }
  static const std::vector<gfx::Size>& sizes(
      const ::blink::Manifest::Icon& icon) {
    return icon.sizes;
  }

  static const std::vector<::blink::Manifest::Icon::IconPurpose>& purpose(
      const ::blink::Manifest::Icon& icon) {
    return icon.purpose;
  }

  static bool Read(blink::mojom::ManifestIconDataView data,
                   ::blink::Manifest::Icon* out);
};

template <>
struct BLINK_COMMON_EXPORT
    StructTraits<blink::mojom::ManifestRelatedApplicationDataView,
                 ::blink::Manifest::RelatedApplication> {
  static base::Optional<base::StringPiece16> platform(
      const ::blink::Manifest::RelatedApplication& related_application) {
    return internal::TruncateNullableString16(related_application.platform);
  }

  static const GURL& url(
      const ::blink::Manifest::RelatedApplication& related_application) {
    return related_application.url;
  }

  static base::Optional<base::StringPiece16> id(
      const ::blink::Manifest::RelatedApplication& related_application) {
    return internal::TruncateNullableString16(related_application.id);
  }

  static bool Read(blink::mojom::ManifestRelatedApplicationDataView data,
                   ::blink::Manifest::RelatedApplication* out);
};

template <>
struct BLINK_COMMON_EXPORT
    StructTraits<blink::mojom::ManifestShareTargetDataView,
                 ::blink::Manifest::ShareTarget> {
  static const GURL& url_template(
      const ::blink::Manifest::ShareTarget& share_target) {
    return share_target.url_template;
  }
  static bool Read(blink::mojom::ManifestShareTargetDataView data,
                   ::blink::Manifest::ShareTarget* out);
};

template <>
struct BLINK_COMMON_EXPORT EnumTraits<blink::mojom::ManifestIcon_Purpose,
                                      ::blink::Manifest::Icon::IconPurpose> {
  static blink::mojom::ManifestIcon_Purpose ToMojom(
      ::blink::Manifest::Icon::IconPurpose purpose) {
    switch (purpose) {
      case ::blink::Manifest::Icon::ANY:
        return blink::mojom::ManifestIcon_Purpose::ANY;
      case ::blink::Manifest::Icon::BADGE:
        return blink::mojom::ManifestIcon_Purpose::BADGE;
    }
    NOTREACHED();
    return blink::mojom::ManifestIcon_Purpose::ANY;
  }
  static bool FromMojom(blink::mojom::ManifestIcon_Purpose input,
                        ::blink::Manifest::Icon::IconPurpose* out) {
    switch (input) {
      case blink::mojom::ManifestIcon_Purpose::ANY:
        *out = ::blink::Manifest::Icon::ANY;
        return true;
      case blink::mojom::ManifestIcon_Purpose::BADGE:
        *out = ::blink::Manifest::Icon::BADGE;
        return true;
    }

    return false;
  }
};

}  // namespace mojo

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_MANIFEST_MANIFEST_MOJOM_TRAITS_H_
