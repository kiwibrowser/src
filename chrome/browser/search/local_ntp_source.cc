// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/local_ntp_source.h"

#include "base/base64.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/json/json_string_value_serializer.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/background/ntp_background_data.h"
#include "chrome/browser/search/background/ntp_background_service.h"
#include "chrome/browser/search/background/ntp_background_service_factory.h"
#include "chrome/browser/search/instant_io_context.h"
#include "chrome/browser/search/local_files_ntp_source.h"
#include "chrome/browser/search/local_ntp_js_integrity.h"
#include "chrome/browser/search/one_google_bar/one_google_bar_data.h"
#include "chrome/browser/search/one_google_bar/one_google_bar_service.h"
#include "chrome/browser/search/one_google_bar/one_google_bar_service_factory.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/search_provider_logos/logo_service_factory.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_observer.h"
#include "components/search_provider_logos/logo_common.h"
#include "components/search_provider_logos/logo_service.h"
#include "components/search_provider_logos/logo_tracker.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/url_util.h"
#include "net/url_request/url_request.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/resources/grit/ui_resources.h"
#include "url/gurl.h"

using search_provider_logos::EncodedLogo;
using search_provider_logos::EncodedLogoCallback;
using search_provider_logos::LogoCallbacks;
using search_provider_logos::LogoCallbackReason;
using search_provider_logos::LogoMetadata;
using search_provider_logos::LogoService;

namespace {

// Signifies a locally constructed resource, i.e. not from grit/.
const int kLocalResource = -1;

const char kConfigDataFilename[] = "config.js";
const char kThemeCSSFilename[] = "theme.css";
const char kMainHtmlFilename[] = "local-ntp.html";
const char kNtpBackgroundCollectionScriptFilename[] =
    "ntp-background-collections.js";
const char kNtpBackgroundImageScriptFilename[] = "ntp-background-images.js";
const char kOneGoogleBarScriptFilename[] = "one-google.js";
const char kDoodleScriptFilename[] = "doodle.js";

const char kIntegrityFormat[] = "integrity=\"sha256-%s\"";

const struct Resource{
  const char* filename;
  int identifier;
  const char* mime_type;
} kResources[] = {
    {kMainHtmlFilename, kLocalResource, "text/html"},
    {"local-ntp.js", IDR_LOCAL_NTP_JS, "application/javascript"},
    {"voice.js", IDR_LOCAL_NTP_VOICE_JS, "application/javascript"},
    {"custom-backgrounds.js", IDR_LOCAL_NTP_CUSTOM_BACKGROUNDS_JS,
     "application/javascript"},
    {kConfigDataFilename, kLocalResource, "application/javascript"},
    {kThemeCSSFilename, kLocalResource, "text/css"},
    {"local-ntp.css", IDR_LOCAL_NTP_CSS, "text/css"},
    {"voice.css", IDR_LOCAL_NTP_VOICE_CSS, "text/css"},
    {"custom-backgrounds.css", IDR_LOCAL_NTP_CUSTOM_BACKGROUNDS_CSS,
     "text/css"},
    {"images/close_3_mask.png", IDR_CLOSE_3_MASK, "image/png"},
    {"images/ntp_default_favicon.png", IDR_NTP_DEFAULT_FAVICON, "image/png"},
    {kNtpBackgroundCollectionScriptFilename, kLocalResource, "text/javascript"},
    {kNtpBackgroundImageScriptFilename, kLocalResource, "text/javascript"},
    {kOneGoogleBarScriptFilename, kLocalResource, "text/javascript"},
    {kDoodleScriptFilename, kLocalResource, "text/javascript"},
};

// Strips any query parameters from the specified path.
std::string StripParameters(const std::string& path) {
  return path.substr(0, path.find("?"));
}

// Adds a localized string keyed by resource id to the dictionary.
void AddString(base::DictionaryValue* dictionary,
               const std::string& key,
               int resource_id) {
  dictionary->SetString(key, l10n_util::GetStringUTF16(resource_id));
}

// Populates |translated_strings| dictionary for the local NTP. |is_google|
// indicates that this page is the Google local NTP.
std::unique_ptr<base::DictionaryValue> GetTranslatedStrings(bool is_google) {
  auto translated_strings = std::make_unique<base::DictionaryValue>();

  AddString(translated_strings.get(), "thumbnailRemovedNotification",
            IDS_NEW_TAB_THUMBNAIL_REMOVED_NOTIFICATION);
  AddString(translated_strings.get(), "removeThumbnailTooltip",
            IDS_NEW_TAB_REMOVE_THUMBNAIL_TOOLTIP);
  AddString(translated_strings.get(), "undoThumbnailRemove",
            IDS_NEW_TAB_UNDO_THUMBNAIL_REMOVE);
  AddString(translated_strings.get(), "restoreThumbnailsShort",
            IDS_NEW_TAB_RESTORE_THUMBNAILS_SHORT_LINK);
  AddString(translated_strings.get(), "attributionIntro",
            IDS_NEW_TAB_ATTRIBUTION_INTRO);
  AddString(translated_strings.get(), "title", IDS_NEW_TAB_TITLE);
  AddString(translated_strings.get(), "mostVisitedTitle",
            IDS_NEW_TAB_MOST_VISITED);

  if (is_google) {
    AddString(translated_strings.get(), "searchboxPlaceholder",
              base::FeatureList::IsEnabled(features::kNtpUIMd)
                  ? IDS_GOOGLE_SEARCH_BOX_EMPTY_HINT_MD
                  : IDS_GOOGLE_SEARCH_BOX_EMPTY_HINT);
    AddString(translated_strings.get(), "customizeBackground",
              IDS_NTP_CUSTOM_BG_CUSTOMIZE_BACKGROUND);
    AddString(translated_strings.get(), "connectGooglePhotos",
              IDS_NTP_CUSTOM_BG_GOOGLE_PHOTOS);
    AddString(translated_strings.get(), "defaultWallpapers",
              IDS_NTP_CUSTOM_BG_CHROME_WALLPAPERS);
    AddString(translated_strings.get(), "uploadImage",
              IDS_NTP_CUSTOM_BG_UPLOAD_AN_IMAGE);
    AddString(translated_strings.get(), "restoreDefaultBackground",
              IDS_NTP_CUSTOM_BG_RESTORE_DEFAULT);
    AddString(translated_strings.get(), "selectChromeWallpaper",
              IDS_NTP_CUSTOM_BG_SELECT_A_COLLECTION);
    AddString(translated_strings.get(), "dailyRefresh",
              IDS_NTP_CUSTOM_BG_DAILY_REFRESH);
    AddString(translated_strings.get(), "selectionDone",
              IDS_NTP_CUSTOM_LINKS_DONE);

    // Voice Search
    AddString(translated_strings.get(), "audioError",
              IDS_NEW_TAB_VOICE_AUDIO_ERROR);
    AddString(translated_strings.get(), "details", IDS_NEW_TAB_VOICE_DETAILS);
    AddString(translated_strings.get(), "clickToViewDoodle",
              IDS_CLICK_TO_VIEW_DOODLE);
    AddString(translated_strings.get(), "fakeboxMicrophoneTooltip",
              IDS_TOOLTIP_MIC_SEARCH);
    AddString(translated_strings.get(), "languageError",
              IDS_NEW_TAB_VOICE_LANGUAGE_ERROR);
    AddString(translated_strings.get(), "learnMore", IDS_LEARN_MORE);
    AddString(translated_strings.get(), "listening",
              IDS_NEW_TAB_VOICE_LISTENING);
    AddString(translated_strings.get(), "networkError",
              IDS_NEW_TAB_VOICE_NETWORK_ERROR);
    AddString(translated_strings.get(), "noTranslation",
              IDS_NEW_TAB_VOICE_NO_TRANSLATION);
    AddString(translated_strings.get(), "noVoice", IDS_NEW_TAB_VOICE_NO_VOICE);
    AddString(translated_strings.get(), "permissionError",
              IDS_NEW_TAB_VOICE_PERMISSION_ERROR);
    AddString(translated_strings.get(), "ready", IDS_NEW_TAB_VOICE_READY);
    AddString(translated_strings.get(), "tryAgain",
              IDS_NEW_TAB_VOICE_TRY_AGAIN);
    AddString(translated_strings.get(), "waiting", IDS_NEW_TAB_VOICE_WAITING);
    AddString(translated_strings.get(), "otherError",
              IDS_NEW_TAB_VOICE_OTHER_ERROR);
  }

  return translated_strings;
}

// Returns a JS dictionary of configuration data for the local NTP.
std::string GetConfigData(bool is_google, const GURL& google_base_url) {
  base::DictionaryValue config_data;
  config_data.Set("translatedStrings", GetTranslatedStrings(is_google));
  config_data.SetBoolean("isGooglePage", is_google);
  config_data.SetString("googleBaseUrl", google_base_url.spec());
  config_data.SetBoolean(
      "isAccessibleBrowser",
      content::BrowserAccessibilityState::GetInstance()->IsAccessibleBrowser());

  bool is_voice_search_enabled =
      base::FeatureList::IsEnabled(features::kVoiceSearchOnLocalNtp);
  config_data.SetBoolean("isVoiceSearchEnabled", is_voice_search_enabled);

  bool is_md_ui_enabled = base::FeatureList::IsEnabled(features::kNtpUIMd);
  config_data.SetBoolean("isMDUIEnabled", is_md_ui_enabled);

  bool is_md_icons_enabled = base::FeatureList::IsEnabled(features::kNtpIcons);
  config_data.SetBoolean("isMDIconsEnabled", is_md_icons_enabled);

  if (is_google) {
    bool is_custom_backgrounds_enabled =
        base::FeatureList::IsEnabled(features::kNtpBackgrounds) ||
        base::FeatureList::IsEnabled(features::kNtpUIMd);
    config_data.SetBoolean("isCustomBackgroundsEnabled",
                           is_custom_backgrounds_enabled);
  }

  // Serialize the dictionary.
  std::string js_text;
  JSONStringValueSerializer serializer(&js_text);
  serializer.Serialize(config_data);

  std::string config_data_js;
  config_data_js.append("var configData = ");
  config_data_js.append(js_text);
  config_data_js.append(";");
  return config_data_js;
}

std::string GetThemeCSS(Profile* profile) {
  SkColor background_color =
      ThemeService::GetThemeProviderForProfile(profile)
          .GetColor(ThemeProperties::COLOR_NTP_BACKGROUND);

  return base::StringPrintf("html { background-color: #%02X%02X%02X; }",
                            SkColorGetR(background_color),
                            SkColorGetG(background_color),
                            SkColorGetB(background_color));
}

std::string GetLocalNtpPath() {
  return std::string(chrome::kChromeSearchScheme) + "://" +
         std::string(chrome::kChromeSearchLocalNtpHost) + "/";
}

base::Value ConvertCollectionInfoToDict(
    const std::vector<CollectionInfo>& collection_info) {
  base::Value collections(base::Value::Type::LIST);
  collections.GetList().reserve(collection_info.size());
  for (const CollectionInfo& collection : collection_info) {
    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetKey("collectionId", base::Value(collection.collection_id));
    dict.SetKey("collectionName", base::Value(collection.collection_name));
    dict.SetKey("previewImageUrl",
                base::Value(collection.preview_image_url.spec()));
    collections.GetList().push_back(std::move(dict));
  }
  return collections;
}

base::Value ConvertCollectionImageToDict(
    const std::vector<CollectionImage>& collection_image) {
  base::Value images(base::Value::Type::LIST);
  images.GetList().reserve(collection_image.size());
  for (const CollectionImage& image : collection_image) {
    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetKey("thumbnailImageUrl",
                base::Value(image.thumbnail_image_url.spec()));
    dict.SetKey("imageUrl", base::Value(image.image_url.spec()));
    dict.SetKey("collectionId", base::Value(image.collection_id));
    base::Value attributions(base::Value::Type::LIST);
    for (const auto& attribution : image.attribution) {
      attributions.GetList().push_back(base::Value(attribution));
    }
    dict.SetKey("attributions", std::move(attributions));
    images.GetList().push_back(std::move(dict));
  }
  return images;
}

std::unique_ptr<base::DictionaryValue> ConvertOGBDataToDict(
    const OneGoogleBarData& og) {
  auto result = std::make_unique<base::DictionaryValue>();
  result->SetString("barHtml", og.bar_html);
  result->SetString("inHeadScript", og.in_head_script);
  result->SetString("inHeadStyle", og.in_head_style);
  result->SetString("afterBarScript", og.after_bar_script);
  result->SetString("endOfBodyHtml", og.end_of_body_html);
  result->SetString("endOfBodyScript", og.end_of_body_script);
  return result;
}

std::string ConvertLogoImageToBase64(const EncodedLogo& logo) {
  std::string base64;
  base::Base64Encode(logo.encoded_image->data(), &base64);
  return base::StringPrintf("data:%s;base64,%s",
                            logo.metadata.mime_type.c_str(), base64.c_str());
}

std::string LogoTypeToString(search_provider_logos::LogoType type) {
  switch (type) {
    case search_provider_logos::LogoType::SIMPLE:
      return "SIMPLE";
    case search_provider_logos::LogoType::ANIMATED:
      return "ANIMATED";
    case search_provider_logos::LogoType::INTERACTIVE:
      return "INTERACTIVE";
  }
  NOTREACHED();
  return std::string();
}

std::unique_ptr<base::DictionaryValue> ConvertLogoMetadataToDict(
    const LogoMetadata& meta) {
  auto result = std::make_unique<base::DictionaryValue>();
  result->SetString("type", LogoTypeToString(meta.type));
  result->SetString("onClickUrl", meta.on_click_url.spec());
  result->SetString("altText", meta.alt_text);
  result->SetString("mimeType", meta.mime_type);
  result->SetString("animatedUrl", meta.animated_url.spec());
  result->SetInteger("iframeWidthPx", meta.iframe_width_px);
  result->SetInteger("iframeHeightPx", meta.iframe_height_px);
  result->SetString("logUrl", meta.log_url.spec());
  result->SetString("ctaLogUrl", meta.cta_log_url.spec());

  GURL full_page_url = meta.full_page_url;
  if (base::GetFieldTrialParamByFeatureAsBool(
          features::kDoodlesOnLocalNtp,
          "local_ntp_interactive_doodles_prevent_redirects",
          /*default_value=*/true) &&
      meta.type == search_provider_logos::LogoType::INTERACTIVE &&
      full_page_url.is_valid()) {
    // Prevent the server from redirecting to ccTLDs. This is a temporary
    // workaround, until the server doesn't redirect these requests by default.
    full_page_url = net::AppendQueryParameter(full_page_url, "gws_rd", "cr");
  }
  result->SetString("fullPageUrl", full_page_url.spec());

  // If support for interactive Doodles is disabled, treat them as simple
  // Doodles instead and use the full page URL as the target URL.
  if (meta.type == search_provider_logos::LogoType::INTERACTIVE &&
      !base::GetFieldTrialParamByFeatureAsBool(features::kDoodlesOnLocalNtp,
                                               "local_ntp_interactive_doodles",
                                               /*default_value=*/true)) {
    result->SetString(
        "type", LogoTypeToString(search_provider_logos::LogoType::SIMPLE));
    result->SetString("onClickUrl", meta.full_page_url.spec());
  }

  return result;
}

// Note: Code that runs on the IO thread is implemented as non-member functions,
// to avoid accidentally accessing member data that's owned by the UI thread.

bool ShouldServiceRequestIOThread(const GURL& url,
                                  content::ResourceContext* resource_context,
                                  int render_process_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  DCHECK(url.host_piece() == chrome::kChromeSearchLocalNtpHost);
  if (!InstantIOContext::ShouldServiceRequest(url, resource_context,
                                              render_process_id)) {
    return false;
  }

  if (url.SchemeIs(chrome::kChromeSearchScheme)) {
    std::string filename;
    webui::ParsePathAndScale(url, &filename, nullptr);
    for (size_t i = 0; i < arraysize(kResources); ++i) {
      if (filename == kResources[i].filename)
        return true;
    }
  }
  return false;
}

std::string GetContentSecurityPolicyScriptSrcIOThread() {
#if !defined(GOOGLE_CHROME_BUILD)
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kLocalNtpReload)) {
    // While live-editing the local NTP files, turn off CSP.
    return "script-src * 'unsafe-inline';";
  }
#endif  // !defined(GOOGLE_CHROME_BUILD)

  return base::StringPrintf(
      "script-src 'strict-dynamic' 'sha256-%s' 'sha256-%s' 'sha256-%s';",
      LOCAL_NTP_JS_INTEGRITY, VOICE_JS_INTEGRITY,
      CUSTOM_BACKGROUNDS_JS_INTEGRITY);
}

std::string GetContentSecurityPolicyChildSrcIOThread() {
  // Allow embedding of the most visited iframe, as well as the account
  // switcher and the notifications dropdown from the One Google Bar, and/or
  // the iframe for interactive Doodles.
  return base::StringPrintf("child-src %s https://*.google.com/;",
                            chrome::kChromeSearchMostVisitedUrl);
}

}  // namespace

class LocalNtpSource::GoogleSearchProviderTracker
    : public TemplateURLServiceObserver {
 public:
  explicit GoogleSearchProviderTracker(TemplateURLService* service)
      : service_(service), is_google_(false) {
    DCHECK(service_);
    service_->AddObserver(this);
    is_google_ = search::DefaultSearchProviderIsGoogle(service_);
    google_base_url_ = GURL(service_->search_terms_data().GoogleBaseURLValue());
  }

  ~GoogleSearchProviderTracker() override {
    if (service_)
      service_->RemoveObserver(this);
  }

  bool DefaultSearchProviderIsGoogle() const { return is_google_; }

  const GURL& GetGoogleBaseUrl() const { return google_base_url_; }

 private:
  void OnTemplateURLServiceChanged() override {
    is_google_ = search::DefaultSearchProviderIsGoogle(service_);
    google_base_url_ = GURL(service_->search_terms_data().GoogleBaseURLValue());
  }

  void OnTemplateURLServiceShuttingDown() override {
    service_->RemoveObserver(this);
    service_ = nullptr;
  }

  TemplateURLService* service_;

  bool is_google_;
  GURL google_base_url_;
};

class LocalNtpSource::DesktopLogoObserver {
 public:
  DesktopLogoObserver() : weak_ptr_factory_(this) {}

  // Get the cached logo.
  void GetCachedLogo(LogoService* service,
                     const content::URLDataSource::GotDataCallback& callback) {
    StartGetLogo(service, callback, /*from_cache=*/true);
  }

  // Get the fresh logo corresponding to a previous request for a cached logo.
  // If that previous request is still ongoing, then schedule the callback to be
  // called when the fresh logo comes in. If it's not, then start a new request
  // and schedule the cached logo to be handed back.
  //
  // Strictly speaking, it's not a "fresh" logo anymore, but it should be the
  // same logo that would have been fresh relative to the corresponding cached
  // request, or perhaps one newer.
  void GetFreshLogo(LogoService* service,
                    int requested_version,
                    const content::URLDataSource::GotDataCallback& callback) {
    bool from_cache = (requested_version <= version_finished_);
    StartGetLogo(service, callback, from_cache);
  }

 private:
  void OnLogoAvailable(const content::URLDataSource::GotDataCallback& callback,
                       LogoCallbackReason type,
                       const base::Optional<EncodedLogo>& logo) {
    scoped_refptr<base::RefCountedString> response;
    auto ddl = std::make_unique<base::DictionaryValue>();
    ddl->SetInteger("v", version_started_);
    if (type == LogoCallbackReason::DETERMINED) {
      ddl->SetBoolean("usable", true);
      if (logo.has_value()) {
        ddl->SetString("image", ConvertLogoImageToBase64(logo.value()));
        ddl->Set("metadata", ConvertLogoMetadataToDict(logo->metadata));
      } else {
        ddl->SetKey("image", base::Value());
        ddl->SetKey("metadata", base::Value());
      }
    } else {
      ddl->SetBoolean("usable", false);
    }

    std::string js;
    base::JSONWriter::Write(*ddl, &js);
    js = "var ddl = " + js + ";";
    response = base::RefCountedString::TakeString(&js);
    callback.Run(response);
  }

  void OnCachedLogoAvailable(
      const content::URLDataSource::GotDataCallback& callback,
      LogoCallbackReason type,
      const base::Optional<EncodedLogo>& logo) {
    OnLogoAvailable(callback, type, logo);
  }

  void OnFreshLogoAvailable(
      const content::URLDataSource::GotDataCallback& callback,
      LogoCallbackReason type,
      const base::Optional<EncodedLogo>& logo) {
    OnLogoAvailable(callback, type, logo);
    OnRequestCompleted(type, logo);
  }

  void OnRequestCompleted(LogoCallbackReason type,
                          const base::Optional<EncodedLogo>& logo) {
    version_finished_ = version_started_;
  }

  void StartGetLogo(LogoService* service,
                    const content::URLDataSource::GotDataCallback& callback,
                    bool from_cache) {
    EncodedLogoCallback cached, fresh;
    LogoCallbacks callbacks;
    if (from_cache) {
      callbacks.on_cached_encoded_logo_available =
          base::BindOnce(&DesktopLogoObserver::OnCachedLogoAvailable,
                         weak_ptr_factory_.GetWeakPtr(), callback);
      callbacks.on_fresh_encoded_logo_available =
          base::BindOnce(&DesktopLogoObserver::OnRequestCompleted,
                         weak_ptr_factory_.GetWeakPtr());
    } else {
      callbacks.on_fresh_encoded_logo_available =
          base::BindOnce(&DesktopLogoObserver::OnFreshLogoAvailable,
                         weak_ptr_factory_.GetWeakPtr(), callback);
    }
    if (!observing()) {
      ++version_started_;
    }
    service->GetLogo(std::move(callbacks));
  }

  bool observing() const {
    DCHECK_LE(version_finished_, version_started_);
    DCHECK_LE(version_started_, version_finished_ + 1);
    return version_started_ != version_finished_;
  }

  int version_started_ = 0;
  int version_finished_ = 0;

  base::WeakPtrFactory<DesktopLogoObserver> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DesktopLogoObserver);
};

LocalNtpSource::LocalNtpSource(Profile* profile)
    : profile_(profile),
      ntp_background_service_(
          NtpBackgroundServiceFactory::GetForProfile(profile_)),
      ntp_background_service_observer_(this),
      one_google_bar_service_(
          OneGoogleBarServiceFactory::GetForProfile(profile_)),
      one_google_bar_service_observer_(this),
      logo_service_(nullptr),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // |ntp_background_service_| is null in incognito, or when the feature is
  // disabled.
  if (ntp_background_service_)
    ntp_background_service_observer_.Add(ntp_background_service_);

  // |one_google_bar_service_| is null in incognito, or when the feature is
  // disabled.
  if (one_google_bar_service_)
    one_google_bar_service_observer_.Add(one_google_bar_service_);

  if (base::FeatureList::IsEnabled(features::kDoodlesOnLocalNtp)) {
    logo_service_ = LogoServiceFactory::GetForProfile(profile_);
    logo_observer_ = std::make_unique<DesktopLogoObserver>();
  }

  TemplateURLService* template_url_service =
      TemplateURLServiceFactory::GetForProfile(profile_);
  if (template_url_service) {
    google_tracker_ =
        std::make_unique<GoogleSearchProviderTracker>(template_url_service);
  }
}

LocalNtpSource::~LocalNtpSource() = default;

std::string LocalNtpSource::GetSource() const {
  return chrome::kChromeSearchLocalNtpHost;
}

void LocalNtpSource::StartDataRequest(
    const std::string& path,
    const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
    const content::URLDataSource::GotDataCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::string stripped_path = StripParameters(path);
  if (stripped_path == kConfigDataFilename) {
    std::string config_data_js =
        GetConfigData(google_tracker_->DefaultSearchProviderIsGoogle(),
                      google_tracker_->GetGoogleBaseUrl());
    callback.Run(base::RefCountedString::TakeString(&config_data_js));
    return;
  }
  if (stripped_path == kThemeCSSFilename) {
    std::string theme_css = GetThemeCSS(profile_);
    callback.Run(base::RefCountedString::TakeString(&theme_css));
    return;
  }

  if (stripped_path == kNtpBackgroundCollectionScriptFilename) {
    if (!ntp_background_service_) {
      callback.Run(nullptr);
      return;
    }

    ntp_background_collections_requests_.emplace_back(base::TimeTicks::Now(),
                                                      callback);
    ntp_background_service_->FetchCollectionInfo();
    return;
  }

  if (stripped_path == kNtpBackgroundImageScriptFilename) {
    if (!ntp_background_service_) {
      callback.Run(nullptr);
      return;
    }
    std::string collection_id_param;
    GURL path_url = GURL(chrome::kChromeSearchLocalNtpUrl).Resolve(path);
    if (net::GetValueForKeyInQuery(path_url, "collection_id",
                                   &collection_id_param)) {
      ntp_background_image_info_requests_.emplace_back(base::TimeTicks::Now(),
                                                       callback);
      ntp_background_service_->FetchCollectionImageInfo(collection_id_param);
    } else {
      callback.Run(nullptr);
    }
    return;
  }

  if (stripped_path == kOneGoogleBarScriptFilename) {
    if (!one_google_bar_service_) {
      callback.Run(nullptr);
      return;
    }

    one_google_bar_requests_.emplace_back(base::TimeTicks::Now(), callback);
    // TODO(treib): Figure out if there are cases where we can safely serve
    // cached data. crbug.com/742937
    one_google_bar_service_->Refresh();

    return;
  }

  if (stripped_path == kDoodleScriptFilename) {
    if (!logo_service_) {
      callback.Run(nullptr);
      return;
    }

    std::string version_string;
    int version = 0;
    GURL url = GURL(chrome::kChromeSearchLocalNtpUrl).Resolve(path);
    if (net::GetValueForKeyInQuery(url, "v", &version_string) &&
        base::StringToInt(version_string, &version)) {
      logo_observer_->GetFreshLogo(logo_service_, version, callback);
    } else {
      logo_observer_->GetCachedLogo(logo_service_, callback);
    }
    return;
  }

#if !defined(GOOGLE_CHROME_BUILD)
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kLocalNtpReload)) {
    if (stripped_path == "local-ntp.html" || stripped_path == "local-ntp.js" ||
        stripped_path == "local-ntp.css" || stripped_path == "voice.js" ||
        stripped_path == "voice.css") {
      base::ReplaceChars(stripped_path, "-", "_", &stripped_path);
      local_ntp::SendLocalFileResource(stripped_path, callback);
      return;
    }
  }
#endif  // !defined(GOOGLE_CHROME_BUILD)

  if (stripped_path == kMainHtmlFilename) {
    std::string html = ui::ResourceBundle::GetSharedInstance()
                           .GetRawDataResource(IDR_LOCAL_NTP_HTML)
                           .as_string();
    std::string local_ntp_integrity =
        base::StringPrintf(kIntegrityFormat, LOCAL_NTP_JS_INTEGRITY);
    base::ReplaceFirstSubstringAfterOffset(&html, 0, "{{LOCAL_NTP_INTEGRITY}}",
                                           local_ntp_integrity);

    std::string local_ntp_voice_integrity =
        base::StringPrintf(kIntegrityFormat, VOICE_JS_INTEGRITY);
    base::ReplaceFirstSubstringAfterOffset(
        &html, 0, "{{LOCAL_NTP_VOICE_INTEGRITY}}", local_ntp_voice_integrity);

    std::string local_ntp_custom_bg_integrity =
        base::StringPrintf(kIntegrityFormat, CUSTOM_BACKGROUNDS_JS_INTEGRITY);
    base::ReplaceFirstSubstringAfterOffset(&html, 0,
                                           "{{LOCAL_NTP_CUSTOM_BG_INTEGRITY}}",
                                           local_ntp_custom_bg_integrity);
    callback.Run(base::RefCountedString::TakeString(&html));
    return;
  }

  float scale = 1.0f;
  std::string filename;
  webui::ParsePathAndScale(
      GURL(GetLocalNtpPath() + stripped_path), &filename, &scale);
  ui::ScaleFactor scale_factor = ui::GetSupportedScaleFactor(scale);

  for (size_t i = 0; i < arraysize(kResources); ++i) {
    if (filename == kResources[i].filename) {
      scoped_refptr<base::RefCountedMemory> response(
          ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytesForScale(
              kResources[i].identifier, scale_factor));
      callback.Run(response.get());
      return;
    }
  }
  callback.Run(nullptr);
}

std::string LocalNtpSource::GetMimeType(
    const std::string& path) const {
  const std::string stripped_path = StripParameters(path);
  for (size_t i = 0; i < arraysize(kResources); ++i) {
    if (stripped_path == kResources[i].filename)
      return kResources[i].mime_type;
  }
  return std::string();
}

bool LocalNtpSource::AllowCaching() const {
  // Some resources served by LocalNtpSource, i.e. config.js, are dynamically
  // generated and could differ on each access. To avoid using old cached
  // content on reload, disallow caching here. Otherwise, it fails to reflect
  // newly revised user configurations in the page.
  return false;
}

bool LocalNtpSource::ShouldServiceRequest(
    const GURL& url,
    content::ResourceContext* resource_context,
    int render_process_id) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  return ShouldServiceRequestIOThread(url, resource_context, render_process_id);
}

std::string LocalNtpSource::GetContentSecurityPolicyScriptSrc() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  return GetContentSecurityPolicyScriptSrcIOThread();
}

std::string LocalNtpSource::GetContentSecurityPolicyChildSrc() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  return GetContentSecurityPolicyChildSrcIOThread();
}

void LocalNtpSource::OnCollectionInfoAvailable() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (ntp_background_collections_requests_.empty())
    return;

  scoped_refptr<base::RefCountedString> result;
  std::string js;
  base::JSONWriter::Write(
      ConvertCollectionInfoToDict(ntp_background_service_->collection_info()),
      &js);
  js = "var coll = " + js + ";";
  result = base::RefCountedString::TakeString(&js);

  base::TimeTicks now = base::TimeTicks::Now();
  for (const auto& request : ntp_background_collections_requests_) {
    request.callback.Run(result);
    base::TimeDelta delta = now - request.start_time;
    UMA_HISTOGRAM_MEDIUM_TIMES(
        "NewTabPage.BackgroundService.Collections.RequestLatency", delta);
    // Any response where no collections are returned is considered a failure.
    if (ntp_background_service_->collection_info().empty()) {
      UMA_HISTOGRAM_MEDIUM_TIMES(
          "NewTabPage.BackgroundService.Collections.RequestLatency.Failure",
          delta);
    } else {
      UMA_HISTOGRAM_MEDIUM_TIMES(
          "NewTabPage.BackgroundService.Collections.RequestLatency.Success",
          delta);
    }
  }
  ntp_background_collections_requests_.clear();
}

void LocalNtpSource::OnCollectionImagesAvailable() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (ntp_background_image_info_requests_.empty())
    return;

  scoped_refptr<base::RefCountedString> result;
  std::string js;
  base::JSONWriter::Write(ConvertCollectionImageToDict(
                              ntp_background_service_->collection_images()),
                          &js);
  js = "var coll_img = " + js + ";";
  result = base::RefCountedString::TakeString(&js);

  base::TimeTicks now = base::TimeTicks::Now();
  for (const auto& request : ntp_background_image_info_requests_) {
    request.callback.Run(result);
    base::TimeDelta delta = now - request.start_time;
    UMA_HISTOGRAM_MEDIUM_TIMES(
        "NewTabPage.BackgroundService.Images.RequestLatency", delta);
    // Any response where no images are returned is considered a failure.
    if (ntp_background_service_->collection_images().empty()) {
      UMA_HISTOGRAM_MEDIUM_TIMES(
          "NewTabPage.BackgroundService.Images.RequestLatency.Failure", delta);
    } else {
      UMA_HISTOGRAM_MEDIUM_TIMES(
          "NewTabPage.BackgroundService.Images.RequestLatency.Success", delta);
    }
  }
  ntp_background_image_info_requests_.clear();
}

void LocalNtpSource::OnNtpBackgroundServiceShuttingDown() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  ntp_background_service_observer_.RemoveAll();
  ntp_background_service_ = nullptr;
}

void LocalNtpSource::OnOneGoogleBarDataUpdated() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  ServeOneGoogleBar(one_google_bar_service_->one_google_bar_data());
}

void LocalNtpSource::OnOneGoogleBarServiceShuttingDown() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  one_google_bar_service_observer_.RemoveAll();
  one_google_bar_service_ = nullptr;
}

void LocalNtpSource::ServeOneGoogleBar(
    const base::Optional<OneGoogleBarData>& data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (one_google_bar_requests_.empty())
    return;

  scoped_refptr<base::RefCountedString> result;
  if (data.has_value()) {
    std::string js;
    base::JSONWriter::Write(*ConvertOGBDataToDict(*data), &js);
    js = "var og = " + js + ";";
    result = base::RefCountedString::TakeString(&js);
  }

  base::TimeTicks now = base::TimeTicks::Now();
  for (const auto& request : one_google_bar_requests_) {
    request.callback.Run(result);
    base::TimeDelta delta = now - request.start_time;
    UMA_HISTOGRAM_MEDIUM_TIMES("NewTabPage.OneGoogleBar.RequestLatency", delta);
    if (result) {
      UMA_HISTOGRAM_MEDIUM_TIMES(
          "NewTabPage.OneGoogleBar.RequestLatency.Success", delta);
    } else {
      UMA_HISTOGRAM_MEDIUM_TIMES(
          "NewTabPage.OneGoogleBar.RequestLatency.Failure", delta);
    }
  }
  one_google_bar_requests_.clear();
}

LocalNtpSource::NtpBackgroundRequest::NtpBackgroundRequest(
    base::TimeTicks start_time,
    const content::URLDataSource::GotDataCallback& callback)
    : start_time(start_time), callback(callback) {}

LocalNtpSource::NtpBackgroundRequest::NtpBackgroundRequest(
    const NtpBackgroundRequest&) = default;

LocalNtpSource::NtpBackgroundRequest::~NtpBackgroundRequest() = default;

LocalNtpSource::OneGoogleBarRequest::OneGoogleBarRequest(
    base::TimeTicks start_time,
    const content::URLDataSource::GotDataCallback& callback)
    : start_time(start_time), callback(callback) {}

LocalNtpSource::OneGoogleBarRequest::OneGoogleBarRequest(
    const OneGoogleBarRequest&) = default;

LocalNtpSource::OneGoogleBarRequest::~OneGoogleBarRequest() = default;
