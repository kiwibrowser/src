// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/manifest/manifest_parser.h"

#include <stddef.h>

#include "base/json/json_reader.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/common/manifest_util.h"
#include "content/renderer/manifest/manifest_uma_util.h"
#include "third_party/blink/public/common/manifest/manifest_share_target_util.h"
#include "third_party/blink/public/platform/web_icon_sizes_parser.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_css_parser.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/size.h"

namespace content {

ManifestParser::ManifestParser(const base::StringPiece& data,
                               const GURL& manifest_url,
                               const GURL& document_url)
    : data_(data),
      manifest_url_(manifest_url),
      document_url_(document_url),
      failed_(false) {
}

ManifestParser::~ManifestParser() {
}

void ManifestParser::Parse() {
  std::string error_msg;
  int error_line = 0;
  int error_column = 0;
  std::unique_ptr<base::Value> value = base::JSONReader::ReadAndReturnError(
      data_, base::JSON_PARSE_RFC, nullptr, &error_msg, &error_line,
      &error_column);

  if (!value) {
    AddErrorInfo(error_msg, true, error_line, error_column);
    ManifestUmaUtil::ParseFailed();
    failed_ = true;
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    AddErrorInfo("root element must be a valid JSON object.", true);
    ManifestUmaUtil::ParseFailed();
    failed_ = true;
    return;
  }
  DCHECK(dictionary);

  manifest_.name = ParseName(*dictionary);
  manifest_.short_name = ParseShortName(*dictionary);
  manifest_.start_url = ParseStartURL(*dictionary);
  manifest_.scope = ParseScope(*dictionary, manifest_.start_url);
  manifest_.display = ParseDisplay(*dictionary);
  manifest_.orientation = ParseOrientation(*dictionary);
  manifest_.icons = ParseIcons(*dictionary);
  manifest_.share_target = ParseShareTarget(*dictionary);
  manifest_.related_applications = ParseRelatedApplications(*dictionary);
  manifest_.prefer_related_applications =
      ParsePreferRelatedApplications(*dictionary);
  manifest_.theme_color = ParseThemeColor(*dictionary);
  manifest_.background_color = ParseBackgroundColor(*dictionary);
  manifest_.splash_screen_url = ParseSplashScreenURL(*dictionary);
  manifest_.gcm_sender_id = ParseGCMSenderID(*dictionary);

  ManifestUmaUtil::ParseSucceeded(manifest_);
}

const blink::Manifest& ManifestParser::manifest() const {
  return manifest_;
}

void ManifestParser::TakeErrors(
    std::vector<blink::mojom::ManifestErrorPtr>* errors) {
  errors->clear();
  errors->swap(errors_);
}

bool ManifestParser::failed() const {
  return failed_;
}

bool ManifestParser::ParseBoolean(const base::DictionaryValue& dictionary,
                                  const std::string& key,
                                  bool default_value) {
  if (!dictionary.HasKey(key))
    return default_value;

  bool value;
  if (!dictionary.GetBoolean(key, &value)) {
    AddErrorInfo("property '" + key + "' ignored, type " +
                 "boolean expected.");
    return default_value;
  }

  return value;
}

base::NullableString16 ManifestParser::ParseString(
    const base::DictionaryValue& dictionary,
    const std::string& key,
    TrimType trim) {
  if (!dictionary.HasKey(key))
    return base::NullableString16();

  base::string16 value;
  if (!dictionary.GetString(key, &value)) {
    AddErrorInfo("property '" + key + "' ignored, type " +
                 "string expected.");
    return base::NullableString16();
  }

  if (trim == Trim)
    base::TrimWhitespace(value, base::TRIM_ALL, &value);
  return base::NullableString16(value, false);
}

base::Optional<SkColor> ManifestParser::ParseColor(
    const base::DictionaryValue& dictionary,
    const std::string& key) {
  base::NullableString16 parsed_color = ParseString(dictionary, key, Trim);
  if (parsed_color.is_null())
    return base::nullopt;

  SkColor color;
  if (!blink::WebCSSParser::ParseColor(
          &color, blink::WebString::FromUTF16(parsed_color.string()))) {
    AddErrorInfo("property '" + key + "' ignored, '" +
                 base::UTF16ToUTF8(parsed_color.string()) + "' is not a " +
                 "valid color.");
    return base::nullopt;
  }

  return color;
}

GURL ManifestParser::ParseURL(const base::DictionaryValue& dictionary,
                              const std::string& key,
                              const GURL& base_url,
                              ParseURLOriginRestrictions origin_restriction) {
  base::NullableString16 url_str = ParseString(dictionary, key, NoTrim);
  if (url_str.is_null())
    return GURL();

  GURL resolved = base_url.Resolve(url_str.string());
  if (!resolved.is_valid()) {
    AddErrorInfo("property '" + key + "' ignored, URL is invalid.");
    return GURL();
  }

  switch (origin_restriction) {
    case ParseURLOriginRestrictions::kSameOriginOnly:
      if (resolved.GetOrigin() != document_url_.GetOrigin()) {
        AddErrorInfo("property '" + key +
                     "' ignored, should be same origin as document.");
        return GURL();
      }
      return resolved;
    case ParseURLOriginRestrictions::kNoRestrictions:
      return resolved;
  }

  NOTREACHED();
  return GURL();
}

base::NullableString16 ManifestParser::ParseName(
    const base::DictionaryValue& dictionary)  {
  return ParseString(dictionary, "name", Trim);
}

base::NullableString16 ManifestParser::ParseShortName(
    const base::DictionaryValue& dictionary)  {
  return ParseString(dictionary, "short_name", Trim);
}

GURL ManifestParser::ParseStartURL(const base::DictionaryValue& dictionary) {
  return ParseURL(dictionary, "start_url", manifest_url_,
                  ParseURLOriginRestrictions::kSameOriginOnly);
}

GURL ManifestParser::ParseScope(const base::DictionaryValue& dictionary,
                                const GURL& start_url) {
  GURL scope = ParseURL(dictionary, "scope", manifest_url_,
                        ParseURLOriginRestrictions::kSameOriginOnly);

  if (!scope.is_empty()) {
    // According to the spec, if the start_url cannot be parsed, the document
    // URL should be used as the start URL. If the start_url could not be
    // parsed, check that the document URL is within scope.
    GURL check_in_scope = start_url.is_empty() ? document_url_ : start_url;
    if (check_in_scope.GetOrigin() != scope.GetOrigin() ||
        !base::StartsWith(check_in_scope.path(), scope.path(),
                          base::CompareCase::SENSITIVE)) {
      AddErrorInfo(
          "property 'scope' ignored. Start url should be within scope "
          "of scope URL.");
      return GURL();
    }
  }
  return scope;
}

blink::WebDisplayMode ManifestParser::ParseDisplay(
    const base::DictionaryValue& dictionary) {
  base::NullableString16 display = ParseString(dictionary, "display", Trim);
  if (display.is_null())
    return blink::kWebDisplayModeUndefined;

  blink::WebDisplayMode display_enum =
      WebDisplayModeFromString(base::UTF16ToUTF8(display.string()));
  if (display_enum == blink::kWebDisplayModeUndefined)
    AddErrorInfo("unknown 'display' value ignored.");
  return display_enum;
}

blink::WebScreenOrientationLockType ManifestParser::ParseOrientation(
    const base::DictionaryValue& dictionary) {
  base::NullableString16 orientation =
      ParseString(dictionary, "orientation", Trim);

  if (orientation.is_null())
    return blink::kWebScreenOrientationLockDefault;

  blink::WebScreenOrientationLockType orientation_enum =
      WebScreenOrientationLockTypeFromString(
          base::UTF16ToUTF8(orientation.string()));
  if (orientation_enum == blink::kWebScreenOrientationLockDefault)
    AddErrorInfo("unknown 'orientation' value ignored.");
  return orientation_enum;
}

GURL ManifestParser::ParseIconSrc(const base::DictionaryValue& icon) {
  return ParseURL(icon, "src", manifest_url_,
                  ParseURLOriginRestrictions::kNoRestrictions);
}

base::string16 ManifestParser::ParseIconType(
    const base::DictionaryValue& icon) {
  base::NullableString16 nullable_string = ParseString(icon, "type", Trim);
  if (nullable_string.is_null())
    return base::string16();
  return nullable_string.string();
}

std::vector<gfx::Size> ManifestParser::ParseIconSizes(
    const base::DictionaryValue& icon) {
  base::NullableString16 sizes_str = ParseString(icon, "sizes", NoTrim);
  std::vector<gfx::Size> sizes;

  if (sizes_str.is_null())
    return sizes;

  blink::WebVector<blink::WebSize> web_sizes =
      blink::WebIconSizesParser::ParseIconSizes(
          blink::WebString::FromUTF16(sizes_str.string()));
  sizes.resize(web_sizes.size());
  for (size_t i = 0; i < web_sizes.size(); ++i)
    sizes[i] = web_sizes[i];
  if (sizes.empty()) {
    AddErrorInfo("found icon with no valid size.");
  }
  return sizes;
}

std::vector<blink::Manifest::Icon::IconPurpose>
ManifestParser::ParseIconPurpose(const base::DictionaryValue& icon) {
  base::NullableString16 purpose_str = ParseString(icon, "purpose", NoTrim);
  std::vector<blink::Manifest::Icon::IconPurpose> purposes;

  if (purpose_str.is_null()) {
    purposes.push_back(blink::Manifest::Icon::IconPurpose::ANY);
    return purposes;
  }

  std::vector<base::string16> keywords = base::SplitString(
      purpose_str.string(), base::ASCIIToUTF16(" "),
      base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  for (const base::string16& keyword : keywords) {
    if (base::LowerCaseEqualsASCII(keyword, "any")) {
      purposes.push_back(blink::Manifest::Icon::IconPurpose::ANY);
    } else if (base::LowerCaseEqualsASCII(keyword, "badge")) {
      purposes.push_back(blink::Manifest::Icon::IconPurpose::BADGE);
    } else {
      AddErrorInfo(
          "found icon with invalid purpose. "
          "Using default value 'any'.");
    }
  }

  if (purposes.empty()) {
    purposes.push_back(blink::Manifest::Icon::IconPurpose::ANY);
  }

  return purposes;
}

std::vector<blink::Manifest::Icon> ManifestParser::ParseIcons(
    const base::DictionaryValue& dictionary) {
  std::vector<blink::Manifest::Icon> icons;
  if (!dictionary.HasKey("icons"))
    return icons;

  const base::ListValue* icons_list = nullptr;
  if (!dictionary.GetList("icons", &icons_list)) {
    AddErrorInfo("property 'icons' ignored, type array expected.");
    return icons;
  }

  for (size_t i = 0; i < icons_list->GetSize(); ++i) {
    const base::DictionaryValue* icon_dictionary = nullptr;
    if (!icons_list->GetDictionary(i, &icon_dictionary))
      continue;

    blink::Manifest::Icon icon;
    icon.src = ParseIconSrc(*icon_dictionary);
    // An icon MUST have a valid src. If it does not, it MUST be ignored.
    if (!icon.src.is_valid())
      continue;
    icon.type = ParseIconType(*icon_dictionary);
    icon.sizes = ParseIconSizes(*icon_dictionary);
    icon.purpose = ParseIconPurpose(*icon_dictionary);

    icons.push_back(icon);
  }

  return icons;
}

GURL ManifestParser::ParseShareTargetURLTemplate(
    const base::DictionaryValue& share_target) {
  GURL url_template = ParseURL(share_target, "url_template", manifest_url_,
                               ParseURLOriginRestrictions::kSameOriginOnly);
  if (!blink::ValidateWebShareUrlTemplate(url_template)) {
    AddErrorInfo(
        "property 'url_template' ignored. Placeholders have incorrect "
        "syntax.");
    return GURL();
  }

  return url_template;
}

base::Optional<blink::Manifest::ShareTarget> ManifestParser::ParseShareTarget(
    const base::DictionaryValue& dictionary) {
  if (!dictionary.HasKey("share_target"))
    return base::nullopt;

  blink::Manifest::ShareTarget share_target;
  const base::DictionaryValue* share_target_dict = nullptr;
  dictionary.GetDictionary("share_target", &share_target_dict);
  share_target.url_template = ParseShareTargetURLTemplate(*share_target_dict);

  if (share_target.url_template.is_empty())
    return base::nullopt;
  return base::Optional<blink::Manifest::ShareTarget>(share_target);
}

base::NullableString16 ManifestParser::ParseRelatedApplicationPlatform(
    const base::DictionaryValue& application) {
  return ParseString(application, "platform", Trim);
}

GURL ManifestParser::ParseRelatedApplicationURL(
    const base::DictionaryValue& application) {
  return ParseURL(application, "url", manifest_url_,
                  ParseURLOriginRestrictions::kNoRestrictions);
}

base::NullableString16 ManifestParser::ParseRelatedApplicationId(
    const base::DictionaryValue& application) {
  return ParseString(application, "id", Trim);
}

std::vector<blink::Manifest::RelatedApplication>
ManifestParser::ParseRelatedApplications(
    const base::DictionaryValue& dictionary) {
  std::vector<blink::Manifest::RelatedApplication> applications;
  if (!dictionary.HasKey("related_applications"))
    return applications;

  const base::ListValue* applications_list = nullptr;
  if (!dictionary.GetList("related_applications", &applications_list)) {
    AddErrorInfo("property 'related_applications' ignored,"
                 " type array expected.");
    return applications;
  }

  for (size_t i = 0; i < applications_list->GetSize(); ++i) {
    const base::DictionaryValue* application_dictionary = nullptr;
    if (!applications_list->GetDictionary(i, &application_dictionary))
      continue;

    blink::Manifest::RelatedApplication application;
    application.platform =
        ParseRelatedApplicationPlatform(*application_dictionary);
    // "If platform is undefined, move onto the next item if any are left."
    if (application.platform.is_null()) {
      AddErrorInfo("'platform' is a required field, related application"
                   " ignored.");
      continue;
    }

    application.id = ParseRelatedApplicationId(*application_dictionary);
    application.url = ParseRelatedApplicationURL(*application_dictionary);
    // "If both id and url are undefined, move onto the next item if any are
    // left."
    if (application.url.is_empty() && application.id.is_null()) {
      AddErrorInfo("one of 'url' or 'id' is required, related application"
                   " ignored.");
      continue;
    }

    applications.push_back(application);
  }

  return applications;
}

bool ManifestParser::ParsePreferRelatedApplications(
    const base::DictionaryValue& dictionary) {
  return ParseBoolean(dictionary, "prefer_related_applications", false);
}

base::Optional<SkColor> ManifestParser::ParseThemeColor(
    const base::DictionaryValue& dictionary) {
  return ParseColor(dictionary, "theme_color");
}

base::Optional<SkColor> ManifestParser::ParseBackgroundColor(
    const base::DictionaryValue& dictionary) {
  return ParseColor(dictionary, "background_color");
}

GURL ManifestParser::ParseSplashScreenURL(
    const base::DictionaryValue& dictionary) {
  return ParseURL(dictionary, "splash_screen_url", manifest_url_,
                  ParseURLOriginRestrictions::kSameOriginOnly);
}

base::NullableString16 ManifestParser::ParseGCMSenderID(
    const base::DictionaryValue& dictionary)  {
  return ParseString(dictionary, "gcm_sender_id", Trim);
}

void ManifestParser::AddErrorInfo(const std::string& error_msg,
                                  bool critical,
                                  int error_line,
                                  int error_column) {
  errors_.push_back(
      {base::in_place, error_msg, critical, error_line, error_column});
}

} // namespace content
