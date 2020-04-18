// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_MANIFEST_MANIFEST_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_MANIFEST_MANIFEST_H_

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "base/optional.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/string16.h"
#include "third_party/blink/common/common_export.h"
#include "third_party/blink/public/common/manifest/web_display_mode.h"
#include "third_party/blink/public/common/screen_orientation/web_screen_orientation_lock_type.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

namespace blink {

// The Manifest structure is an internal representation of the Manifest file
// described in the "Manifest for Web Application" document:
// http://w3c.github.io/manifest/
struct BLINK_COMMON_EXPORT Manifest {
  // Structure representing an icon as per the Manifest specification, see:
  // http://w3c.github.io/manifest/#dfn-icon-object
  struct BLINK_COMMON_EXPORT Icon {
    enum IconPurpose {
      ANY = 0,
      BADGE,
      ICON_PURPOSE_LAST = BADGE,
    };

    Icon();
    Icon(const Icon& other);
    ~Icon();

    bool operator==(const Icon& other) const;

    // MUST be a valid url. If an icon doesn't have a valid URL, it will not be
    // successfully parsed, thus will not be represented in the Manifest.
    GURL src;

    // Empty if the parsing failed or the field was not present. The type can be
    // any string and doesn't have to be a valid image MIME type at this point.
    // It is up to the consumer of the object to check if the type matches a
    // supported type.
    base::string16 type;

    // Empty if the parsing failed, the field was not present or empty.
    // The special value "any" is represented by gfx::Size(0, 0).
    std::vector<gfx::Size> sizes;

    // Empty if the field was not present or not of type "string". Defaults to
    // a vector with a single value, IconPurpose::ANY, for all other parsing
    // exceptions.
    std::vector<IconPurpose> purpose;
  };

  // Structure representing how a Web Share target handles an incoming share.
  struct BLINK_COMMON_EXPORT ShareTarget {
    ShareTarget();
    ~ShareTarget();

    // The URL template that contains placeholders to be replaced with shared
    // data. Empty if the parsing failed.
    GURL url_template;
  };

  // Structure representing a related application.
  struct BLINK_COMMON_EXPORT RelatedApplication {
    RelatedApplication();
    ~RelatedApplication();

    // The platform on which the application can be found. This can be any
    // string, and is interpreted by the consumer of the object. Empty if the
    // parsing failed.
    base::NullableString16 platform;

    // URL at which the application can be found. One of |url| and |id| must be
    // present. Empty if the parsing failed or the field was not present.
    GURL url;

    // An id which is used to represent the application on the platform. One of
    // |url| and |id| must be present. Empty if the parsing failed or the field
    // was not present.
    base::NullableString16 id;
  };

  Manifest();
  Manifest(const Manifest& other);
  ~Manifest();

  // Returns whether this Manifest had no attribute set. A newly created
  // Manifest is always empty.
  bool IsEmpty() const;

  // Null if the parsing failed or the field was not present.
  base::NullableString16 name;

  // Null if the parsing failed or the field was not present.
  base::NullableString16 short_name;

  // Empty if the parsing failed or the field was not present.
  GURL start_url;

  // Set to WebDisplayModeUndefined if the parsing failed or the field was not
  // present.
  blink::WebDisplayMode display;

  // Set to blink::WebScreenOrientationLockDefault if the parsing failed or the
  // field was not present.
  blink::WebScreenOrientationLockType orientation;

  // Empty if the parsing failed, the field was not present, empty or all the
  // icons inside the JSON array were invalid.
  std::vector<Icon> icons;

  // Null if parsing failed or the field was not present.
  // TODO(constantina): This field is non-standard and part of a Chrome
  // experiment. See:
  // https://github.com/WICG/web-share-target/blob/master/docs/interface.md
  // As such, this field should not be exposed to web contents.
  base::Optional<ShareTarget> share_target;

  // Empty if the parsing failed, the field was not present, empty or all the
  // applications inside the array were invalid. The order of the array
  // indicates the priority of the application to use.
  std::vector<RelatedApplication> related_applications;

  // A boolean that is used as a hint for the user agent to say that related
  // applications should be preferred over the web application. False if missing
  // or there is a parsing failure.
  bool prefer_related_applications;

  // Null if field is not present or parsing failed.
  base::Optional<SkColor> theme_color;

  // Null if field is not present or parsing failed.
  base::Optional<SkColor> background_color;

  // A URL of the HTML splash screen.
  // Empty if the parsing failed or the field was not present.
  GURL splash_screen_url;

  // This is a proprietary extension of the web Manifest, double-check that it
  // is okay to use this entry.
  // Null if parsing failed or the field was not present.
  base::NullableString16 gcm_sender_id;

  // Empty if the parsing failed or the field was not present.
  GURL scope;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_MANIFEST_MANIFEST_H_
