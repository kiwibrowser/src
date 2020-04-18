// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/page_info/permission_menu_model.h"

#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/plugins/plugin_utils.h"
#include "chrome/browser/plugins/plugins_field_trial.h"
#include "chrome/common/chrome_features.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/common/origin_util.h"
#include "ppapi/buildflags/buildflags.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"

PermissionMenuModel::PermissionMenuModel(Profile* profile,
                                         const GURL& url,
                                         const PageInfoUI::PermissionInfo& info,
                                         const ChangeCallback& callback)
    : ui::SimpleMenuModel(this),
      host_content_settings_map_(
          HostContentSettingsMapFactory::GetForProfile(profile)),
      permission_(info),
      callback_(callback) {
  DCHECK(!callback_.is_null());
  base::string16 label;

  // Retrieve the string to show for the default setting for this permission.
  ContentSetting effective_default_setting = permission_.default_setting;

#if BUILDFLAG(ENABLE_PLUGINS)
  effective_default_setting = PluginsFieldTrial::EffectiveContentSetting(
      host_content_settings_map_, permission_.type,
      permission_.default_setting);
#endif  // BUILDFLAG(ENABLE_PLUGINS)

  switch (effective_default_setting) {
    case CONTENT_SETTING_ALLOW:
      label = l10n_util::GetStringUTF16(IDS_PAGE_INFO_MENU_ITEM_DEFAULT_ALLOW);
      break;
    case CONTENT_SETTING_BLOCK:
      label = l10n_util::GetStringUTF16(IDS_PAGE_INFO_MENU_ITEM_DEFAULT_BLOCK);
      break;
    case CONTENT_SETTING_ASK:
      label = l10n_util::GetStringUTF16(IDS_PAGE_INFO_MENU_ITEM_DEFAULT_ASK);
      break;
    case CONTENT_SETTING_DETECT_IMPORTANT_CONTENT:
      // TODO(tommycli): We display the ASK string for DETECT because with
      // HTML5 by Default, Chrome will ask before running Flash on most sites.
      // Once the feature flag is gone, migrate the actual setting to ASK.
      label = l10n_util::GetStringUTF16(
          PluginUtils::ShouldPreferHtmlOverPlugins(host_content_settings_map_)
              ? IDS_PAGE_INFO_MENU_ITEM_DEFAULT_ASK
              : IDS_PAGE_INFO_MENU_ITEM_DEFAULT_DETECT_IMPORTANT_CONTENT);
      break;
    case CONTENT_SETTING_NUM_SETTINGS:
      NOTREACHED();
      break;
    default:
      break;
  }

  // The Material UI for site settings uses comboboxes instead of menubuttons,
  // which means the elements of the menu themselves have to be shorter, instead
  // of simply setting a shorter label on the menubutton.
  if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
    label = PageInfoUI::PermissionActionToUIString(
        profile, permission_.type, CONTENT_SETTING_DEFAULT,
        effective_default_setting, permission_.source);
  }

  AddCheckItem(CONTENT_SETTING_DEFAULT, label);

  // Retrieve the string to show for allowing the permission.
  if (ShouldShowAllow(url)) {
    label = l10n_util::GetStringUTF16(IDS_PAGE_INFO_MENU_ITEM_ALLOW);
    if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
      label = PageInfoUI::PermissionActionToUIString(
          profile, permission_.type, CONTENT_SETTING_ALLOW,
          effective_default_setting, permission_.source);
    }
    AddCheckItem(CONTENT_SETTING_ALLOW, label);
  }

  // TODO(tommycli): With the HTML5 by Default feature, Flash is treated the
  // same as any other permission with ASK, i.e. there is no ASK exception.
  // Once the feature flag is gone, remove this block of code entirely.
  if (permission_.type == CONTENT_SETTINGS_TYPE_PLUGINS &&
      !PluginUtils::ShouldPreferHtmlOverPlugins(host_content_settings_map_)) {
    label = l10n_util::GetStringUTF16(
        IDS_PAGE_INFO_MENU_ITEM_DETECT_IMPORTANT_CONTENT);
    AddCheckItem(CONTENT_SETTING_DETECT_IMPORTANT_CONTENT, label);
  }

  // Retrieve the string to show for blocking the permission.
  label = l10n_util::GetStringUTF16(IDS_PAGE_INFO_MENU_ITEM_BLOCK);
  if (permission_.type == CONTENT_SETTINGS_TYPE_ADS) {
    label = l10n_util::GetStringUTF16(IDS_PAGE_INFO_MENU_ITEM_ADS_BLOCK);
  }
  if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
    label = PageInfoUI::PermissionActionToUIString(
        profile, info.type, CONTENT_SETTING_BLOCK, effective_default_setting,
        info.source);
  }
  AddCheckItem(CONTENT_SETTING_BLOCK, label);

  // Retrieve the string to show for allowing the user to be asked about the
  // permission.
  if (ShouldShowAsk(url)) {
    label = l10n_util::GetStringUTF16(IDS_PAGE_INFO_MENU_ITEM_ASK);
    if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
      label = PageInfoUI::PermissionActionToUIString(
          profile, info.type, CONTENT_SETTING_ASK, effective_default_setting,
          info.source);
    }
    AddCheckItem(CONTENT_SETTING_ASK, label);
  }
}

PermissionMenuModel::~PermissionMenuModel() {}

bool PermissionMenuModel::IsCommandIdChecked(int command_id) const {
  ContentSetting setting = permission_.setting;

#if BUILDFLAG(ENABLE_PLUGINS)
  setting = PluginsFieldTrial::EffectiveContentSetting(
      host_content_settings_map_, permission_.type, permission_.setting);
#endif  // BUILDFLAG(ENABLE_PLUGINS)

  return setting == command_id;
}

bool PermissionMenuModel::IsCommandIdEnabled(int command_id) const {
  return true;
}

void PermissionMenuModel::ExecuteCommand(int command_id, int event_flags) {
  permission_.setting = static_cast<ContentSetting>(command_id);
  callback_.Run(permission_);
}

bool PermissionMenuModel::ShouldShowAllow(const GURL& url) {
  // Notifications does not support CONTENT_SETTING_ALLOW in incognito.
  if (permission_.is_incognito &&
      permission_.type == CONTENT_SETTINGS_TYPE_NOTIFICATIONS) {
    return false;
  }

  // Media only supports CONTENT_SETTING_ALLOW for secure origins.
  if ((permission_.type == CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC ||
       permission_.type == CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA) &&
      !content::IsOriginSecure(url)) {
    return false;
  }

  // Chooser permissions do not support CONTENT_SETTING_ALLOW.
  if (permission_.type == CONTENT_SETTINGS_TYPE_USB_GUARD)
    return false;

  return true;
}

bool PermissionMenuModel::ShouldShowAsk(const GURL& url) {
  return permission_.type == CONTENT_SETTINGS_TYPE_USB_GUARD;
}
