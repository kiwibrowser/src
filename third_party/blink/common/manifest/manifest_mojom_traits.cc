// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/manifest/manifest_mojom_traits.h"

#include "mojo/public/cpp/base/string16_mojom_traits.h"
#include "third_party/blink/public/common/manifest/web_display_mode_mojom_traits.h"
#include "third_party/blink/public/common/screen_orientation/web_screen_orientation_enum_traits.h"
#include "ui/gfx/geometry/mojo/geometry_struct_traits.h"
#include "url/mojom/url_gurl_mojom_traits.h"

namespace mojo {
namespace {

// A wrapper around base::Optional<base::string16> so a custom StructTraits
// specialization can enforce maximum string length.
struct TruncatedString16 {
  base::Optional<base::string16> string;
};

}  // namespace

template <>
struct StructTraits<mojo_base::mojom::String16DataView, TruncatedString16> {
  static void SetToNull(TruncatedString16* output) { output->string.reset(); }

  static bool Read(mojo_base::mojom::String16DataView input,
                   TruncatedString16* output) {
    if (input.is_null()) {
      output->string.reset();
      return true;
    }
    mojo::ArrayDataView<uint16_t> buffer_view;
    input.GetDataDataView(&buffer_view);
    if (buffer_view.size() > 4 * 1024)
      return false;

    output->string.emplace();
    return StructTraits<mojo_base::mojom::String16DataView,
                        base::string16>::Read(input, &output->string.value());
  }
};

bool StructTraits<blink::mojom::ManifestDataView, ::blink::Manifest>::Read(
    blink::mojom::ManifestDataView data,
    ::blink::Manifest* out) {
  TruncatedString16 string;
  if (!data.ReadName(&string))
    return false;
  out->name = base::NullableString16(std::move(string.string));

  if (!data.ReadShortName(&string))
    return false;
  out->short_name = base::NullableString16(std::move(string.string));

  if (!data.ReadGcmSenderId(&string))
    return false;
  out->gcm_sender_id = base::NullableString16(std::move(string.string));

  if (!data.ReadStartUrl(&out->start_url))
    return false;

  if (!data.ReadIcons(&out->icons))
    return false;

  if (!data.ReadShareTarget(&out->share_target))
    return false;

  if (!data.ReadRelatedApplications(&out->related_applications))
    return false;

  out->prefer_related_applications = data.prefer_related_applications();

  if (data.has_theme_color())
    out->theme_color = data.theme_color();

  if (data.has_background_color())
    out->background_color = data.background_color();

  if (!data.ReadSplashScreenUrl(&out->splash_screen_url))
    return false;

  if (!data.ReadDisplay(&out->display))
    return false;

  if (!data.ReadOrientation(&out->orientation))
    return false;

  if (!data.ReadScope(&out->scope))
    return false;

  return true;
}

bool StructTraits<blink::mojom::ManifestIconDataView, ::blink::Manifest::Icon>::
    Read(blink::mojom::ManifestIconDataView data,
         ::blink::Manifest::Icon* out) {
  if (!data.ReadSrc(&out->src))
    return false;

  TruncatedString16 string;
  if (!data.ReadType(&string))
    return false;

  if (!string.string)
    return false;

  out->type = *std::move(string.string);

  if (!data.ReadSizes(&out->sizes))
    return false;

  if (!data.ReadPurpose(&out->purpose))
    return false;

  return true;
}

bool StructTraits<blink::mojom::ManifestRelatedApplicationDataView,
                  ::blink::Manifest::RelatedApplication>::
    Read(blink::mojom::ManifestRelatedApplicationDataView data,
         ::blink::Manifest::RelatedApplication* out) {
  TruncatedString16 string;
  if (!data.ReadPlatform(&string))
    return false;
  out->platform = base::NullableString16(std::move(string.string));

  if (!data.ReadUrl(&out->url))
    return false;

  if (!data.ReadId(&string))
    return false;
  out->id = base::NullableString16(std::move(string.string));

  return !(out->url.is_empty() && out->id.is_null());
}

bool StructTraits<blink::mojom::ManifestShareTargetDataView,
                  ::blink::Manifest::ShareTarget>::
    Read(blink::mojom::ManifestShareTargetDataView data,
         ::blink::Manifest::ShareTarget* out) {
  return data.ReadUrlTemplate(&out->url_template);
}

}  // namespace mojo
