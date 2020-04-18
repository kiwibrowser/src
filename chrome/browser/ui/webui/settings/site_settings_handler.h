// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_SITE_SETTINGS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_SITE_SETTINGS_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/scoped_observer.h"
#include "chrome/browser/storage/storage_info_fetcher.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/content_settings/core/browser/content_settings_observer.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "ppapi/buildflags/buildflags.h"
#include "third_party/blink/public/mojom/quota/quota_types.mojom.h"

class HostContentSettingsMap;
class Profile;

#if defined(OS_CHROMEOS)
class PrefChangeRegistrar;
#endif

namespace base {
class ListValue;
}

namespace settings {

// Chrome "ContentSettings" settings page UI handler.
class SiteSettingsHandler : public SettingsPageUIHandler,
                            public content_settings::Observer,
                            public content::NotificationObserver {
 public:
  explicit SiteSettingsHandler(Profile* profile);
  ~SiteSettingsHandler() override;

  // SettingsPageUIHandler:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  // Usage info.
  void OnGetUsageInfo(const storage::UsageInfoEntries& entries);
  void OnStorageCleared(base::OnceClosure callback,
                        blink::mojom::QuotaStatusCode code);
  void OnUsageCleared();

#if defined(OS_CHROMEOS)
  // Alert the Javascript that the |kEnableDRM| pref has changed.
  void OnPrefEnableDrmChanged();
#endif

  // content_settings::Observer:
  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type,
                               std::string resource_identifier) override;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // content::HostZoomMap subscription.
  void OnZoomLevelChanged(const content::HostZoomMap::ZoomLevelChange& change);

 private:
  friend class SiteSettingsHandlerTest;
  friend class SiteSettingsHandlerInfobarTest;
#if BUILDFLAG(ENABLE_PLUGINS)
  FRIEND_TEST_ALL_PREFIXES(SiteSettingsHandlerTest,
                           ChangingFlashSettingForSiteIsRemembered);
#endif
  FRIEND_TEST_ALL_PREFIXES(SiteSettingsHandlerTest, DefaultSettingSource);
  FRIEND_TEST_ALL_PREFIXES(SiteSettingsHandlerTest, ExceptionHelpers);
  FRIEND_TEST_ALL_PREFIXES(SiteSettingsHandlerTest, ExtensionDisplayName);
  FRIEND_TEST_ALL_PREFIXES(SiteSettingsHandlerTest, GetAndSetDefault);
  FRIEND_TEST_ALL_PREFIXES(SiteSettingsHandlerTest, GetAndSetOriginPermissions);
  FRIEND_TEST_ALL_PREFIXES(SiteSettingsHandlerTest, GetAndSetForInvalidURLs);
  FRIEND_TEST_ALL_PREFIXES(SiteSettingsHandlerTest, Incognito);
  FRIEND_TEST_ALL_PREFIXES(SiteSettingsHandlerTest, Origins);
  FRIEND_TEST_ALL_PREFIXES(SiteSettingsHandlerTest, Patterns);
  FRIEND_TEST_ALL_PREFIXES(SiteSettingsHandlerTest, ZoomLevels);
  FRIEND_TEST_ALL_PREFIXES(SiteSettingsHandlerInfobarTest,
                           SettingPermissionsTriggersInfobar);
  FRIEND_TEST_ALL_PREFIXES(SiteSettingsHandlerTest, SessionOnlyException);

  // Asynchronously fetches the usage for a given origin. Replies back with
  // OnGetUsageInfo above.
  void HandleFetchUsageTotal(const base::ListValue* args);

  // Deletes the storage being used for a given host.
  void HandleClearUsage(const base::ListValue* args);

  // Handles the request for a list of all USB devices.
  void HandleFetchUsbDevices(const base::ListValue* args);

  // Removes a particular USB device permission.
  void HandleRemoveUsbDevice(const base::ListValue* args);

  // Gets and sets the default value for a particular content settings type.
  void HandleSetDefaultValueForContentType(const base::ListValue* args);
  void HandleGetDefaultValueForContentType(const base::ListValue* args);

  // Returns the list of site exceptions for a given content settings type.
  void HandleGetExceptionList(const base::ListValue* args);

  // Gets and sets a list of ContentSettingTypes for an origin.
  // TODO(https://crbug.com/739241): Investigate replacing the
  // '*CategoryPermissionForPattern' equivalents below with these methods.
  void HandleGetOriginPermissions(const base::ListValue* args);
  void HandleSetOriginPermissions(const base::ListValue* args);

  // Clears the Flash data setting used to remember if the user has changed the
  // Flash permission for an origin.
  void HandleClearFlashPref(const base::ListValue* args);

  // Handles setting and resetting an origin permission.
  void HandleResetCategoryPermissionForPattern(const base::ListValue* args);
  void HandleSetCategoryPermissionForPattern(const base::ListValue* args);

  // Returns whether a given string is a valid origin.
  void HandleIsOriginValid(const base::ListValue* args);

  // Returns whether a given pattern is valid.
  void HandleIsPatternValid(const base::ListValue* args);

  // Looks up whether an incognito session is active.
  void HandleUpdateIncognitoStatus(const base::ListValue* args);

  // Notifies the JS side whether incognito is enabled.
  void SendIncognitoStatus(Profile* profile, bool was_destroyed);

  // Handles the request for a list of all zoom levels.
  void HandleFetchZoomLevels(const base::ListValue* args);

  // Sends the zoom level list down to the web ui.
  void SendZoomLevels();

  // Removes a particular zoom level for a given host.
  void HandleRemoveZoomLevel(const base::ListValue* args);

  Profile* profile_;

  content::NotificationRegistrar notification_registrar_;

  // Keeps track of events related to zooming.
  std::unique_ptr<content::HostZoomMap::Subscription>
      host_zoom_map_subscription_;

  // The host for which to fetch usage.
  std::string usage_host_;

  // The origin for which to clear usage.
  std::string clearing_origin_;

  // Change observer for content settings.
  ScopedObserver<HostContentSettingsMap, content_settings::Observer> observer_;

#if defined(OS_CHROMEOS)
  // Change observer for prefs.
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;
#endif

  DISALLOW_COPY_AND_ASSIGN(SiteSettingsHandler);
};

}  // namespace settings

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_SITE_SETTINGS_HANDLER_H_
