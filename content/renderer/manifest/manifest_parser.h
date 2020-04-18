// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MANIFEST_MANIFEST_PARSER_H_
#define CONTENT_RENDERER_MANIFEST_MANIFEST_PARSER_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/optional.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/string_piece.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/common/manifest/manifest.h"
#include "third_party/blink/public/mojom/manifest/manifest.mojom.h"
#include "third_party/skia/include/core/SkColor.h"

class GURL;

namespace base {
class DictionaryValue;
}

namespace content {

// ManifestParser handles the logic of parsing the Web Manifest from a string.
// It implements:
// http://w3c.github.io/manifest/#dfn-steps-for-processing-a-manifest
class CONTENT_EXPORT ManifestParser {
 public:
  ManifestParser(const base::StringPiece& data,
                 const GURL& manifest_url,
                 const GURL& document_url);
  ~ManifestParser();

  // Parse the Manifest from a string using following:
  // http://w3c.github.io/manifest/#dfn-steps-for-processing-a-manifest
  void Parse();

  const blink::Manifest& manifest() const;
  bool failed() const;

  void TakeErrors(std::vector<blink::mojom::ManifestErrorPtr>* errors);

 private:
  // Used to indicate whether to strip whitespace when parsing a string.
  enum TrimType {
    Trim,
    NoTrim
  };

  // Indicate whether a parsed URL should be restricted to document origin.
  enum class ParseURLOriginRestrictions {
    kNoRestrictions = 0,
    kSameOriginOnly,
  };

  // Helper function to parse booleans present on a given |dictionary| in a
  // given field identified by its |key|.
  // Returns the parsed boolean if any, or |default_value| if parsing failed.
  bool ParseBoolean(const base::DictionaryValue& dictionary,
                    const std::string& key,
                    bool default_value);

  // Helper function to parse strings present on a given |dictionary| in a given
  // field identified by its |key|.
  // Returns the parsed string if any, a null string if the parsing failed.
  base::NullableString16 ParseString(const base::DictionaryValue& dictionary,
                                     const std::string& key,
                                     TrimType trim);

  // Helper function to parse colors present on a given |dictionary| in a given
  // field identified by its |key|. Returns a null optional if the value is not
  // present or is not a valid color.
  base::Optional<SkColor> ParseColor(const base::DictionaryValue& dictionary,
                                     const std::string& key);

  // Helper function to parse URLs present on a given |dictionary| in a given
  // field identified by its |key|. The URL is first parsed as a string then
  // resolved using |base_url|. |enforce_document_origin| specified whether to
  // enforce matching of the document's and parsed URL's origins.
  // Returns a GURL. If the parsing failed or origin matching was enforced but
  // not present, the returned GURL will be empty.
  GURL ParseURL(const base::DictionaryValue& dictionary,
                const std::string& key,
                const GURL& base_url,
                ParseURLOriginRestrictions origin_restriction);

  // Parses the 'name' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-name-member
  // Returns the parsed string if any, a null string if the parsing failed.
  base::NullableString16 ParseName(const base::DictionaryValue& dictionary);

  // Parses the 'short_name' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-short-name-member
  // Returns the parsed string if any, a null string if the parsing failed.
  base::NullableString16 ParseShortName(
      const base::DictionaryValue& dictionary);

  // Parses the 'scope' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-scope-member
  // Returns the parsed GURL if any, an empty GURL if the parsing failed.
  GURL ParseScope(const base::DictionaryValue& dictionary,
                  const GURL& start_url);

  // Parses the 'start_url' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-start_url-member
  // Returns the parsed GURL if any, an empty GURL if the parsing failed.
  GURL ParseStartURL(const base::DictionaryValue& dictionary);

  // Parses the 'display' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-display-member
  // Returns the parsed DisplayMode if any, WebDisplayModeUndefined if the
  // parsing failed.
  blink::WebDisplayMode ParseDisplay(const base::DictionaryValue& dictionary);

  // Parses the 'orientation' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-orientation-member
  // Returns the parsed WebScreenOrientationLockType if any,
  // WebScreenOrientationLockDefault if the parsing failed.
  blink::WebScreenOrientationLockType ParseOrientation(
      const base::DictionaryValue& dictionary);

  // Parses the 'src' field of an icon, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-src-member-of-an-image
  // Returns the parsed GURL if any, an empty GURL if the parsing failed.
  GURL ParseIconSrc(const base::DictionaryValue& icon);

  // Parses the 'type' field of an icon, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-type-member-of-an-image
  // Returns the parsed string if any, an empty string if the parsing failed.
  base::string16 ParseIconType(const base::DictionaryValue& icon);

  // Parses the 'sizes' field of an icon, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-a-sizes-member-of-an-image
  // Returns a vector of gfx::Size with the successfully parsed sizes, if any.
  // An empty vector if the field was not present or empty. "Any" is represented
  // by gfx::Size(0, 0).
  std::vector<gfx::Size> ParseIconSizes(const base::DictionaryValue& icon);

  // Parses the 'purpose' field of an icon, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-a-purpose-member-of-an-image
  // Returns a vector of Manifest::Icon::IconPurpose with the successfully
  // parsed icon purposes, and a vector with Manifest::Icon::IconPurpose::Any if
  // the parsing failed.
  std::vector<blink::Manifest::Icon::IconPurpose> ParseIconPurpose(
      const base::DictionaryValue& icon);

  // Parses the 'icons' field of a Manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-an-array-of-images
  // Returns a vector of Manifest::Icon with the successfully parsed icons, if
  // any. An empty vector if the field was not present or empty.
  std::vector<blink::Manifest::Icon> ParseIcons(
      const base::DictionaryValue& dictionary);

  // Parses the 'url_template' field of a Share Target, as defined in:
  // https://github.com/WICG/web-share-target/blob/master/docs/interface.md
  // Returns the parsed GURL if any, or an empty GURL if the parsing failed.
  GURL ParseShareTargetURLTemplate(const base::DictionaryValue& share_target);

  // Parses the 'share_target' field of a Manifest, as defined in:
  // https://github.com/WICG/web-share-target/blob/master/docs/interface.md
  // Returns the parsed Web Share target. The returned Share Target is null if
  // the field didn't exist, parsing failed, or it was empty.
  base::Optional<blink::Manifest::ShareTarget> ParseShareTarget(
      const base::DictionaryValue& dictionary);

  // Parses the 'platform' field of a related application, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-platform-member-of-an-application
  // Returns the parsed string if any, a null string if the parsing failed.
  base::NullableString16 ParseRelatedApplicationPlatform(
      const base::DictionaryValue& application);

  // Parses the 'url' field of a related application, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-url-member-of-an-application
  // Returns the parsed GURL if any, an empty GURL if the parsing failed.
  GURL ParseRelatedApplicationURL(const base::DictionaryValue& application);

  // Parses the 'id' field of a related application, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-id-member-of-an-application
  // Returns the parsed string if any, a null string if the parsing failed.
  base::NullableString16 ParseRelatedApplicationId(
      const base::DictionaryValue& application);

  // Parses the 'related_applications' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-related_applications-member
  // Returns a vector of Manifest::RelatedApplication with the successfully
  // parsed applications, if any. An empty vector if the field was not present
  // or empty.
  std::vector<blink::Manifest::RelatedApplication> ParseRelatedApplications(
      const base::DictionaryValue& dictionary);

  // Parses the 'prefer_related_applications' field on the manifest, as defined
  // in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-prefer_related_applications-member
  // returns true iff the field could be parsed as the boolean true.
  bool ParsePreferRelatedApplications(const base::DictionaryValue& dictionary);

  // Parses the 'theme_color' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-theme_color-member
  // Returns the parsed theme color if any, or a null optional otherwise.
  base::Optional<SkColor> ParseThemeColor(
      const base::DictionaryValue& dictionary);

  // Parses the 'background_color' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-background_color-member
  // Returns the parsed background color if any, or a null optional otherwise.
  base::Optional<SkColor> ParseBackgroundColor(
      const base::DictionaryValue& dictionary);

  // Parses the 'splash_screen_url' field of the manifest.
  // Returns the parsed GURL if any, an empty GURL if the parsing failed.
  GURL ParseSplashScreenURL(const base::DictionaryValue& dictionary);

  // Parses the 'gcm_sender_id' field of the manifest.
  // This is a proprietary extension of the Web Manifest specification.
  // Returns the parsed string if any, a null string if the parsing failed.
  base::NullableString16 ParseGCMSenderID(
      const base::DictionaryValue& dictionary);

  void AddErrorInfo(const std::string& error_msg,
                    bool critical = false,
                    int error_line = 0,
                    int error_column = 0);

  const base::StringPiece& data_;
  GURL manifest_url_;
  GURL document_url_;

  bool failed_;
  blink::Manifest manifest_;
  std::vector<blink::mojom::ManifestErrorPtr> errors_;

  DISALLOW_COPY_AND_ASSIGN(ManifestParser);
};

} // namespace content

#endif  // CONTENT_RENDERER_MANIFEST_MANIFEST_PARSER_H_
