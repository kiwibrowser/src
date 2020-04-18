// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/intent_helper/arc_settings_service.h"

#include <string>

#include "ash/public/cpp/ash_pref_names.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/gtest_prod_util.h"
#include "base/json/json_writer.h"
#include "base/memory/singleton.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/arc/policy/arc_policy_util.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/system/timezone_resolver_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/ui/zoom/chrome_zoom_level_prefs.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_state_handler_observer.h"
#include "chromeos/network/onc/onc_utils.h"
#include "chromeos/network/proxy/proxy_config_service_impl.h"
#include "chromeos/settings/cros_settings_names.h"
#include "chromeos/settings/timezone_settings.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "components/arc/arc_prefs.h"
#include "components/arc/common/backup_settings.mojom.h"
#include "components/arc/intent_helper/arc_intent_helper_bridge.h"
#include "components/arc/intent_helper/font_size_util.h"
#include "components/onc/onc_pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/pref_proxy_config_tracker_impl.h"
#include "components/proxy_config/proxy_config_dictionary.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "content/public/common/page_zoom.h"
#include "net/proxy_resolution/proxy_config.h"

using ::chromeos::CrosSettings;
using ::chromeos::system::TimezoneSettings;

namespace {

bool GetHttpProxyServer(const ProxyConfigDictionary* proxy_config_dict,
                        std::string* host,
                        int* port) {
  std::string proxy_rules_string;
  if (!proxy_config_dict->GetProxyServer(&proxy_rules_string))
    return false;

  net::ProxyConfig::ProxyRules proxy_rules;
  proxy_rules.ParseFromString(proxy_rules_string);

  const net::ProxyList* proxy_list = nullptr;
  if (proxy_rules.type == net::ProxyConfig::ProxyRules::Type::PROXY_LIST) {
    proxy_list = &proxy_rules.single_proxies;
  } else if (proxy_rules.type ==
             net::ProxyConfig::ProxyRules::Type::PROXY_LIST_PER_SCHEME) {
    proxy_list = proxy_rules.MapUrlSchemeToProxyList(url::kHttpScheme);
  }
  if (!proxy_list || proxy_list->IsEmpty())
    return false;

  const net::ProxyServer& server = proxy_list->Get();
  *host = server.host_port_pair().host();
  *port = server.host_port_pair().port();
  return !host->empty() && *port;
}

}  // namespace

namespace arc {
namespace {

// Singleton factory for ArcSettingsService.
class ArcSettingsServiceFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcSettingsService,
          ArcSettingsServiceFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcSettingsServiceFactory";

  static ArcSettingsServiceFactory* GetInstance() {
    return base::Singleton<ArcSettingsServiceFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcSettingsServiceFactory>;
  ArcSettingsServiceFactory() = default;
  ~ArcSettingsServiceFactory() override = default;
};

}  // namespace

// Listens to changes for select Chrome settings (prefs) that Android cares
// about and sends the new values to Android to keep the state in sync.
class ArcSettingsServiceImpl
    : public chromeos::system::TimezoneSettings::Observer,
      public ConnectionObserver<mojom::AppInstance>,
      public chromeos::NetworkStateHandlerObserver {
 public:
  ArcSettingsServiceImpl(Profile* profile,
                         ArcBridgeService* arc_bridge_service);
  ~ArcSettingsServiceImpl() override;

  // Called when a Chrome pref we have registered an observer for has changed.
  // Obtains the new pref value and sends it to Android.
  void OnPrefChanged(const std::string& pref_name) const;

  // TimezoneSettings::Observer:
  void TimezoneChanged(const icu::TimeZone& timezone) override;

  // NetworkStateHandlerObserver:
  void DefaultNetworkChanged(const chromeos::NetworkState* network) override;

  // Retrieves Chrome's state for the settings that need to be synced on the
  // initial Android boot and send it to Android. Called by ArcSettingsService.
  void SyncInitialSettings() const;

 private:
  PrefService* GetPrefs() const { return profile_->GetPrefs(); }

  // Returns whether kProxy pref proxy config is applied.
  bool IsPrefProxyConfigApplied() const;

  // Registers to observe changes for Chrome settings we care about.
  void StartObservingSettingsChanges();

  // Stops listening for Chrome settings changes.
  void StopObservingSettingsChanges();

  // Retrieves Chrome's state for the settings that need to be synced on each
  // Android boot and send it to Android.
  void SyncBootTimeSettings() const;
  // Retrieves Chrome's state for the settings that need to be synced on each
  // Android boot after AppInstance is ready and send it to Android.
  // TODO(crbug.com/762553): Sync settings at proper time.
  void SyncAppTimeSettings();
  // Send particular settings to Android.
  // Keep these lines ordered lexicographically.
  void SyncAccessibilityLargeMouseCursorEnabled() const;
  void SyncAccessibilityVirtualKeyboardEnabled() const;
  void SyncBackupEnabled() const;
  void SyncFocusHighlightEnabled() const;
  void SyncFontSize() const;
  void SyncPageZoom() const;
  void SyncLocale() const;
  void SyncLocationServiceEnabled() const;
  void SyncProxySettings() const;
  void SyncReportingConsent(bool initial_sync) const;
  void SyncSelectToSpeakEnabled() const;
  void SyncSmsConnectEnabled() const;
  void SyncSpokenFeedbackEnabled() const;
  void SyncSwitchAccessEnabled() const;
  void SyncTimeZone() const;
  void SyncTimeZoneByGeolocation() const;
  void SyncUse24HourClock() const;

  // Registers to listen to a particular perf.
  void AddPrefToObserve(const std::string& pref_name);

  // Returns the integer value of the pref.  pref_name must exist.
  int GetIntegerPref(const std::string& pref_name) const;

  // Gets whether this is a managed pref.
  bool IsBooleanPrefManaged(const std::string& pref_name) const;

  // Sends boolean pref broadcast to the delegate.
  void SendBoolPrefSettingsBroadcast(const std::string& pref_name,
                                     const std::string& action) const;

  // Same as above, except sends a specific boolean value.
  void SendBoolValueSettingsBroadcast(bool value,
                                      bool managed,
                                      const std::string& action) const;

  // Sends a broadcast to the delegate.
  void SendSettingsBroadcast(const std::string& action,
                             const base::DictionaryValue& extras) const;

  // ConnectionObserver<mojom::AppInstance>:
  void OnConnectionReady() override;

  Profile* const profile_;
  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.

  // Manages pref observation registration.
  PrefChangeRegistrar registrar_;

  std::unique_ptr<chromeos::CrosSettings::ObserverSubscription>
      reporting_consent_subscription_;

  // Subscription for preference change of default zoom level. Subscription
  // automatically unregisters a callback when it's destructed.
  std::unique_ptr<ChromeZoomLevelPrefs::DefaultZoomLevelSubscription>
      default_zoom_level_subscription_;

  DISALLOW_COPY_AND_ASSIGN(ArcSettingsServiceImpl);
};

ArcSettingsServiceImpl::ArcSettingsServiceImpl(
    Profile* profile,
    ArcBridgeService* arc_bridge_service)
    : profile_(profile), arc_bridge_service_(arc_bridge_service) {
  StartObservingSettingsChanges();
  SyncBootTimeSettings();

  // Note: if App connection is already established, OnConnectionReady()
  // is synchronously called, so that initial sync is done in the method.
  arc_bridge_service_->app()->AddObserver(this);
}

ArcSettingsServiceImpl::~ArcSettingsServiceImpl() {
  StopObservingSettingsChanges();

  arc_bridge_service_->app()->RemoveObserver(this);
}

void ArcSettingsServiceImpl::OnPrefChanged(const std::string& pref_name) const {
  VLOG(1) << "OnPrefChanged: " << pref_name;
  // Keep these lines ordered lexicographically by pref_name.
  if (pref_name == onc::prefs::kDeviceOpenNetworkConfiguration ||
      pref_name == onc::prefs::kOpenNetworkConfiguration) {
    // Only update proxy settings if kProxy pref is not applied.
    if (IsPrefProxyConfigApplied()) {
      LOG(ERROR) << "Open Network Configuration proxy settings are not applied,"
                 << " because kProxy preference is configured.";
      return;
    }
    SyncProxySettings();
  } else if (pref_name == ash::prefs::kAccessibilityFocusHighlightEnabled) {
    SyncFocusHighlightEnabled();
  } else if (pref_name == ash::prefs::kAccessibilityLargeCursorEnabled) {
    SyncAccessibilityLargeMouseCursorEnabled();
  } else if (pref_name == ash::prefs::kAccessibilitySelectToSpeakEnabled) {
    SyncSelectToSpeakEnabled();
  } else if (pref_name == ash::prefs::kAccessibilitySpokenFeedbackEnabled) {
    SyncSpokenFeedbackEnabled();
  } else if (pref_name == ash::prefs::kAccessibilitySwitchAccessEnabled) {
    SyncSwitchAccessEnabled();
  } else if (pref_name == ash::prefs::kAccessibilityVirtualKeyboardEnabled) {
    SyncAccessibilityVirtualKeyboardEnabled();
  } else if (pref_name == ::prefs::kApplicationLocale ||
             pref_name == ::prefs::kLanguagePreferredLanguages) {
    SyncLocale();
  } else if (pref_name == ::prefs::kUse24HourClock) {
    SyncUse24HourClock();
  } else if (pref_name == ::prefs::kResolveTimezoneByGeolocationMethod) {
    SyncTimeZoneByGeolocation();
  } else if (pref_name == ::prefs::kWebKitDefaultFixedFontSize ||
             pref_name == ::prefs::kWebKitDefaultFontSize ||
             pref_name == ::prefs::kWebKitMinimumFontSize) {
    SyncFontSize();
  } else if (pref_name == prefs::kSmsConnectEnabled) {
    SyncSmsConnectEnabled();
  } else if (pref_name == proxy_config::prefs::kProxy) {
    SyncProxySettings();
  } else {
    LOG(ERROR) << "Unknown pref changed.";
  }
}

void ArcSettingsServiceImpl::TimezoneChanged(const icu::TimeZone& timezone) {
  SyncTimeZone();
}

void ArcSettingsServiceImpl::DefaultNetworkChanged(
    const chromeos::NetworkState* network) {
  // kProxy pref has more priority than the default network update.
  // If a default network is changed to the network with ONC policy with proxy
  // settings, it should be translated here.
  if (network && !IsPrefProxyConfigApplied())
    SyncProxySettings();
}

bool ArcSettingsServiceImpl::IsPrefProxyConfigApplied() const {
  net::ProxyConfigWithAnnotation config;
  return PrefProxyConfigTrackerImpl::PrefPrecedes(
      PrefProxyConfigTrackerImpl::ReadPrefConfig(GetPrefs(), &config));
}

void ArcSettingsServiceImpl::StartObservingSettingsChanges() {
  registrar_.Init(GetPrefs());

  // Keep these lines ordered lexicographically.
  AddPrefToObserve(ash::prefs::kAccessibilityFocusHighlightEnabled);
  AddPrefToObserve(ash::prefs::kAccessibilityLargeCursorEnabled);
  AddPrefToObserve(ash::prefs::kAccessibilitySelectToSpeakEnabled);
  AddPrefToObserve(ash::prefs::kAccessibilitySpokenFeedbackEnabled);
  AddPrefToObserve(ash::prefs::kAccessibilitySwitchAccessEnabled);
  AddPrefToObserve(ash::prefs::kAccessibilityVirtualKeyboardEnabled);
  AddPrefToObserve(prefs::kSmsConnectEnabled);
  AddPrefToObserve(::prefs::kResolveTimezoneByGeolocationMethod);
  AddPrefToObserve(::prefs::kUse24HourClock);
  AddPrefToObserve(::prefs::kWebKitDefaultFixedFontSize);
  AddPrefToObserve(::prefs::kWebKitDefaultFontSize);
  AddPrefToObserve(::prefs::kWebKitMinimumFontSize);
  AddPrefToObserve(proxy_config::prefs::kProxy);
  AddPrefToObserve(onc::prefs::kDeviceOpenNetworkConfiguration);
  AddPrefToObserve(onc::prefs::kOpenNetworkConfiguration);

  // Note that some preferences, such as kArcBackupRestoreEnabled and
  // kArcLocationServiceEnabled, are not dynamically updated after initial
  // ARC setup and therefore are not observed here.

  reporting_consent_subscription_ = CrosSettings::Get()->AddSettingsObserver(
      chromeos::kStatsReportingPref,
      base::Bind(&ArcSettingsServiceImpl::SyncReportingConsent,
                 base::Unretained(this), /*initial_sync=*/false));

  // It's safe to use base::Unretained. This is unregistered when
  // default_zoom_level_subscription_ is destructed which is stored as
  // unique_ptr in member of this class.
  default_zoom_level_subscription_ =
      profile_->GetZoomLevelPrefs()->RegisterDefaultZoomLevelCallback(
          base::BindRepeating(&ArcSettingsServiceImpl::SyncPageZoom,
                              base::Unretained(this)));

  TimezoneSettings::GetInstance()->AddObserver(this);

  chromeos::NetworkHandler::Get()->network_state_handler()->AddObserver(
      this, FROM_HERE);
}

void ArcSettingsServiceImpl::StopObservingSettingsChanges() {
  registrar_.RemoveAll();
  reporting_consent_subscription_.reset();

  TimezoneSettings::GetInstance()->RemoveObserver(this);
  chromeos::NetworkHandler::Get()->network_state_handler()->RemoveObserver(
      this, FROM_HERE);
}

void ArcSettingsServiceImpl::SyncInitialSettings() const {
  // Keep these lines ordered lexicographically.
  SyncBackupEnabled();
  SyncLocationServiceEnabled();
  SyncReportingConsent(/*initial_sync=*/true);
}

void ArcSettingsServiceImpl::SyncBootTimeSettings() const {
  // Keep these lines ordered lexicographically.
  SyncAccessibilityLargeMouseCursorEnabled();
  SyncAccessibilityVirtualKeyboardEnabled();
  SyncFocusHighlightEnabled();
  SyncFontSize();
  SyncPageZoom();
  SyncProxySettings();
  SyncReportingConsent(/*initial_sync=*/false);
  SyncSelectToSpeakEnabled();
  SyncSmsConnectEnabled();
  SyncSpokenFeedbackEnabled();
  SyncSwitchAccessEnabled();
  SyncTimeZone();
  SyncTimeZoneByGeolocation();
  SyncUse24HourClock();
}

void ArcSettingsServiceImpl::SyncAppTimeSettings() {
  SyncLocale();

  // Applying system locales change on ARC will cause restarting other services
  // and applications on ARC and doing such change in early phase may lead to
  // ARC OptIn failure.  So that observing preferred languages change should be
  // deferred at least until |mojom::AppInstance| is ready. But it's not ideal
  // (b/6773449, b/65385376).
  AddPrefToObserve(::prefs::kApplicationLocale);
  AddPrefToObserve(::prefs::kLanguagePreferredLanguages);
}

void ArcSettingsServiceImpl::SyncAccessibilityLargeMouseCursorEnabled() const {
  SendBoolPrefSettingsBroadcast(
      ash::prefs::kAccessibilityLargeCursorEnabled,
      "org.chromium.arc.intent_helper.ACCESSIBILITY_LARGE_POINTER_ICON");
}

void ArcSettingsServiceImpl::SyncAccessibilityVirtualKeyboardEnabled() const {
  SendBoolPrefSettingsBroadcast(
      ash::prefs::kAccessibilityVirtualKeyboardEnabled,
      "org.chromium.arc.intent_helper.SET_SHOW_IME_WITH_HARD_KEYBOARD");
}

void ArcSettingsServiceImpl::SyncBackupEnabled() const {
  auto* backup_settings = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service_->backup_settings(), SetBackupEnabled);
  if (backup_settings) {
    const PrefService::Preference* pref =
        registrar_.prefs()->FindPreference(prefs::kArcBackupRestoreEnabled);
    DCHECK(pref);
    const base::Value* value = pref->GetValue();
    DCHECK(value->is_bool());
    backup_settings->SetBackupEnabled(value->GetBool(),
                                      !pref->IsUserModifiable());
  }
}

void ArcSettingsServiceImpl::SyncFocusHighlightEnabled() const {
  SendBoolPrefSettingsBroadcast(
      ash::prefs::kAccessibilityFocusHighlightEnabled,
      "org.chromium.arc.intent_helper.SET_FOCUS_HIGHLIGHT_ENABLED");
}

void ArcSettingsServiceImpl::SyncFontSize() const {
  int default_size = GetIntegerPref(::prefs::kWebKitDefaultFontSize);
  int default_fixed_size = GetIntegerPref(::prefs::kWebKitDefaultFixedFontSize);
  int minimum_size = GetIntegerPref(::prefs::kWebKitMinimumFontSize);

  double android_scale = ConvertFontSizeChromeToAndroid(
      default_size, default_fixed_size, minimum_size);

  base::DictionaryValue extras;
  extras.SetDouble("scale", android_scale);
  SendSettingsBroadcast("org.chromium.arc.intent_helper.SET_FONT_SCALE",
                        extras);
}

void ArcSettingsServiceImpl::SyncPageZoom() const {
  double zoom_level = profile_->GetZoomLevelPrefs()->GetDefaultZoomLevelPref();
  double zoom_factor = content::ZoomLevelToZoomFactor(zoom_level);

  base::DictionaryValue extras;
  extras.SetDouble("zoomFactor", zoom_factor);
  SendSettingsBroadcast("org.chromium.arc.intent_helper.SET_PAGE_ZOOM", extras);
}

void ArcSettingsServiceImpl::SyncLocale() const {
  const PrefService::Preference* pref =
      registrar_.prefs()->FindPreference(::prefs::kApplicationLocale);
  DCHECK(pref);
  std::string locale;
  bool value_exists = pref->GetValue()->GetAsString(&locale);
  DCHECK(value_exists);
  base::DictionaryValue extras;
  // Chrome OS locale may contain only the language part (e.g. fr) but country
  // code (e.g. fr_FR).  Since Android expects locale to contain country code,
  // ARC will derive a likely locale with country code from such.
  extras.SetString("locale", locale);
  const std::string preferredLanguages =
      registrar_.prefs()->GetString(::prefs::kLanguagePreferredLanguages);
  // |preferredLanguages| consists of comma separated locale strings. It may be
  // empty or contain empty items, but those are ignored on ARC.  If an item
  // has no country code, it is derived in ARC.  In such a case, it may
  // conflict with another item in the list, then these will be dedupped (the
  // first one is taken) in ARC.
  extras.SetString("preferredLanguages", preferredLanguages);
  SendSettingsBroadcast("org.chromium.arc.intent_helper.SET_LOCALE", extras);
}

void ArcSettingsServiceImpl::SyncLocationServiceEnabled() const {
  SendBoolPrefSettingsBroadcast(
      prefs::kArcLocationServiceEnabled,
      "org.chromium.arc.intent_helper.SET_LOCATION_SERVICE_ENABLED");
}

void ArcSettingsServiceImpl::SyncProxySettings() const {
  std::unique_ptr<ProxyConfigDictionary> proxy_config_dict =
      chromeos::ProxyConfigServiceImpl::GetActiveProxyConfigDictionary(
          GetPrefs(), g_browser_process->local_state());
  if (!proxy_config_dict)
    return;

  ProxyPrefs::ProxyMode mode;
  if (!proxy_config_dict || !proxy_config_dict->GetMode(&mode))
    mode = ProxyPrefs::MODE_DIRECT;

  base::DictionaryValue extras;
  extras.SetString("mode", ProxyPrefs::ProxyModeToString(mode));

  switch (mode) {
    case ProxyPrefs::MODE_DIRECT:
      break;
    case ProxyPrefs::MODE_SYSTEM:
      VLOG(1) << "The system mode is not translated.";
      return;
    case ProxyPrefs::MODE_AUTO_DETECT:
      extras.SetString("pacUrl", "http://wpad/wpad.dat");
      break;
    case ProxyPrefs::MODE_PAC_SCRIPT: {
      std::string pac_url;
      if (!proxy_config_dict->GetPacUrl(&pac_url)) {
        LOG(ERROR) << "No pac URL for pac_script proxy mode.";
        return;
      }
      extras.SetString("pacUrl", pac_url);
      break;
    }
    case ProxyPrefs::MODE_FIXED_SERVERS: {
      std::string host;
      int port = 0;
      if (!GetHttpProxyServer(proxy_config_dict.get(), &host, &port)) {
        LOG(ERROR) << "No Http proxy server is sent.";
        return;
      }
      extras.SetString("host", host);
      extras.SetInteger("port", port);

      std::string bypass_list;
      if (proxy_config_dict->GetBypassList(&bypass_list) &&
          !bypass_list.empty()) {
        extras.SetString("bypassList", bypass_list);
      }
      break;
    }
    default:
      LOG(ERROR) << "Incorrect proxy mode.";
      return;
  }

  SendSettingsBroadcast("org.chromium.arc.intent_helper.SET_PROXY", extras);
}

void ArcSettingsServiceImpl::SyncReportingConsent(bool initial_sync) const {
  bool consent = false;
  // Public session user never saw the consent even if the admin forced
  // the pref by a policy.
  if (!profiles::IsPublicSession())
    CrosSettings::Get()->GetBoolean(chromeos::kStatsReportingPref, &consent);
  if (consent && !initial_sync && policy_util::IsAccountManaged(profile_)) {
    // Don't enable reporting for managed users who might not have seen the
    // reporting notice during ARC setup.
    return;
  }
  base::DictionaryValue extras;
  extras.SetBoolean("reportingConsent", consent);
  SendSettingsBroadcast("org.chromium.arc.intent_helper.SET_REPORTING_CONSENT",
                        extras);
}

void ArcSettingsServiceImpl::SyncSelectToSpeakEnabled() const {
  SendBoolPrefSettingsBroadcast(
      ash::prefs::kAccessibilitySelectToSpeakEnabled,
      "org.chromium.arc.intent_helper.SET_SELECT_TO_SPEAK_ENABLED");
}

void ArcSettingsServiceImpl::SyncSmsConnectEnabled() const {
  // Only sync the preferences value if the feature flag is enabled, otherwise
  // sync a 'false' value.
  std::string action =
      std::string("org.chromium.arc.intent_helper.SET_SMS_CONNECT_ENABLED");
  if (!base::FeatureList::IsEnabled(features::kMultidevice)) {
    const PrefService::Preference* pref =
        registrar_.prefs()->FindPreference(prefs::kSmsConnectEnabled);
    DCHECK(pref);
    SendBoolValueSettingsBroadcast(false, pref->IsManaged(), action);
    return;
  }

  SendBoolPrefSettingsBroadcast(prefs::kSmsConnectEnabled, action);
}

void ArcSettingsServiceImpl::SyncSpokenFeedbackEnabled() const {
  SendBoolPrefSettingsBroadcast(
      ash::prefs::kAccessibilitySpokenFeedbackEnabled,
      "org.chromium.arc.intent_helper.SET_SPOKEN_FEEDBACK_ENABLED");
}

void ArcSettingsServiceImpl::SyncSwitchAccessEnabled() const {
  SendBoolPrefSettingsBroadcast(
      ash::prefs::kAccessibilitySwitchAccessEnabled,
      "org.chromium.arc.intent_helper.SET_SWITCH_ACCESS_ENABLED");
}

void ArcSettingsServiceImpl::SyncTimeZone() const {
  TimezoneSettings* timezone_settings = TimezoneSettings::GetInstance();
  base::string16 timezoneID = timezone_settings->GetCurrentTimezoneID();
  base::DictionaryValue extras;
  extras.SetString("olsonTimeZone", timezoneID);
  SendSettingsBroadcast("org.chromium.arc.intent_helper.SET_TIME_ZONE", extras);
}

void ArcSettingsServiceImpl::SyncTimeZoneByGeolocation() const {
  const PrefService::Preference* pref = registrar_.prefs()->FindPreference(
      ::prefs::kResolveTimezoneByGeolocationMethod);
  DCHECK(pref);
  int setTimeZoneByGeolocation =
      static_cast<int>(chromeos::system::TimeZoneResolverManager::
                           TimeZoneResolveMethod::DISABLED);
  bool value_exists = pref->GetValue()->GetAsInteger(&setTimeZoneByGeolocation);
  DCHECK(value_exists);
  base::DictionaryValue extras;
  extras.SetBoolean("autoTimeZone",
                    chromeos::system::TimeZoneResolverManager::
                            GetEffectiveUserTimeZoneResolveMethod(
                                registrar_.prefs(), false) !=
                        chromeos::system::TimeZoneResolverManager::
                            TimeZoneResolveMethod::DISABLED);
  SendSettingsBroadcast("org.chromium.arc.intent_helper.SET_AUTO_TIME_ZONE",
                        extras);
}

void ArcSettingsServiceImpl::SyncUse24HourClock() const {
  const PrefService::Preference* pref =
      registrar_.prefs()->FindPreference(::prefs::kUse24HourClock);
  DCHECK(pref);
  bool use24HourClock = false;
  bool value_exists = pref->GetValue()->GetAsBoolean(&use24HourClock);
  DCHECK(value_exists);
  base::DictionaryValue extras;
  extras.SetBoolean("use24HourClock", use24HourClock);
  SendSettingsBroadcast("org.chromium.arc.intent_helper.SET_USE_24_HOUR_CLOCK",
                        extras);
}

void ArcSettingsServiceImpl::AddPrefToObserve(const std::string& pref_name) {
  registrar_.Add(pref_name, base::Bind(&ArcSettingsServiceImpl::OnPrefChanged,
                                       base::Unretained(this)));
}

int ArcSettingsServiceImpl::GetIntegerPref(const std::string& pref_name) const {
  const PrefService::Preference* pref =
      registrar_.prefs()->FindPreference(pref_name);
  DCHECK(pref);
  int val = -1;
  bool value_exists = pref->GetValue()->GetAsInteger(&val);
  DCHECK(value_exists);
  return val;
}

bool ArcSettingsServiceImpl::IsBooleanPrefManaged(
    const std::string& pref_name) const {
  const PrefService::Preference* pref =
      registrar_.prefs()->FindPreference(pref_name);
  DCHECK(pref);
  bool value_exists = pref->GetValue()->is_bool();
  DCHECK(value_exists);
  return !pref->IsUserModifiable();
}

void ArcSettingsServiceImpl::SendBoolPrefSettingsBroadcast(
    const std::string& pref_name,
    const std::string& action) const {
  const PrefService::Preference* pref =
      registrar_.prefs()->FindPreference(pref_name);
  DCHECK(pref);
  bool enabled = false;
  bool value_exists = pref->GetValue()->GetAsBoolean(&enabled);
  DCHECK(value_exists);
  SendBoolValueSettingsBroadcast(enabled, !pref->IsUserModifiable(), action);
}

void ArcSettingsServiceImpl::SendBoolValueSettingsBroadcast(
    bool enabled,
    bool managed,
    const std::string& action) const {
  base::DictionaryValue extras;
  extras.SetBoolean("enabled", enabled);
  extras.SetBoolean("managed", managed);
  SendSettingsBroadcast(action, extras);
}

void ArcSettingsServiceImpl::SendSettingsBroadcast(
    const std::string& action,
    const base::DictionaryValue& extras) const {
  auto* instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service_->intent_helper(), SendBroadcast);
  if (!instance)
    return;
  std::string extras_json;
  bool write_success = base::JSONWriter::Write(extras, &extras_json);
  DCHECK(write_success);

  instance->SendBroadcast(
      action, ArcIntentHelperBridge::kArcIntentHelperPackageName,
      ArcIntentHelperBridge::AppendStringToIntentHelperPackageName(
          "SettingsReceiver"),
      extras_json);
}

// ConnectionObserver<mojom::AppInstance>:
void ArcSettingsServiceImpl::OnConnectionReady() {
  arc_bridge_service_->app()->RemoveObserver(this);
  SyncAppTimeSettings();
}

// static
ArcSettingsService* ArcSettingsService::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcSettingsServiceFactory::GetForBrowserContext(context);
}

ArcSettingsService::ArcSettingsService(content::BrowserContext* context,
                                       ArcBridgeService* bridge_service)
    : profile_(Profile::FromBrowserContext(context)),
      arc_bridge_service_(bridge_service) {
  arc_bridge_service_->intent_helper()->AddObserver(this);
  ArcSessionManager::Get()->AddObserver(this);

  if (!IsArcPlayStoreEnabledForProfile(profile_))
    SetInitialSettingsPending(false);
}

ArcSettingsService::~ArcSettingsService() {
  ArcSessionManager::Get()->RemoveObserver(this);
  arc_bridge_service_->intent_helper()->RemoveObserver(this);
}

void ArcSettingsService::OnConnectionReady() {
  impl_ =
      std::make_unique<ArcSettingsServiceImpl>(profile_, arc_bridge_service_);
  if (!IsInitialSettingsPending())
    return;
  impl_->SyncInitialSettings();
  SetInitialSettingsPending(false);
}

void ArcSettingsService::OnConnectionClosed() {
  impl_.reset();
}

void ArcSettingsService::OnArcPlayStoreEnabledChanged(bool enabled) {
  if (!enabled)
    SetInitialSettingsPending(false);
}

void ArcSettingsService::OnArcInitialStart() {
  DCHECK(!IsInitialSettingsPending());

  if (!impl_) {
    SetInitialSettingsPending(true);
    return;
  }

  impl_->SyncInitialSettings();
}

void ArcSettingsService::SetInitialSettingsPending(bool pending) {
  profile_->GetPrefs()->SetBoolean(prefs::kArcInitialSettingsPending, pending);
}

bool ArcSettingsService::IsInitialSettingsPending() const {
  return profile_->GetPrefs()->GetBoolean(prefs::kArcInitialSettingsPending);
}

}  // namespace arc
