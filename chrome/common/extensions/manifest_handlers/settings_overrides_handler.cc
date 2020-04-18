// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/manifest_handlers/settings_overrides_handler.h"

#include <memory>
#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/url_formatter/url_formatter.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/feature_switch.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/permissions_parser.h"
#include "extensions/common/permissions/api_permission_set.h"
#include "extensions/common/permissions/manifest_permission.h"
#include "extensions/common/permissions/permissions_info.h"
#include "extensions/common/permissions/settings_override_permission.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_message_utils.h"
#include "url/gurl.h"

using extensions::api::manifest_types::ChromeSettingsOverrides;

namespace extensions {
namespace {

std::unique_ptr<GURL> CreateManifestURL(const std::string& url) {
  std::unique_ptr<GURL> manifest_url(new GURL(url));
  if (!manifest_url->is_valid() ||
      !manifest_url->SchemeIsHTTPOrHTTPS())
    return std::unique_ptr<GURL>();
  return manifest_url;
}

std::unique_ptr<GURL> ParseHomepage(const ChromeSettingsOverrides& overrides,
                                    base::string16* error) {
  if (!overrides.homepage)
    return std::unique_ptr<GURL>();
  std::unique_ptr<GURL> manifest_url = CreateManifestURL(*overrides.homepage);
  if (!manifest_url) {
    *error = extensions::ErrorUtils::FormatErrorMessageUTF16(
        manifest_errors::kInvalidHomepageOverrideURL, *overrides.homepage);
  }
  return manifest_url;
}

std::vector<GURL> ParseStartupPage(const ChromeSettingsOverrides& overrides,
                                   base::string16* error) {
  std::vector<GURL> urls;
  if (!overrides.startup_pages)
    return urls;

  for (std::vector<std::string>::const_iterator i =
       overrides.startup_pages->begin(); i != overrides.startup_pages->end();
       ++i) {
    std::unique_ptr<GURL> manifest_url = CreateManifestURL(*i);
    if (!manifest_url) {
      *error = extensions::ErrorUtils::FormatErrorMessageUTF16(
          manifest_errors::kInvalidStartupOverrideURL, *i);
    } else {
      urls.push_back(GURL());
      urls.back().Swap(manifest_url.get());
    }
  }
  return urls;
}

std::unique_ptr<ChromeSettingsOverrides::Search_provider> ParseSearchEngine(
    ChromeSettingsOverrides* overrides,
    base::string16* error) {
  if (!overrides->search_provider)
    return std::unique_ptr<ChromeSettingsOverrides::Search_provider>();
  if (!CreateManifestURL(overrides->search_provider->search_url)) {
    *error = extensions::ErrorUtils::FormatErrorMessageUTF16(
        manifest_errors::kInvalidSearchEngineURL,
        overrides->search_provider->search_url);
    return std::unique_ptr<ChromeSettingsOverrides::Search_provider>();
  }
  if (overrides->search_provider->prepopulated_id)
    return std::move(overrides->search_provider);
  if (!overrides->search_provider->name ||
      !overrides->search_provider->keyword ||
      !overrides->search_provider->encoding ||
      !overrides->search_provider->favicon_url) {
    *error =
        base::ASCIIToUTF16(manifest_errors::kInvalidSearchEngineMissingKeys);
    return std::unique_ptr<ChromeSettingsOverrides::Search_provider>();
  }
  if (!CreateManifestURL(*overrides->search_provider->favicon_url)) {
    *error = extensions::ErrorUtils::FormatErrorMessageUTF16(
        manifest_errors::kInvalidSearchEngineURL,
        *overrides->search_provider->favicon_url);
    return std::unique_ptr<ChromeSettingsOverrides::Search_provider>();
  }
  return std::move(overrides->search_provider);
}

std::string FormatUrlForDisplay(const GURL& url) {
  base::StringPiece host = url.host_piece();
  // A www. prefix is not informative and thus not worth the limited real estate
  // in the permissions UI.
  // TODO(catmullings): Ideally, we wouldn't be using custom code to format URLs
  // here, since we have a number of methods that do that more universally.
  return base::UTF16ToUTF8(url_formatter::StripWWW(base::UTF8ToUTF16(host)));
}

}  // namespace

SettingsOverrides::SettingsOverrides() {}

SettingsOverrides::~SettingsOverrides() {}

// static
const SettingsOverrides* SettingsOverrides::Get(
    const Extension* extension) {
  return static_cast<SettingsOverrides*>(
      extension->GetManifestData(manifest_keys::kSettingsOverride));
}

SettingsOverridesHandler::SettingsOverridesHandler() {}

SettingsOverridesHandler::~SettingsOverridesHandler() {}

bool SettingsOverridesHandler::Parse(Extension* extension,
                                     base::string16* error) {
  const base::Value* dict = NULL;
  CHECK(extension->manifest()->Get(manifest_keys::kSettingsOverride, &dict));
  std::unique_ptr<ChromeSettingsOverrides> settings(
      ChromeSettingsOverrides::FromValue(*dict, error));
  if (!settings)
    return false;

  std::unique_ptr<SettingsOverrides> info(new SettingsOverrides);
  info->homepage = ParseHomepage(*settings, error);
  info->search_engine = ParseSearchEngine(settings.get(), error);
  info->startup_pages = ParseStartupPage(*settings, error);
  if (!info->homepage && !info->search_engine && info->startup_pages.empty()) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        manifest_errors::kInvalidEmptyDictionary,
        manifest_keys::kSettingsOverride);
    return false;
  }

  if (info->search_engine) {
    PermissionsParser::AddAPIPermission(
        extension, new SettingsOverrideAPIPermission(
                       PermissionsInfo::GetInstance()->GetByID(
                           APIPermission::kSearchProvider),
                       FormatUrlForDisplay(*CreateManifestURL(
                           info->search_engine->search_url))));
  }
  if (!info->startup_pages.empty()) {
    PermissionsParser::AddAPIPermission(
        extension,
        new SettingsOverrideAPIPermission(
            PermissionsInfo::GetInstance()->GetByID(
                APIPermission::kStartupPages),
            // We only support one startup page even though the type of the
            // manifest property is a list, only the first one is used.
            FormatUrlForDisplay(info->startup_pages[0])));
  }
  if (info->homepage) {
    PermissionsParser::AddAPIPermission(
        extension,
        new SettingsOverrideAPIPermission(
            PermissionsInfo::GetInstance()->GetByID(APIPermission::kHomepage),
            FormatUrlForDisplay(*(info->homepage))));
  }
  extension->SetManifestData(manifest_keys::kSettingsOverride, std::move(info));
  return true;
}

base::span<const char* const> SettingsOverridesHandler::Keys() const {
  static constexpr const char* kKeys[] = {manifest_keys::kSettingsOverride};
  return kKeys;
}

}  // namespace extensions
