// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/page_info/page_info.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/i18n/time_formatting.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browsing_data/browsing_data_channel_id_helper.h"
#include "chrome/browser/browsing_data/browsing_data_cookie_helper.h"
#include "chrome/browser/browsing_data/browsing_data_database_helper.h"
#include "chrome/browser/browsing_data/browsing_data_file_system_helper.h"
#include "chrome/browser/browsing_data/browsing_data_indexed_db_helper.h"
#include "chrome/browser/browsing_data/browsing_data_local_storage_helper.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/content_settings/local_shared_objects_container.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/permissions/chooser_context_base.h"
#include "chrome/browser/permissions/permission_decision_auto_blocker.h"
#include "chrome/browser/permissions/permission_manager.h"
#include "chrome/browser/permissions/permission_result.h"
#include "chrome/browser/permissions/permission_uma_util.h"
#include "chrome/browser/permissions/permission_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "chrome/browser/ssl/chrome_ssl_host_state_delegate.h"
#include "chrome/browser/ssl/chrome_ssl_host_state_delegate_factory.h"
#include "chrome/browser/ui/page_info/page_info_ui.h"
#include "chrome/browser/usb/usb_chooser_context.h"
#include "chrome/browser/usb/usb_chooser_context_factory.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/theme_resources.h"
#include "components/content_settings/core/browser/content_settings_registry.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/rappor/public/rappor_utils.h"
#include "components/rappor/rappor_service_impl.h"
#include "components/ssl_errors/error_info.h"
#include "components/strings/grit/components_chromium_strings.h"
#include "components/strings/grit/components_strings.h"
#include "components/subresource_filter/core/browser/subresource_filter_features.h"
#include "components/url_formatter/elide_url.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "net/cert/cert_status_flags.h"
#include "net/cert/x509_certificate.h"
#include "net/ssl/ssl_cipher_suite_names.h"
#include "net/ssl/ssl_connection_status_flags.h"
#include "third_party/boringssl/src/include/openssl/ssl.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/policy/policy_cert_service.h"
#include "chrome/browser/chromeos/policy/policy_cert_service_factory.h"
#endif

#if !defined(OS_ANDROID)
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/page_info/page_info_infobar_delegate.h"
#endif

#if defined(SAFE_BROWSING_DB_LOCAL)
#include "chrome/browser/safe_browsing/chrome_password_protection_service.h"
#endif

using base::ASCIIToUTF16;
using base::UTF8ToUTF16;
using base::UTF16ToUTF8;
using content::BrowserThread;

namespace {

// The list of content settings types to display on the Page Info UI. THE
// ORDER OF THESE ITEMS IS IMPORTANT and comes from https://crbug.com/610358. To
// propose changing it, email security-dev@chromium.org.
ContentSettingsType kPermissionType[] = {
    CONTENT_SETTINGS_TYPE_GEOLOCATION,
    CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA,
    CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC,
    CONTENT_SETTINGS_TYPE_SENSORS,
    CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
    CONTENT_SETTINGS_TYPE_JAVASCRIPT,
#if !defined(OS_ANDROID)
    CONTENT_SETTINGS_TYPE_PLUGINS,
    CONTENT_SETTINGS_TYPE_IMAGES,
#endif
    CONTENT_SETTINGS_TYPE_POPUPS,
    CONTENT_SETTINGS_TYPE_ADS,
    CONTENT_SETTINGS_TYPE_BACKGROUND_SYNC,
    CONTENT_SETTINGS_TYPE_SOUND,
    CONTENT_SETTINGS_TYPE_AUTOMATIC_DOWNLOADS,
    CONTENT_SETTINGS_TYPE_AUTOPLAY,
    CONTENT_SETTINGS_TYPE_MIDI_SYSEX,
    CONTENT_SETTINGS_TYPE_CLIPBOARD_READ,
    CONTENT_SETTINGS_TYPE_USB_GUARD,
};

// Checks whether this permission is currently the factory default, as set by
// Chrome. Specifically, that the following three conditions are true:
//   - The current active setting comes from the default or pref provider.
//   - The setting is the factory default setting (as opposed to a global
//     default setting set by the user).
//   - The setting is a wildcard setting applying to all origins (which can only
//     be set from the default provider).
bool IsPermissionFactoryDefault(HostContentSettingsMap* content_settings,
                                const PageInfoUI::PermissionInfo& info) {
  const ContentSetting factory_default_setting =
      content_settings::ContentSettingsRegistry::GetInstance()
          ->Get(info.type)
          ->GetInitialDefaultSetting();
  return (info.source == content_settings::SETTING_SOURCE_USER &&
          factory_default_setting == info.default_setting &&
          info.setting == CONTENT_SETTING_DEFAULT);
}

// Determines whether to show permission |type| in the Page Info UI. Only
// applies to permissions listed in |kPermissionType|.
bool ShouldShowPermission(
    const PageInfoUI::PermissionInfo& info,
    const GURL& site_url,
    HostContentSettingsMap* content_settings,
    content::WebContents* web_contents,
    TabSpecificContentSettings* tab_specific_content_settings) {
  // Note |CONTENT_SETTINGS_TYPE_ADS| will show up regardless of its default
  // value when it has been activated on the current origin.
  if (info.type == CONTENT_SETTINGS_TYPE_ADS) {
    if (!base::FeatureList::IsEnabled(
            subresource_filter::kSafeBrowsingSubresourceFilterExperimentalUI)) {
      return false;
    }

    // The setting for subresource filtering should not show up if the site is
    // not activated, both on android and desktop platforms.
    return content_settings->GetWebsiteSetting(
               site_url, GURL(), CONTENT_SETTINGS_TYPE_ADS_DATA, std::string(),
               nullptr) != nullptr;
  }

  if (info.type == CONTENT_SETTINGS_TYPE_SOUND) {
    if (!base::FeatureList::IsEnabled(features::kSoundContentSetting))
      return false;

    // The sound content setting should always show up when the tab has played
    // audio.
    if (web_contents && web_contents->WasEverAudible())
      return true;
  }

  if (info.type == CONTENT_SETTINGS_TYPE_CLIPBOARD_READ) {
    if (!base::FeatureList::IsEnabled(features::kClipboardContentSetting))
      return false;
  }

#if defined(OS_ANDROID)
  // Special geolocation DSE settings apply only on Android, so make sure it
  // gets checked there regardless of default setting on Desktop.
  if (info.type == CONTENT_SETTINGS_TYPE_GEOLOCATION)
    return true;
#else
  // Flash is shown if the user has ever changed its setting for |site_url|.
  if (info.type == CONTENT_SETTINGS_TYPE_PLUGINS &&
      content_settings->GetWebsiteSetting(site_url, site_url,
                                          CONTENT_SETTINGS_TYPE_PLUGINS_DATA,
                                          std::string(), nullptr) != nullptr) {
    return true;
  }
#endif

#if !defined(OS_ANDROID)
  // Autoplay is Android-only at the moment.
  if (info.type == CONTENT_SETTINGS_TYPE_AUTOPLAY)
    return false;
#endif

  // Show the content setting if it has been changed by the user since the last
  // page load.
  if (tab_specific_content_settings->HasContentSettingChangedViaPageInfo(
          info.type)) {
    return true;
  }

  // Show the content setting when it has a non-default value.
  if (!IsPermissionFactoryDefault(content_settings, info))
    return true;

  return false;
}

void CheckContentStatus(security_state::ContentStatus content_status,
                        bool* displayed,
                        bool* ran) {
  switch (content_status) {
    case security_state::CONTENT_STATUS_DISPLAYED:
      *displayed = true;
      break;
    case security_state::CONTENT_STATUS_RAN:
      *ran = true;
      break;
    case security_state::CONTENT_STATUS_DISPLAYED_AND_RAN:
      *displayed = true;
      *ran = true;
      break;
    case security_state::CONTENT_STATUS_UNKNOWN:
    case security_state::CONTENT_STATUS_NONE:
      break;
  }
}

// If the |security_info| indicates that mixed content or certificate errors
// were present, update |connection_status| and |connection_details|.
void ReportAnyInsecureContent(const security_state::SecurityInfo& security_info,
                              PageInfo::SiteConnectionStatus* connection_status,
                              base::string16* connection_details) {
  bool displayed_insecure_content = false;
  bool ran_insecure_content = false;
  CheckContentStatus(security_info.mixed_content_status,
                     &displayed_insecure_content, &ran_insecure_content);
  // Only note subresources with certificate errors if the main resource was
  // loaded without major certificate errors. If the main resource had a
  // certificate error, then it would not be that useful (and could
  // potentially be confusing) to warn about subresources that had certificate
  // errors too.
  if (!net::IsCertStatusError(security_info.cert_status) ||
      net::IsCertStatusMinorError(security_info.cert_status)) {
    CheckContentStatus(security_info.content_with_cert_errors_status,
                       &displayed_insecure_content, &ran_insecure_content);
  }

  // Only one insecure content warning is displayed; show the most severe.
  if (ran_insecure_content) {
    *connection_status =
        PageInfo::SITE_CONNECTION_STATUS_INSECURE_ACTIVE_SUBRESOURCE;
    connection_details->assign(l10n_util::GetStringFUTF16(
        IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTED_SENTENCE_LINK, *connection_details,
        l10n_util::GetStringUTF16(
            IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTED_INSECURE_CONTENT_ERROR)));
    return;
  }
  if (security_info.contained_mixed_form) {
    *connection_status = PageInfo::SITE_CONNECTION_STATUS_INSECURE_FORM_ACTION;
    connection_details->assign(l10n_util::GetStringFUTF16(
        IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTED_SENTENCE_LINK, *connection_details,
        l10n_util::GetStringUTF16(
            IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTED_INSECURE_FORM_WARNING)));
    return;
  }
  if (displayed_insecure_content) {
    *connection_status =
        PageInfo::SITE_CONNECTION_STATUS_INSECURE_PASSIVE_SUBRESOURCE;
    connection_details->assign(l10n_util::GetStringFUTF16(
        IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTED_SENTENCE_LINK, *connection_details,
        l10n_util::GetStringUTF16(
            IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTED_INSECURE_CONTENT_WARNING)));
  }
}

base::string16 GetSimpleSiteName(const GURL& url) {
  return url_formatter::FormatUrlForSecurityDisplay(
      url, url_formatter::SchemeDisplay::OMIT_HTTP_AND_HTTPS);
}

ChooserContextBase* GetUsbChooserContext(Profile* profile) {
  return UsbChooserContextFactory::GetForProfile(profile);
}

// The list of chooser types that need to display entries in the Website
// Settings UI. THE ORDER OF THESE ITEMS IS IMPORTANT. To propose changing it,
// email security-dev@chromium.org.
const PageInfo::ChooserUIInfo kChooserUIInfo[] = {
    {CONTENT_SETTINGS_TYPE_USB_CHOOSER_DATA, &GetUsbChooserContext,
     IDS_PAGE_INFO_USB_DEVICE_LABEL, IDS_PAGE_INFO_USB_DEVICE_SECONDARY_LABEL,
     IDS_PAGE_INFO_DELETE_USB_DEVICE, "name"},
};

}  // namespace

PageInfo::PageInfo(PageInfoUI* ui,
                   Profile* profile,
                   TabSpecificContentSettings* tab_specific_content_settings,
                   content::WebContents* web_contents,
                   const GURL& url,
                   const security_state::SecurityInfo& security_info)
    : TabSpecificContentSettings::SiteDataObserver(
          tab_specific_content_settings),
      content::WebContentsObserver(web_contents),
      ui_(ui),
      show_info_bar_(false),
      site_url_(url),
      site_identity_status_(SITE_IDENTITY_STATUS_UNKNOWN),
      site_connection_status_(SITE_CONNECTION_STATUS_UNKNOWN),
      show_ssl_decision_revoke_button_(false),
      content_settings_(HostContentSettingsMapFactory::GetForProfile(profile)),
      chrome_ssl_host_state_delegate_(
          ChromeSSLHostStateDelegateFactory::GetForProfile(profile)),
      did_revoke_user_ssl_decisions_(false),
      profile_(profile),
      security_level_(security_state::NONE),
#if defined(SAFE_BROWSING_DB_LOCAL)
      password_protection_service_(
          safe_browsing::ChromePasswordProtectionService::
              GetPasswordProtectionService(profile_)),
#endif
      show_change_password_buttons_(false) {
  Init(url, security_info);

  PresentSitePermissions();
  PresentSiteIdentity();
  PresentSiteData();

  // Every time the Page Info UI is opened a |PageInfo| object is
  // created. So this counts how ofter the Page Info UI is opened.
  RecordPageInfoAction(PAGE_INFO_OPENED);
}

PageInfo::~PageInfo() {
  // Check if Re-enable warnings button was visible, if so, log on UMA whether
  // it was clicked or not.
  SSLCertificateDecisionsDidRevoke user_decision =
      did_revoke_user_ssl_decisions_ ? USER_CERT_DECISIONS_REVOKED
                                     : USER_CERT_DECISIONS_NOT_REVOKED;
  if (show_ssl_decision_revoke_button_) {
    UMA_HISTOGRAM_ENUMERATION("interstitial.ssl.did_user_revoke_decisions2",
                              user_decision,
                              END_OF_SSL_CERTIFICATE_DECISIONS_DID_REVOKE_ENUM);
  }
}

void PageInfo::RecordPageInfoAction(PageInfoAction action) {
  UMA_HISTOGRAM_ENUMERATION("WebsiteSettings.Action", action, PAGE_INFO_COUNT);

  std::string histogram_name;
  if (site_url_.SchemeIsCryptographic()) {
    if (security_level_ == security_state::SECURE) {
      UMA_HISTOGRAM_ENUMERATION("Security.PageInfo.Action.HttpsUrl.ValidNonEV",
                                action, PAGE_INFO_COUNT);
    } else if (security_level_ == security_state::EV_SECURE) {
      UMA_HISTOGRAM_ENUMERATION("Security.PageInfo.Action.HttpsUrl.ValidEV",
                                action, PAGE_INFO_COUNT);
    } else if (security_level_ == security_state::NONE) {
      UMA_HISTOGRAM_ENUMERATION("Security.PageInfo.Action.HttpsUrl.Downgraded",
                                action, PAGE_INFO_COUNT);
    } else if (security_level_ == security_state::DANGEROUS) {
      UMA_HISTOGRAM_ENUMERATION("Security.PageInfo.Action.HttpsUrl.Dangerous",
                                action, PAGE_INFO_COUNT);
    }
    return;
  }

  if (security_level_ == security_state::HTTP_SHOW_WARNING) {
    UMA_HISTOGRAM_ENUMERATION("Security.PageInfo.Action.HttpUrl.Warning",
                              action, PAGE_INFO_COUNT);
  } else if (security_level_ == security_state::DANGEROUS) {
    UMA_HISTOGRAM_ENUMERATION("Security.PageInfo.Action.HttpUrl.Dangerous",
                              action, PAGE_INFO_COUNT);
  } else {
    UMA_HISTOGRAM_ENUMERATION("Security.PageInfo.Action.HttpUrl.Neutral",
                              action, PAGE_INFO_COUNT);
  }
}

void PageInfo::OnSitePermissionChanged(ContentSettingsType type,
                                       ContentSetting setting) {
  tab_specific_content_settings()->ContentSettingChangedViaPageInfo(type);

  // Count how often a permission for a specific content type is changed using
  // the Page Info UI.
  size_t num_values;
  int histogram_value = ContentSettingTypeToHistogramValue(type, &num_values);
  UMA_HISTOGRAM_EXACT_LINEAR("WebsiteSettings.OriginInfo.PermissionChanged",
                             histogram_value, num_values);

  if (setting == ContentSetting::CONTENT_SETTING_ALLOW) {
    UMA_HISTOGRAM_EXACT_LINEAR(
        "WebsiteSettings.OriginInfo.PermissionChanged.Allowed", histogram_value,
        num_values);

    if (type == CONTENT_SETTINGS_TYPE_PLUGINS) {
      rappor::SampleDomainAndRegistryFromGURL(
          g_browser_process->rappor_service(),
          "ContentSettings.Plugins.AddedAllowException", site_url_);
    }
  } else if (setting == ContentSetting::CONTENT_SETTING_BLOCK) {
    UMA_HISTOGRAM_EXACT_LINEAR(
        "WebsiteSettings.OriginInfo.PermissionChanged.Blocked", histogram_value,
        num_values);
  }

  // This is technically redundant given the histogram above, but putting the
  // total count of permission changes in another histogram makes it easier to
  // compare it against other kinds of actions in Page Info.
  RecordPageInfoAction(PAGE_INFO_CHANGED_PERMISSION);
  if (type == CONTENT_SETTINGS_TYPE_SOUND) {
    ContentSetting default_setting =
        content_settings_->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_SOUND,
                                                    nullptr);
    bool mute = (setting == CONTENT_SETTING_BLOCK) ||
                (setting == CONTENT_SETTING_DEFAULT &&
                 default_setting == CONTENT_SETTING_BLOCK);
    if (mute) {
      base::RecordAction(
          base::UserMetricsAction("SoundContentSetting.MuteBy.PageInfo"));
    } else {
      base::RecordAction(
          base::UserMetricsAction("SoundContentSetting.UnmuteBy.PageInfo"));
    }
  }

  PermissionUtil::ScopedRevocationReporter scoped_revocation_reporter(
      profile_, site_url_, site_url_, type, PermissionSourceUI::OIB);

  // The permission may have been blocked due to being under embargo, so if it
  // was changed away from BLOCK, clear embargo status if it exists.
  if (setting != CONTENT_SETTING_BLOCK) {
    PermissionDecisionAutoBlocker::GetForProfile(profile_)->RemoveEmbargoByUrl(
        site_url_, type);
  }
  content_settings_->SetNarrowestContentSetting(site_url_, site_url_, type,
                                                setting);

  // When the sound setting is changed, no reload is necessary.
  if (type != CONTENT_SETTINGS_TYPE_SOUND)
    show_info_bar_ = true;

  // Refresh the UI to reflect the new setting.
  PresentSitePermissions();
}

void PageInfo::OnSiteChosenObjectDeleted(const ChooserUIInfo& ui_info,
                                         const base::DictionaryValue& object) {
  // TODO(reillyg): Create metrics for revocations. crbug.com/556845
  ChooserContextBase* context = ui_info.get_context(profile_);
  const GURL origin = site_url_.GetOrigin();
  context->RevokeObjectPermission(origin, origin, object);
  show_info_bar_ = true;

  // Refresh the UI to reflect the changed settings.
  PresentSitePermissions();
}

void PageInfo::OnSiteDataAccessed() {
  PresentSiteData();
}

void PageInfo::OnUIClosing() {
#if defined(OS_ANDROID)
  NOTREACHED();
#else
  if (show_info_bar_ && web_contents() && !web_contents()->IsBeingDestroyed()) {
    InfoBarService* infobar_service =
        InfoBarService::FromWebContents(web_contents());
    if (infobar_service)
      PageInfoInfoBarDelegate::Create(infobar_service);
  }
#endif
}

void PageInfo::OnRevokeSSLErrorBypassButtonPressed() {
  DCHECK(chrome_ssl_host_state_delegate_);
  chrome_ssl_host_state_delegate_->RevokeUserAllowExceptionsHard(
      site_url().host());
  did_revoke_user_ssl_decisions_ = true;
}

void PageInfo::OpenSiteSettingsView() {
#if defined(OS_ANDROID)
  NOTREACHED();
#else
  chrome::ShowSiteSettings(chrome::FindBrowserWithWebContents(web_contents()),
                           site_url());
  RecordPageInfoAction(PageInfo::PAGE_INFO_SITE_SETTINGS_OPENED);
#endif
}

void PageInfo::OnChangePasswordButtonPressed(
    content::WebContents* web_contents) {
#if defined(SAFE_BROWSING_DB_LOCAL)
  DCHECK(password_protection_service_);
  password_protection_service_->OnUserAction(
      web_contents, safe_browsing::PasswordProtectionService::PAGE_INFO,
      safe_browsing::PasswordProtectionService::CHANGE_PASSWORD);
#endif
}

void PageInfo::OnWhitelistPasswordReuseButtonPressed(
    content::WebContents* web_contents) {
#if defined(SAFE_BROWSING_DB_LOCAL)
  DCHECK(password_protection_service_);
  password_protection_service_->OnUserAction(
      web_contents, safe_browsing::PasswordProtectionService::PAGE_INFO,
      safe_browsing::PasswordProtectionService::MARK_AS_LEGITIMATE);
#endif
}

void PageInfo::Init(const GURL& url,
                    const security_state::SecurityInfo& security_info) {
#if !defined(OS_ANDROID)
  // On desktop, internal URLs aren't handled by this class. Instead, a
  // custom and simpler bubble is shown.
  DCHECK(!url.SchemeIs(content::kChromeUIScheme) &&
         !url.SchemeIs(content::kChromeDevToolsScheme) &&
         !url.SchemeIs(content::kViewSourceScheme) &&
         !url.SchemeIs(content_settings::kExtensionScheme));
#endif

  bool is_chrome_ui_native_scheme = false;
#if defined(OS_ANDROID)
  is_chrome_ui_native_scheme = url.SchemeIs(chrome::kChromeUINativeScheme);
#endif

  security_level_ = security_info.security_level;

  if (url.SchemeIs(url::kAboutScheme)) {
    // All about: URLs except about:blank are redirected.
    DCHECK_EQ(url::kAboutBlankURL, url.spec());
    site_identity_status_ = SITE_IDENTITY_STATUS_NO_CERT;
    site_identity_details_ =
        l10n_util::GetStringUTF16(IDS_PAGE_INFO_SECURITY_TAB_INSECURE_IDENTITY);
    site_connection_status_ = SITE_CONNECTION_STATUS_UNENCRYPTED;
    site_connection_details_ = l10n_util::GetStringFUTF16(
        IDS_PAGE_INFO_SECURITY_TAB_NOT_ENCRYPTED_CONNECTION_TEXT,
        UTF8ToUTF16(url.spec()));
    return;
  }

  if (url.SchemeIs(content::kChromeUIScheme) || is_chrome_ui_native_scheme) {
    site_identity_status_ = SITE_IDENTITY_STATUS_INTERNAL_PAGE;
    site_identity_details_ =
        l10n_util::GetStringUTF16(IDS_PAGE_INFO_INTERNAL_PAGE);
    site_connection_status_ = SITE_CONNECTION_STATUS_INTERNAL_PAGE;
    return;
  }

  // Identity section.
  certificate_ = security_info.certificate;

  if (security_info.malicious_content_status !=
      security_state::MALICIOUS_CONTENT_STATUS_NONE) {
    // The site has been flagged by Safe Browsing as dangerous.
    GetSiteIdentityByMaliciousContentStatus(
        security_info.malicious_content_status, &site_identity_status_,
        &site_identity_details_);
    show_change_password_buttons_ =
        security_info.malicious_content_status ==
        security_state::MALICIOUS_CONTENT_STATUS_PASSWORD_REUSE;
  } else if (certificate_ &&
             (!net::IsCertStatusError(security_info.cert_status) ||
              net::IsCertStatusMinorError(security_info.cert_status))) {
    // HTTPS with no or minor errors.
    if (security_info.security_level ==
        security_state::SECURE_WITH_POLICY_INSTALLED_CERT) {
#if defined(OS_CHROMEOS)
      site_identity_status_ = SITE_IDENTITY_STATUS_ADMIN_PROVIDED_CERT;
      site_identity_details_ = l10n_util::GetStringFUTF16(
          IDS_CERT_POLICY_PROVIDED_CERT_MESSAGE, UTF8ToUTF16(url.host()));
#else
      DCHECK(false) << "Policy certificates exist only on ChromeOS";
#endif
    } else if (net::IsCertStatusMinorError(security_info.cert_status)) {
      site_identity_status_ = SITE_IDENTITY_STATUS_CERT_REVOCATION_UNKNOWN;
      base::string16 issuer_name(
          UTF8ToUTF16(certificate_->issuer().GetDisplayName()));
      if (issuer_name.empty()) {
        issuer_name.assign(l10n_util::GetStringUTF16(
            IDS_PAGE_INFO_SECURITY_TAB_UNKNOWN_PARTY));
      }

      site_identity_details_.assign(l10n_util::GetStringFUTF16(
          IDS_PAGE_INFO_SECURITY_TAB_SECURE_IDENTITY_VERIFIED, issuer_name));

      site_identity_details_ += ASCIIToUTF16("\n\n");
      if (security_info.cert_status &
          net::CERT_STATUS_UNABLE_TO_CHECK_REVOCATION) {
        site_identity_details_ += l10n_util::GetStringUTF16(
            IDS_PAGE_INFO_SECURITY_TAB_UNABLE_TO_CHECK_REVOCATION);
      } else if (security_info.cert_status &
                 net::CERT_STATUS_NO_REVOCATION_MECHANISM) {
        site_identity_details_ += l10n_util::GetStringUTF16(
            IDS_PAGE_INFO_SECURITY_TAB_NO_REVOCATION_MECHANISM);
      } else {
        NOTREACHED() << "Need to specify string for this warning";
      }
    } else {
      // No major or minor errors.
      if (security_info.cert_status & net::CERT_STATUS_IS_EV) {
        // EV HTTPS page.
        site_identity_status_ = SITE_IDENTITY_STATUS_EV_CERT;
        DCHECK(!certificate_->subject().organization_names.empty());
        organization_name_ =
            UTF8ToUTF16(certificate_->subject().organization_names[0]);
        // An EV Cert is required to have a city (localityName) and country but
        // state is "if any".
        DCHECK(!certificate_->subject().locality_name.empty());
        DCHECK(!certificate_->subject().country_name.empty());
        base::string16 locality;
        if (!certificate_->subject().state_or_province_name.empty()) {
          locality = l10n_util::GetStringFUTF16(
              IDS_PAGE_INFO_ADDRESS,
              UTF8ToUTF16(certificate_->subject().locality_name),
              UTF8ToUTF16(certificate_->subject().state_or_province_name),
              UTF8ToUTF16(certificate_->subject().country_name));
        } else {
          locality = l10n_util::GetStringFUTF16(
              IDS_PAGE_INFO_PARTIAL_ADDRESS,
              UTF8ToUTF16(certificate_->subject().locality_name),
              UTF8ToUTF16(certificate_->subject().country_name));
        }
        DCHECK(!certificate_->subject().organization_names.empty());
        site_identity_details_.assign(l10n_util::GetStringFUTF16(
            IDS_PAGE_INFO_SECURITY_TAB_SECURE_IDENTITY_EV_VERIFIED,
            UTF8ToUTF16(certificate_->subject().organization_names[0]),
            locality, UTF8ToUTF16(certificate_->issuer().GetDisplayName())));
      } else {
        // Non-EV OK HTTPS page.
        site_identity_status_ = SITE_IDENTITY_STATUS_CERT;
        base::string16 issuer_name(
            UTF8ToUTF16(certificate_->issuer().GetDisplayName()));
        if (issuer_name.empty()) {
          issuer_name.assign(l10n_util::GetStringUTF16(
              IDS_PAGE_INFO_SECURITY_TAB_UNKNOWN_PARTY));
        }

        site_identity_details_.assign(l10n_util::GetStringFUTF16(
            IDS_PAGE_INFO_SECURITY_TAB_SECURE_IDENTITY_VERIFIED, issuer_name));
      }
      if (security_info.sha1_in_chain) {
        site_identity_status_ =
            SITE_IDENTITY_STATUS_DEPRECATED_SIGNATURE_ALGORITHM;
        site_identity_details_ +=
            UTF8ToUTF16("\n\n") +
            l10n_util::GetStringUTF16(
                IDS_PAGE_INFO_SECURITY_TAB_DEPRECATED_SIGNATURE_ALGORITHM);
      }
    }
  } else {
    // HTTP or HTTPS with errors (not warnings).
    site_identity_details_.assign(l10n_util::GetStringUTF16(
        IDS_PAGE_INFO_SECURITY_TAB_INSECURE_IDENTITY));
    if (!security_info.scheme_is_cryptographic || !security_info.certificate)
      site_identity_status_ = SITE_IDENTITY_STATUS_NO_CERT;
    else
      site_identity_status_ = SITE_IDENTITY_STATUS_ERROR;

    const base::string16 bullet = UTF8ToUTF16("\n • ");
    std::vector<ssl_errors::ErrorInfo> errors;
    ssl_errors::ErrorInfo::GetErrorsForCertStatus(
        certificate_, security_info.cert_status, url, &errors);
    for (size_t i = 0; i < errors.size(); ++i) {
      site_identity_details_ += bullet;
      site_identity_details_ += errors[i].short_description();
    }

    if (security_info.cert_status & net::CERT_STATUS_NON_UNIQUE_NAME) {
      site_identity_details_ += ASCIIToUTF16("\n\n");
      site_identity_details_ +=
          l10n_util::GetStringUTF16(IDS_PAGE_INFO_SECURITY_TAB_NON_UNIQUE_NAME);
    }
  }

  // Site Connection
  // We consider anything less than 80 bits encryption to be weak encryption.
  // TODO(wtc): Bug 1198735: report mixed/unsafe content for unencrypted and
  // weakly encrypted connections.
  site_connection_status_ = SITE_CONNECTION_STATUS_UNKNOWN;

  base::string16 subject_name(GetSimpleSiteName(url));
  if (subject_name.empty()) {
    subject_name.assign(
        l10n_util::GetStringUTF16(IDS_PAGE_INFO_SECURITY_TAB_UNKNOWN_PARTY));
  }

  if (!security_info.certificate || !security_info.scheme_is_cryptographic) {
    // Page is still loading (so SSL status is not yet available) or
    // loaded over HTTP or loaded over HTTPS with no cert.
    site_connection_status_ = SITE_CONNECTION_STATUS_UNENCRYPTED;

    site_connection_details_.assign(l10n_util::GetStringFUTF16(
        IDS_PAGE_INFO_SECURITY_TAB_NOT_ENCRYPTED_CONNECTION_TEXT,
        subject_name));
  } else if (security_info.security_bits < 0) {
    // Security strength is unknown.  Say nothing.
    site_connection_status_ = SITE_CONNECTION_STATUS_ENCRYPTED_ERROR;
  } else if (security_info.security_bits == 0) {
    DCHECK_NE(security_info.security_level, security_state::NONE);
    site_connection_status_ = SITE_CONNECTION_STATUS_ENCRYPTED_ERROR;
    site_connection_details_.assign(l10n_util::GetStringFUTF16(
        IDS_PAGE_INFO_SECURITY_TAB_NOT_ENCRYPTED_CONNECTION_TEXT,
        subject_name));
  } else {
    site_connection_status_ = SITE_CONNECTION_STATUS_ENCRYPTED;

    if (security_info.obsolete_ssl_status == net::OBSOLETE_SSL_NONE) {
      site_connection_details_.assign(l10n_util::GetStringFUTF16(
          IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTED_CONNECTION_TEXT, subject_name));
    } else {
      site_connection_details_.assign(l10n_util::GetStringFUTF16(
          IDS_PAGE_INFO_SECURITY_TAB_WEAK_ENCRYPTION_CONNECTION_TEXT,
          subject_name));
    }

    ReportAnyInsecureContent(security_info, &site_connection_status_,
                             &site_connection_details_);
  }

  uint16_t cipher_suite =
      net::SSLConnectionStatusToCipherSuite(security_info.connection_status);
  if (security_info.security_bits > 0 && cipher_suite) {
    int ssl_version =
        net::SSLConnectionStatusToVersion(security_info.connection_status);
    const char* ssl_version_str;
    net::SSLVersionToString(&ssl_version_str, ssl_version);
    site_connection_details_ += ASCIIToUTF16("\n\n");
    site_connection_details_ += l10n_util::GetStringFUTF16(
        IDS_PAGE_INFO_SECURITY_TAB_SSL_VERSION, ASCIIToUTF16(ssl_version_str));

    const char *key_exchange, *cipher, *mac;
    bool is_aead, is_tls13;
    net::SSLCipherSuiteToStrings(&key_exchange, &cipher, &mac, &is_aead,
                                 &is_tls13, cipher_suite);

    site_connection_details_ += ASCIIToUTF16("\n\n");
    if (is_aead) {
      if (is_tls13) {
        // For TLS 1.3 ciphers, report the group (historically, curve) as the
        // key exchange.
        key_exchange = SSL_get_curve_name(security_info.key_exchange_group);
        if (!key_exchange) {
          NOTREACHED();
          key_exchange = "";
        }
      }
      site_connection_details_ += l10n_util::GetStringFUTF16(
          IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTION_DETAILS_AEAD,
          ASCIIToUTF16(cipher), ASCIIToUTF16(key_exchange));
    } else {
      site_connection_details_ += l10n_util::GetStringFUTF16(
          IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTION_DETAILS, ASCIIToUTF16(cipher),
          ASCIIToUTF16(mac), ASCIIToUTF16(key_exchange));
    }
  }

  // Check if a user decision has been made to allow or deny certificates with
  // errors on this site.
  ChromeSSLHostStateDelegate* delegate =
      ChromeSSLHostStateDelegateFactory::GetForProfile(profile_);
  DCHECK(delegate);
  // Only show an SSL decision revoke button if the user has chosen to bypass
  // SSL host errors for this host in the past.
  show_ssl_decision_revoke_button_ = delegate->HasAllowException(url.host());
}

void PageInfo::PresentSitePermissions() {
  PermissionInfoList permission_info_list;
  ChosenObjectInfoList chosen_object_info_list;

  PageInfoUI::PermissionInfo permission_info;
  for (size_t i = 0; i < arraysize(kPermissionType); ++i) {
    permission_info.type = kPermissionType[i];

    content_settings::SettingInfo info;
    std::unique_ptr<base::Value> value = content_settings_->GetWebsiteSetting(
        site_url_, site_url_, permission_info.type, std::string(), &info);
    DCHECK(value.get());
    if (value->type() == base::Value::Type::INTEGER) {
      permission_info.setting =
          content_settings::ValueToContentSetting(value.get());
    } else {
      NOTREACHED();
    }

    permission_info.source = info.source;
    permission_info.is_incognito = profile_->IsOffTheRecord();

    if (info.primary_pattern == ContentSettingsPattern::Wildcard() &&
        info.secondary_pattern == ContentSettingsPattern::Wildcard()) {
      permission_info.default_setting = permission_info.setting;
      permission_info.setting = CONTENT_SETTING_DEFAULT;
    } else {
      permission_info.default_setting =
          content_settings_->GetDefaultContentSetting(permission_info.type,
                                                      NULL);
    }

    // For permissions that are still prompting the user and haven't been
    // explicitly set by another source, check its embargo status.
    if (PermissionUtil::IsPermission(permission_info.type) &&
        permission_info.setting == CONTENT_SETTING_DEFAULT &&
        permission_info.source ==
            content_settings::SettingSource::SETTING_SOURCE_USER) {
      // TODO(raymes): Use GetPermissionStatus() to retrieve information
      // about *all* permissions once it has default behaviour implemented for
      // ContentSettingTypes that aren't permissions.
      PermissionResult permission_result =
          PermissionManager::Get(profile_)->GetPermissionStatus(
              permission_info.type, site_url_, site_url_);

      // If under embargo, update |permission_info| to reflect that.
      if (permission_result.content_setting == CONTENT_SETTING_BLOCK &&
          permission_result.source ==
              PermissionStatusSource::MULTIPLE_DISMISSALS) {
        permission_info.setting = permission_result.content_setting;
      }
    }

    if (ShouldShowPermission(permission_info, site_url_, content_settings_,
                             web_contents(), tab_specific_content_settings())) {
      permission_info_list.push_back(permission_info);
    }
  }

  for (const ChooserUIInfo& ui_info : kChooserUIInfo) {
    ChooserContextBase* context = ui_info.get_context(profile_);
    const GURL origin = site_url_.GetOrigin();

    // Hide individual object permissions because when the chooser is blocked
    // previously granted device permissions are also ignored.
    if (!context->CanRequestObjectPermission(origin, origin))
      continue;

    auto chosen_objects = context->GetGrantedObjects(origin, origin);
    for (std::unique_ptr<base::DictionaryValue>& object : chosen_objects) {
      chosen_object_info_list.push_back(
          std::make_unique<PageInfoUI::ChosenObjectInfo>(ui_info,
                                                         std::move(object)));
    }
  }

  ui_->SetPermissionInfo(permission_info_list,
                         std::move(chosen_object_info_list));
}

void PageInfo::PresentSiteData() {
  CookieInfoList cookie_info_list;
  const LocalSharedObjectsContainer& allowed_objects =
      tab_specific_content_settings()->allowed_local_shared_objects();
  const LocalSharedObjectsContainer& blocked_objects =
      tab_specific_content_settings()->blocked_local_shared_objects();

  // Add first party cookie and site data counts.
  PageInfoUI::CookieInfo cookie_info;
  cookie_info.allowed = allowed_objects.GetObjectCountForDomain(site_url_);
  cookie_info.blocked = blocked_objects.GetObjectCountForDomain(site_url_);
  cookie_info.is_first_party = true;
  cookie_info_list.push_back(cookie_info);

  // Add third party cookie counts.
  cookie_info.allowed = allowed_objects.GetObjectCount() - cookie_info.allowed;
  cookie_info.blocked = blocked_objects.GetObjectCount() - cookie_info.blocked;
  cookie_info.is_first_party = false;
  cookie_info_list.push_back(cookie_info);

  ui_->SetCookieInfo(cookie_info_list);
}

void PageInfo::PresentSiteIdentity() {
  // After initialization the status about the site's connection and its
  // identity must be available.
  DCHECK_NE(site_identity_status_, SITE_IDENTITY_STATUS_UNKNOWN);
  DCHECK_NE(site_connection_status_, SITE_CONNECTION_STATUS_UNKNOWN);
  PageInfoUI::IdentityInfo info;
  if (site_identity_status_ == SITE_IDENTITY_STATUS_EV_CERT)
    info.site_identity = UTF16ToUTF8(organization_name());
  else
    info.site_identity = UTF16ToUTF8(GetSimpleSiteName(site_url_));

  info.connection_status = site_connection_status_;
  info.connection_status_description = UTF16ToUTF8(site_connection_details_);
  info.identity_status = site_identity_status_;
  info.identity_status_description = UTF16ToUTF8(site_identity_details_);
  info.certificate = certificate_;
  info.show_ssl_decision_revoke_button = show_ssl_decision_revoke_button_;
  info.show_change_password_buttons = show_change_password_buttons_;
  ui_->SetIdentityInfo(info);
#if defined(SAFE_BROWSING_DB_LOCAL)
  if (password_protection_service_ && show_change_password_buttons_) {
    password_protection_service_->RecordWarningAction(
        safe_browsing::PasswordProtectionService::PAGE_INFO,
        safe_browsing::PasswordProtectionService::SHOWN);
  }
#endif
}

std::vector<ContentSettingsType> PageInfo::GetAllPermissionsForTesting() {
  std::vector<ContentSettingsType> permission_list;
  for (size_t i = 0; i < arraysize(kPermissionType); ++i) {
#if !defined(OS_ANDROID)
    if (kPermissionType[i] == CONTENT_SETTINGS_TYPE_AUTOPLAY)
      continue;
#endif
    permission_list.push_back(kPermissionType[i]);
  }
  return permission_list;
}

void PageInfo::GetSiteIdentityByMaliciousContentStatus(
    security_state::MaliciousContentStatus malicious_content_status,
    PageInfo::SiteIdentityStatus* status,
    base::string16* details) {
  switch (malicious_content_status) {
    case security_state::MALICIOUS_CONTENT_STATUS_NONE:
      NOTREACHED();
      break;
    case security_state::MALICIOUS_CONTENT_STATUS_MALWARE:
      *status = PageInfo::SITE_IDENTITY_STATUS_MALWARE;
      *details = l10n_util::GetStringUTF16(IDS_PAGE_INFO_MALWARE_DETAILS);
      break;
    case security_state::MALICIOUS_CONTENT_STATUS_SOCIAL_ENGINEERING:
      *status = PageInfo::SITE_IDENTITY_STATUS_SOCIAL_ENGINEERING;
      *details =
          l10n_util::GetStringUTF16(IDS_PAGE_INFO_SOCIAL_ENGINEERING_DETAILS);
      break;
    case security_state::MALICIOUS_CONTENT_STATUS_UNWANTED_SOFTWARE:
      *status = PageInfo::SITE_IDENTITY_STATUS_UNWANTED_SOFTWARE;
      *details =
          l10n_util::GetStringUTF16(IDS_PAGE_INFO_UNWANTED_SOFTWARE_DETAILS);
      break;
    case security_state::MALICIOUS_CONTENT_STATUS_PASSWORD_REUSE:
#if defined(SAFE_BROWSING_DB_LOCAL)
      *status = PageInfo::SITE_IDENTITY_STATUS_PASSWORD_REUSE;
      *details = safe_browsing::ChromePasswordProtectionService::
                     GetPasswordProtectionService(profile_)
                         ->GetWarningDetailText();
#endif
      break;
  }
}
