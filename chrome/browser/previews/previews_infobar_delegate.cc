// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/previews/previews_infobar_delegate.h"

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/android/android_theme_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/net/spdyproxy/data_reduction_proxy_chrome_settings.h"
#include "chrome/browser/net/spdyproxy/data_reduction_proxy_chrome_settings_factory.h"
#include "chrome/browser/page_load_metrics/metrics_web_contents_observer.h"
#include "chrome/browser/previews/previews_infobar_tab_helper.h"
#include "chrome/grit/generated_resources.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "components/infobars/core/infobar.h"
#include "components/network_time/network_time_tracker.h"
#include "components/previews/content/previews_ui_service.h"
#include "components/previews/core/previews_features.h"
#include "components/previews/core/previews_logger.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(OS_ANDROID)
#include "chrome/browser/ui/android/infobars/previews_infobar.h"
#endif

namespace {

const void* const kOptOutEventKey = 0;

const char kMinStalenessParamName[] = "min_staleness_in_minutes";
const char kMaxStalenessParamName[] = "max_staleness_in_minutes";
const int kMinStalenessParamDefaultValue = 5;
const int kMaxStalenessParamDefaultValue = 1440;
static const char kPreviewInfobarEventType[] = "InfoBar";

void RecordPreviewsInfoBarAction(
    previews::PreviewsType previews_type,
    PreviewsInfoBarDelegate::PreviewsInfoBarAction action) {
  int32_t max_limit =
      static_cast<int32_t>(PreviewsInfoBarDelegate::INFOBAR_INDEX_BOUNDARY);
  base::LinearHistogram::FactoryGet(
      base::StringPrintf("Previews.InfoBarAction.%s",
                         GetStringNameForType(previews_type).c_str()),
      1, max_limit, max_limit + 1,
      base::HistogramBase::kUmaTargetedHistogramFlag)
      ->Add(static_cast<int32_t>(action));
}

void RecordStaleness(PreviewsInfoBarDelegate::PreviewsInfoBarTimestamp value) {
  UMA_HISTOGRAM_ENUMERATION("Previews.InfoBarTimestamp", value,
                            PreviewsInfoBarDelegate::TIMESTAMP_INDEX_BOUNDARY);
}

// Reloads the content of the page without previews.
void ReloadWithoutPreviews(previews::PreviewsType previews_type,
                           content::WebContents* web_contents) {
  switch (previews_type) {
    case previews::PreviewsType::LITE_PAGE:
    case previews::PreviewsType::OFFLINE:
    case previews::PreviewsType::AMP_REDIRECTION:
    case previews::PreviewsType::NOSCRIPT:
      // Prevent previews and lite page modes from showing after reload.
      web_contents->GetController().Reload(
          content::ReloadType::DISABLE_PREVIEWS, true);
      break;
    case previews::PreviewsType::LOFI:
      web_contents->ReloadLoFiImages();
      break;
    case previews::PreviewsType::NONE:
    case previews::PreviewsType::UNSPECIFIED:
    case previews::PreviewsType::LAST:
      NOTREACHED();
      break;
  }
}

void InformPLMOfOptOut(content::WebContents* web_contents) {
  page_load_metrics::MetricsWebContentsObserver* metrics_web_contents_observer =
      page_load_metrics::MetricsWebContentsObserver::FromWebContents(
          web_contents);
  if (!metrics_web_contents_observer)
    return;

  metrics_web_contents_observer->BroadcastEventToObservers(
      PreviewsInfoBarDelegate::OptOutEventKey());
}

}  // namespace

PreviewsInfoBarDelegate::~PreviewsInfoBarDelegate() {
  if (!on_dismiss_callback_.is_null())
    std::move(on_dismiss_callback_).Run(false);

  RecordPreviewsInfoBarAction(previews_type_, infobar_dismissed_action_);
}

// static
void PreviewsInfoBarDelegate::Create(
    content::WebContents* web_contents,
    previews::PreviewsType previews_type,
    base::Time previews_freshness,
    bool is_data_saver_user,
    bool is_reload,
    OnDismissPreviewsInfobarCallback on_dismiss_callback,
    previews::PreviewsUIService* previews_ui_service) {
  PreviewsInfoBarTabHelper* infobar_tab_helper =
      PreviewsInfoBarTabHelper::FromWebContents(web_contents);
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(web_contents);

  // The WebContents may not have TabHelpers set. If TabHelpers are not set,
  // don't show Previews infobars.
  if (!infobar_tab_helper || !infobar_service)
    return;
  if (infobar_tab_helper->displayed_preview_infobar())
    return;

  std::unique_ptr<PreviewsInfoBarDelegate> delegate(new PreviewsInfoBarDelegate(
      infobar_tab_helper, previews_type, previews_freshness, is_data_saver_user,
      is_reload, std::move(on_dismiss_callback)));

#if defined(OS_ANDROID)
  std::unique_ptr<infobars::InfoBar> infobar_ptr(
      PreviewsInfoBar::CreateInfoBar(infobar_service, std::move(delegate)));
#else
  std::unique_ptr<infobars::InfoBar> infobar_ptr(
      infobar_service->CreateConfirmInfoBar(std::move(delegate)));
#endif

  infobar_service->AddInfoBar(std::move(infobar_ptr));
  uint64_t page_id = (infobar_tab_helper->previews_user_data())
                         ? infobar_tab_helper->previews_user_data()->page_id()
                         : 0;

  if (previews_ui_service) {
    // Not in incognito mode or guest mode.
    previews_ui_service->previews_logger()->LogMessage(
        kPreviewInfobarEventType,
        previews::GetDescriptionForInfoBarDescription(previews_type),
        web_contents->GetController()
            .GetLastCommittedEntry()
            ->GetRedirectChain()[0] /* GURL */,
        base::Time::Now(), page_id);
  }

  RecordPreviewsInfoBarAction(previews_type, INFOBAR_SHOWN);
  infobar_tab_helper->set_displayed_preview_infobar(true);
}

PreviewsInfoBarDelegate::PreviewsInfoBarDelegate(
    PreviewsInfoBarTabHelper* infobar_tab_helper,
    previews::PreviewsType previews_type,
    base::Time previews_freshness,
    bool is_data_saver_user,
    bool is_reload,
    OnDismissPreviewsInfobarCallback on_dismiss_callback)
    : ConfirmInfoBarDelegate(),
      infobar_tab_helper_(infobar_tab_helper),
      previews_type_(previews_type),
      previews_freshness_(previews_freshness),
      is_reload_(is_reload),
      infobar_dismissed_action_(INFOBAR_DISMISSED_BY_TAB_CLOSURE),
      message_text_(l10n_util::GetStringUTF16(
          is_data_saver_user ? IDS_PREVIEWS_INFOBAR_SAVED_DATA_TITLE
                             : IDS_PREVIEWS_INFOBAR_FASTER_PAGE_TITLE)),
      on_dismiss_callback_(std::move(on_dismiss_callback)) {
  DCHECK(previews_type_ != previews::PreviewsType::NONE &&
         previews_type_ != previews::PreviewsType::UNSPECIFIED);
}

infobars::InfoBarDelegate::InfoBarIdentifier
PreviewsInfoBarDelegate::GetIdentifier() const {
  return DATA_REDUCTION_PROXY_PREVIEW_INFOBAR_DELEGATE;
}

int PreviewsInfoBarDelegate::GetIconId() const {
#if defined(OS_ANDROID)
  return IDR_ANDROID_INFOBAR_PREVIEWS;
#else
  return kNoIconID;
#endif
}

bool PreviewsInfoBarDelegate::ShouldExpire(
    const NavigationDetails& details) const {
  infobar_dismissed_action_ = details.is_reload
                                  ? INFOBAR_DISMISSED_BY_RELOAD
                                  : INFOBAR_DISMISSED_BY_NAVIGATION;
  return InfoBarDelegate::ShouldExpire(details);
}

void PreviewsInfoBarDelegate::InfoBarDismissed() {
  infobar_dismissed_action_ = INFOBAR_DISMISSED_BY_USER;
}

base::string16 PreviewsInfoBarDelegate::GetMessageText() const {
// Android has a custom infobar that calls GetTimestampText() and adds the
// timestamp in a separate description view. Other OS's can enable previews
// for debugging purposes and don't have a custom infobar with a description
// view, so the timestamp should be appended to the message.
#if defined(OS_ANDROID)
  return message_text_;
#else
  base::string16 timestamp = GetTimestampText();
  if (timestamp.empty())
    return message_text_;
  // This string concatenation wouldn't fly for l10n, but this is only a hack
  // for Chromium devs and not expected to ever appear for users.
  return message_text_ + base::ASCIIToUTF16(" - ") + timestamp;
#endif
}

int PreviewsInfoBarDelegate::GetButtons() const {
  return BUTTON_NONE;
}

base::string16 PreviewsInfoBarDelegate::GetLinkText() const {
  return l10n_util::GetStringUTF16(IDS_PREVIEWS_INFOBAR_LINK);
}

bool PreviewsInfoBarDelegate::LinkClicked(WindowOpenDisposition disposition) {
  infobar_dismissed_action_ = INFOBAR_LOAD_ORIGINAL_CLICKED;
  if (!on_dismiss_callback_.is_null())
    std::move(on_dismiss_callback_).Run(true);

  content::WebContents* web_contents =
      InfoBarService::WebContentsFromInfoBar(infobar());

  InformPLMOfOptOut(web_contents);

  ReloadWithoutPreviews(previews_type_, web_contents);

  return true;
}

base::string16 PreviewsInfoBarDelegate::GetTimestampText() const {
  if (previews_freshness_.is_null())
    return base::string16();
  if (!base::FeatureList::IsEnabled(
          previews::features::kStalePreviewsTimestamp)) {
    return base::string16();
  }

  int min_staleness_in_minutes = base::GetFieldTrialParamByFeatureAsInt(
      previews::features::kStalePreviewsTimestamp, kMinStalenessParamName,
      kMinStalenessParamDefaultValue);
  int max_staleness_in_minutes = base::GetFieldTrialParamByFeatureAsInt(
      previews::features::kStalePreviewsTimestamp, kMaxStalenessParamName,
      kMaxStalenessParamDefaultValue);

  if (min_staleness_in_minutes <= 0 || max_staleness_in_minutes <= 0) {
    NOTREACHED();
    return base::string16();
  }

  base::Time network_time;
  if (g_browser_process->network_time_tracker()->GetNetworkTime(&network_time,
                                                                nullptr) !=
      network_time::NetworkTimeTracker::NETWORK_TIME_AVAILABLE) {
    // When network time has not been initialized yet, simply rely on the
    // machine's current time.
    network_time = base::Time::Now();
  }

  if (network_time < previews_freshness_) {
    RecordStaleness(TIMESTAMP_NOT_SHOWN_STALENESS_NEGATIVE);
    return base::string16();
  }

  int staleness_in_minutes = (network_time - previews_freshness_).InMinutes();
  if (staleness_in_minutes < min_staleness_in_minutes) {
    if (is_reload_) {
      RecordStaleness(TIMESTAMP_UPDATED_NOW_SHOWN);
      if (infobar_tab_helper_)
        infobar_tab_helper_->set_displayed_preview_timestamp(true);
      return l10n_util::GetStringUTF16(
          IDS_PREVIEWS_INFOBAR_TIMESTAMP_UPDATED_NOW);
    }
    RecordStaleness(TIMESTAMP_NOT_SHOWN_PREVIEW_NOT_STALE);
    return base::string16();
  }
  if (staleness_in_minutes > max_staleness_in_minutes) {
    RecordStaleness(TIMESTAMP_NOT_SHOWN_STALENESS_GREATER_THAN_MAX);
    return base::string16();
  }

  RecordStaleness(TIMESTAMP_SHOWN);
  if (infobar_tab_helper_)
    infobar_tab_helper_->set_displayed_preview_timestamp(true);

  if (staleness_in_minutes < 60) {
    return l10n_util::GetStringFUTF16(
        IDS_PREVIEWS_INFOBAR_TIMESTAMP_MINUTES,
        base::IntToString16(staleness_in_minutes));
  } else if (staleness_in_minutes < 120) {
    return l10n_util::GetStringUTF16(IDS_PREVIEWS_INFOBAR_TIMESTAMP_ONE_HOUR);
  } else {
    return l10n_util::GetStringFUTF16(
        IDS_PREVIEWS_INFOBAR_TIMESTAMP_HOURS,
        base::IntToString16(staleness_in_minutes / 60));
  }
}

// static
const void* PreviewsInfoBarDelegate::OptOutEventKey() {
  return &kOptOutEventKey;
}
