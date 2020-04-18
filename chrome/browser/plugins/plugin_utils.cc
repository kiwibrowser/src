// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/plugins/plugin_utils.h"

#include "base/values.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/plugin_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "content/public/common/webplugininfo.h"
#include "url/gurl.h"
#include "url/origin.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/info_map.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handlers/mime_types_handler.h"
#endif

namespace {

const char kFlashPluginID[] = "adobe-flash-player";

void GetPluginContentSettingInternal(
    const HostContentSettingsMap* host_content_settings_map,
    bool use_javascript_setting,
    const url::Origin& main_frame_origin,
    const GURL& plugin_url,
    const std::string& resource,
    ContentSetting* setting,
    bool* is_default,
    bool* is_managed) {
  GURL main_frame_url = main_frame_origin.GetURL();
  std::unique_ptr<base::Value> value;
  content_settings::SettingInfo info;
  bool uses_plugin_specific_setting = false;
  if (use_javascript_setting) {
    value = host_content_settings_map->GetWebsiteSetting(
        main_frame_url, main_frame_url, CONTENT_SETTINGS_TYPE_JAVASCRIPT,
        std::string(), &info);
  } else {
    content_settings::SettingInfo specific_info;
    std::unique_ptr<base::Value> specific_setting =
        host_content_settings_map->GetWebsiteSetting(
            main_frame_url, plugin_url, CONTENT_SETTINGS_TYPE_PLUGINS, resource,
            &specific_info);
    content_settings::SettingInfo general_info;
    std::unique_ptr<base::Value> general_setting =
        host_content_settings_map->GetWebsiteSetting(
            main_frame_url, plugin_url, CONTENT_SETTINGS_TYPE_PLUGINS,
            std::string(), &general_info);
    // If there is a plugin-specific setting, we use it, unless the general
    // setting was set by policy, in which case it takes precedence.
    uses_plugin_specific_setting =
        specific_setting &&
        general_info.source != content_settings::SETTING_SOURCE_POLICY;
    if (uses_plugin_specific_setting) {
      value = std::move(specific_setting);
      info = specific_info;
    } else {
      value = std::move(general_setting);
      info = general_info;
    }
  }
  *setting = content_settings::ValueToContentSetting(value.get());

  bool uses_default_content_setting =
      !uses_plugin_specific_setting &&
      info.primary_pattern == ContentSettingsPattern::Wildcard() &&
      info.secondary_pattern == ContentSettingsPattern::Wildcard();

  if (is_default)
    *is_default = uses_default_content_setting;
  if (is_managed)
    *is_managed = info.source == content_settings::SETTING_SOURCE_POLICY;

  // Special behavior for non-JavaScript treated plugins (Flash):
  if (!use_javascript_setting) {
    // ALLOW-by-default is obsolete and should be treated as DETECT.
    if (*setting == CONTENT_SETTING_ALLOW && uses_default_content_setting)
      *setting = CONTENT_SETTING_DETECT_IMPORTANT_CONTENT;

    // Unless the setting is explicitly ALLOW, return BLOCK for any scheme that
    // is not HTTP, HTTPS, FILE, or chrome-extension.
    if (*setting != CONTENT_SETTING_ALLOW &&
        PluginUtils::ShouldPreferHtmlOverPlugins(host_content_settings_map) &&
        !main_frame_url.SchemeIsHTTPOrHTTPS() &&
        !main_frame_url.SchemeIsFile() &&
        !main_frame_url.SchemeIs(extensions::kExtensionScheme)) {
      *setting = CONTENT_SETTING_BLOCK;
    }
  }
}

}  // namespace

// static
void PluginUtils::GetPluginContentSetting(
    const HostContentSettingsMap* host_content_settings_map,
    const content::WebPluginInfo& plugin,
    const url::Origin& main_frame_origin,
    const GURL& plugin_url,
    const std::string& resource,
    ContentSetting* setting,
    bool* uses_default_content_setting,
    bool* is_managed) {
  GetPluginContentSettingInternal(
      host_content_settings_map, ShouldUseJavaScriptSettingForPlugin(plugin),
      main_frame_origin, plugin_url, resource, setting,
      uses_default_content_setting, is_managed);
}

// static
ContentSetting PluginUtils::GetFlashPluginContentSetting(
    const HostContentSettingsMap* host_content_settings_map,
    const url::Origin& main_frame_origin,
    const GURL& plugin_url,
    bool* is_managed) {
  ContentSetting plugin_setting = CONTENT_SETTING_DEFAULT;
  GetPluginContentSettingInternal(host_content_settings_map,
                                  false /* use_javascript_setting */,
                                  main_frame_origin, plugin_url, kFlashPluginID,
                                  &plugin_setting, nullptr, is_managed);
  return plugin_setting;
}

// static
void PluginUtils::RememberFlashChangedForSite(
    HostContentSettingsMap* host_content_settings_map,
    const GURL& top_level_url) {
  // A |base::DictionaryValue| is set here but for now, clients only check this
  // is a non-nullptr value.
  auto dict = std::make_unique<base::DictionaryValue>();
  constexpr char kFlagKey[] = "flashPreviouslyChanged";
  dict->SetKey(kFlagKey, base::Value(true));
  host_content_settings_map->SetWebsiteSettingDefaultScope(
      top_level_url, top_level_url, CONTENT_SETTINGS_TYPE_PLUGINS_DATA,
      std::string(), std::move(dict));
}

// static
bool PluginUtils::ShouldPreferHtmlOverPlugins(
    const HostContentSettingsMap* host_content_settings_map) {
  return base::FeatureList::IsEnabled(features::kPreferHtmlOverPlugins);
}

// static
std::string PluginUtils::GetExtensionIdForMimeType(
    content::ResourceContext* resource_context,
    const std::string& mime_type) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  ProfileIOData* io_data = ProfileIOData::FromResourceContext(resource_context);
  bool profile_is_off_the_record = io_data->IsOffTheRecord();
  const scoped_refptr<const extensions::InfoMap> extension_info_map(
      io_data->GetExtensionInfoMap());
  std::vector<std::string> whitelist = MimeTypesHandler::GetMIMETypeWhitelist();
  // Go through the white-listed extensions and try to use them to intercept
  // the URL request.
  for (const std::string& extension_id : whitelist) {
    const extensions::Extension* extension =
        extension_info_map->extensions().GetByID(extension_id);
    // The white-listed extension may not be installed, so we have to NULL check
    // |extension|.
    if (!extension || (profile_is_off_the_record &&
                       !extension_info_map->IsIncognitoEnabled(extension_id))) {
      continue;
    }

    if (extension_id == extension_misc::kPdfExtensionId &&
        io_data->always_open_pdf_externally()->GetValue()) {
      continue;
    }

    MimeTypesHandler* handler = MimeTypesHandler::GetHandler(extension);
    if (handler && handler->CanHandleMIMEType(mime_type))
      return extension_id;
  }
#endif
  return std::string();
}
