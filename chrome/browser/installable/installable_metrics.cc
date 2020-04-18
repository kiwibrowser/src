// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/installable/installable_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "build/build_config.h"
#include "content/public/browser/web_contents.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/tab_android.h"
#endif

namespace {

void WriteMenuOpenHistogram(InstallabilityCheckStatus status, int count) {
  for (int i = 0; i < count; ++i) {
    UMA_HISTOGRAM_ENUMERATION("Webapp.InstallabilityCheckStatus.MenuOpen",
                              status, InstallabilityCheckStatus::COUNT);
  }
}

void WriteMenuItemAddToHomescreenHistogram(InstallabilityCheckStatus status,
                                           int count) {
  for (int i = 0; i < count; ++i) {
    UMA_HISTOGRAM_ENUMERATION(
        "Webapp.InstallabilityCheckStatus.MenuItemAddToHomescreen", status,
        InstallabilityCheckStatus::COUNT);
  }
}

void WriteAddToHomescreenHistogram(AddToHomescreenTimeoutStatus status,
                                   int count) {
  for (int i = 0; i < count; ++i) {
    UMA_HISTOGRAM_ENUMERATION(
        "Webapp.InstallabilityCheckStatus.AddToHomescreenTimeout", status,
        AddToHomescreenTimeoutStatus::COUNT);
  }
}

// Buffers metrics calls until we have resolved whether a site is a PWA.
class AccumulatingRecorder : public InstallableMetrics::Recorder {
 public:
  AccumulatingRecorder()
      : InstallableMetrics::Recorder(),
        menu_open_count_(0),
        menu_item_add_to_homescreen_count_(0),
        add_to_homescreen_manifest_timeout_count_(0),
        add_to_homescreen_installability_timeout_count_(0),
        started_(false) {}

  ~AccumulatingRecorder() override {
    DCHECK_EQ(0, menu_open_count_);
    DCHECK_EQ(0, menu_item_add_to_homescreen_count_);
    DCHECK_EQ(0, add_to_homescreen_manifest_timeout_count_);
    DCHECK_EQ(0, add_to_homescreen_installability_timeout_count_);
  }

  void Resolve(bool check_passed) override {
    // Resolve queued metrics to their appropriate state based on whether or not
    // we passed the installability check.
    if (check_passed) {
      WriteMetricsAndResetCounts(
          InstallabilityCheckStatus::IN_PROGRESS_PROGRESSIVE_WEB_APP,
          AddToHomescreenTimeoutStatus::
              TIMEOUT_MANIFEST_FETCH_PROGRESSIVE_WEB_APP,
          AddToHomescreenTimeoutStatus::
              TIMEOUT_INSTALLABILITY_CHECK_PROGRESSIVE_WEB_APP);
    } else {
      WriteMetricsAndResetCounts(
          InstallabilityCheckStatus::IN_PROGRESS_NON_PROGRESSIVE_WEB_APP,
          AddToHomescreenTimeoutStatus::
              TIMEOUT_MANIFEST_FETCH_NON_PROGRESSIVE_WEB_APP,
          AddToHomescreenTimeoutStatus::
              TIMEOUT_INSTALLABILITY_CHECK_NON_PROGRESSIVE_WEB_APP);
    }
  }

  void Flush(bool waiting_for_service_worker) override {
    InstallabilityCheckStatus status =
        started_ ? InstallabilityCheckStatus::IN_PROGRESS_UNKNOWN
                 : InstallabilityCheckStatus::NOT_STARTED;

    if (waiting_for_service_worker)
      status = InstallabilityCheckStatus::COMPLETE_NON_PROGRESSIVE_WEB_APP;

    WriteMetricsAndResetCounts(
        status, AddToHomescreenTimeoutStatus::TIMEOUT_MANIFEST_FETCH_UNKNOWN,
        AddToHomescreenTimeoutStatus::TIMEOUT_INSTALLABILITY_CHECK_UNKNOWN);
  }

  void RecordMenuOpen() override {
    if (started_)
      ++menu_open_count_;
    else
      WriteMenuOpenHistogram(InstallabilityCheckStatus::NOT_STARTED, 1);
  }

  void RecordMenuItemAddToHomescreen() override {
    if (started_) {
      ++menu_item_add_to_homescreen_count_;
    } else {
      WriteMenuItemAddToHomescreenHistogram(
          InstallabilityCheckStatus::NOT_STARTED, 1);
    }
  }

  void RecordAddToHomescreenNoTimeout() override {
    // If this class is instantiated and there is no timeout, we must have
    // failed the installability check early.
    WriteAddToHomescreenHistogram(
        AddToHomescreenTimeoutStatus::NO_TIMEOUT_NON_PROGRESSIVE_WEB_APP, 1);
  }

  void RecordAddToHomescreenManifestAndIconTimeout() override {
    ++add_to_homescreen_manifest_timeout_count_;
  }

  void RecordAddToHomescreenInstallabilityTimeout() override {
    ++add_to_homescreen_installability_timeout_count_;
  }

  void Start() override { started_ = true; }

 private:
  void WriteMetricsAndResetCounts(
      InstallabilityCheckStatus status,
      AddToHomescreenTimeoutStatus manifest_status,
      AddToHomescreenTimeoutStatus installability_status) {
    WriteMenuOpenHistogram(status, menu_open_count_);
    WriteMenuItemAddToHomescreenHistogram(status,
                                          menu_item_add_to_homescreen_count_);
    WriteAddToHomescreenHistogram(manifest_status,
                                  add_to_homescreen_manifest_timeout_count_);
    WriteAddToHomescreenHistogram(
        installability_status, add_to_homescreen_installability_timeout_count_);

    menu_open_count_ = 0;
    menu_item_add_to_homescreen_count_ = 0;
    add_to_homescreen_manifest_timeout_count_ = 0;
    add_to_homescreen_installability_timeout_count_ = 0;
  }

  // Counts for the number of queued requests of the menu and add to homescreen
  // menu item there have been whilst the installable check is running.
  int menu_open_count_;
  int menu_item_add_to_homescreen_count_;

  // Counts for the number of times the add to homescreen dialog times out at
  // different stages of the installable check.
  int add_to_homescreen_manifest_timeout_count_;
  int add_to_homescreen_installability_timeout_count_;

  // True if we have started working yet.
  bool started_;
};

// Directly writes metrics calls with the current page's PWA status.
class DirectRecorder : public InstallableMetrics::Recorder {
 public:
  explicit DirectRecorder(bool check_passed)
      : InstallableMetrics::Recorder(),
        installability_check_status_(
            check_passed
                ? InstallabilityCheckStatus::COMPLETE_PROGRESSIVE_WEB_APP
                : InstallabilityCheckStatus::COMPLETE_NON_PROGRESSIVE_WEB_APP),
        no_timeout_status_(
            check_passed
                ? AddToHomescreenTimeoutStatus::NO_TIMEOUT_PROGRESSIVE_WEB_APP
                : AddToHomescreenTimeoutStatus::
                      NO_TIMEOUT_NON_PROGRESSIVE_WEB_APP),
        manifest_timeout_status_(
            check_passed ? AddToHomescreenTimeoutStatus::
                               TIMEOUT_MANIFEST_FETCH_PROGRESSIVE_WEB_APP
                         : AddToHomescreenTimeoutStatus::
                               TIMEOUT_MANIFEST_FETCH_NON_PROGRESSIVE_WEB_APP),
        installability_timeout_status_(
            check_passed
                ? AddToHomescreenTimeoutStatus::
                      TIMEOUT_INSTALLABILITY_CHECK_PROGRESSIVE_WEB_APP
                : AddToHomescreenTimeoutStatus::
                      TIMEOUT_INSTALLABILITY_CHECK_NON_PROGRESSIVE_WEB_APP) {}

  ~DirectRecorder() override {}

  void Resolve(bool check_passed) override {}
  void Flush(bool waiting_for_service_worker) override {}
  void Start() override {}
  void RecordMenuOpen() override {
    WriteMenuOpenHistogram(installability_check_status_, 1);
  }

  void RecordMenuItemAddToHomescreen() override {
    WriteMenuItemAddToHomescreenHistogram(installability_check_status_, 1);
  }

  void RecordAddToHomescreenNoTimeout() override {
    WriteAddToHomescreenHistogram(no_timeout_status_, 1);
  }

  void RecordAddToHomescreenManifestAndIconTimeout() override {
    WriteAddToHomescreenHistogram(manifest_timeout_status_, 1);
  }

  void RecordAddToHomescreenInstallabilityTimeout() override {
    WriteAddToHomescreenHistogram(installability_timeout_status_, 1);
  }

  InstallabilityCheckStatus installability_check_status_;
  AddToHomescreenTimeoutStatus no_timeout_status_;
  AddToHomescreenTimeoutStatus manifest_timeout_status_;
  AddToHomescreenTimeoutStatus installability_timeout_status_;
};

}  // anonymous namespace

// static
void InstallableMetrics::TrackInstallEvent(WebappInstallSource source) {
  DCHECK(IsReportableInstallSource(source));
  UMA_HISTOGRAM_ENUMERATION("Webapp.Install.InstallEvent", source,
                            WebappInstallSource::COUNT);
}

// static
bool InstallableMetrics::IsReportableInstallSource(WebappInstallSource source) {
  return source == WebappInstallSource::MENU_BROWSER_TAB ||
         source == WebappInstallSource::MENU_CUSTOM_TAB ||
         source == WebappInstallSource::AUTOMATIC_PROMPT_BROWSER_TAB ||
         source == WebappInstallSource::AUTOMATIC_PROMPT_CUSTOM_TAB ||
         source == WebappInstallSource::API_BROWSER_TAB ||
         source == WebappInstallSource::API_CUSTOM_TAB ||
         source == WebappInstallSource::DEVTOOLS ||
         source == WebappInstallSource::AMBIENT_BADGE_BROWSER_TAB ||
         source == WebappInstallSource::AMBIENT_BADGE_CUSTOM_TAB;
}

// static
WebappInstallSource InstallableMetrics::GetInstallSource(
    content::WebContents* web_contents,
    InstallTrigger trigger) {
  bool is_custom_tab = false;
#if defined(OS_ANDROID)
  is_custom_tab =
      TabAndroid::FromWebContents(web_contents)->IsCurrentlyACustomTab();
#endif

  switch (trigger) {
    case InstallTrigger::AMBIENT_BADGE:
      return is_custom_tab ? WebappInstallSource::AMBIENT_BADGE_CUSTOM_TAB
                           : WebappInstallSource::AMBIENT_BADGE_BROWSER_TAB;
    case InstallTrigger::API:
      return is_custom_tab ? WebappInstallSource::API_CUSTOM_TAB
                           : WebappInstallSource::API_BROWSER_TAB;
    case InstallTrigger::AUTOMATIC_PROMPT:
      return is_custom_tab ? WebappInstallSource::AUTOMATIC_PROMPT_CUSTOM_TAB
                           : WebappInstallSource::AUTOMATIC_PROMPT_BROWSER_TAB;
    case InstallTrigger::MENU:
      return is_custom_tab ? WebappInstallSource::MENU_CUSTOM_TAB
                           : WebappInstallSource::MENU_BROWSER_TAB;
  }
  NOTREACHED();
  return WebappInstallSource::COUNT;
}

InstallableMetrics::InstallableMetrics()
    : recorder_(std::make_unique<AccumulatingRecorder>()) {}

InstallableMetrics::~InstallableMetrics() {}

void InstallableMetrics::RecordMenuOpen() {
  recorder_->RecordMenuOpen();
}

void InstallableMetrics::RecordMenuItemAddToHomescreen() {
  recorder_->RecordMenuItemAddToHomescreen();
}

void InstallableMetrics::RecordAddToHomescreenNoTimeout() {
  recorder_->RecordAddToHomescreenNoTimeout();
}

void InstallableMetrics::RecordAddToHomescreenManifestAndIconTimeout() {
  recorder_->RecordAddToHomescreenManifestAndIconTimeout();
}

void InstallableMetrics::RecordAddToHomescreenInstallabilityTimeout() {
  recorder_->RecordAddToHomescreenInstallabilityTimeout();
}

void InstallableMetrics::Resolve(bool check_passed) {
  recorder_->Resolve(check_passed);
  recorder_ = std::make_unique<DirectRecorder>(check_passed);
}

void InstallableMetrics::Start() {
  recorder_->Start();
}

void InstallableMetrics::Flush(bool waiting_for_service_worker) {
  recorder_->Flush(waiting_for_service_worker);
  recorder_ = std::make_unique<AccumulatingRecorder>();
}
