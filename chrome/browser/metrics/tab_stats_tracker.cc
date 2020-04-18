// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_stats_tracker.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/power_monitor/power_monitor.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/pref_names.h"
#include "components/metrics/daily_event.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
#include "chrome/browser/background/background_mode_manager.h"
#endif  // BUILDFLAG(ENABLE_BACKGROUND_MODE)

namespace metrics {

namespace {

// The interval at which the DailyEvent::CheckInterval function should be
// called.
constexpr base::TimeDelta kDailyEventIntervalTimeDelta =
    base::TimeDelta::FromMilliseconds(60 * 30);

// The intervals at which we report the number of unused tabs. This is used for
// all the tab usage histograms listed below.
//
// The 'Tabs.TabUsageIntervalLength' histogram suffixes entry in histograms.xml
// should be kept in sync with these values.
constexpr base::TimeDelta kTabUsageReportingIntervals[] = {
    base::TimeDelta::FromSeconds(30), base::TimeDelta::FromMinutes(1),
    base::TimeDelta::FromMinutes(10), base::TimeDelta::FromHours(1),
    base::TimeDelta::FromHours(5),    base::TimeDelta::FromHours(12)};

#if defined(OS_WIN)
const base::TimeDelta kNativeWindowOcclusionCalculationInterval =
    base::TimeDelta::FromMinutes(10);
#endif

// The global TabStatsTracker instance.
TabStatsTracker* g_tab_stats_tracker_instance = nullptr;

// Ensure that an interval is a valid one (i.e. listed in
// |kTabUsageReportingIntervals|).
bool IsValidInterval(base::TimeDelta interval) {
  return base::ContainsValue(kTabUsageReportingIntervals, interval);
}

}  // namespace

// static
const char TabStatsTracker::kTabStatsDailyEventHistogramName[] =
    "Tabs.TabsStatsDailyEventInterval";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kNumberOfTabsOnResumeHistogramName[] = "Tabs.NumberOfTabsOnResume";
const char
    TabStatsTracker::UmaStatsReportingDelegate::kMaxTabsInADayHistogramName[] =
        "Tabs.MaxTabsInADay";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kMaxTabsPerWindowInADayHistogramName[] = "Tabs.MaxTabsPerWindowInADay";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kMaxWindowsInADayHistogramName[] = "Tabs.MaxWindowsInADay";

// Tab usage histograms.
const char TabStatsTracker::UmaStatsReportingDelegate::
    kUnusedAndClosedInIntervalHistogramNameBase[] =
        "Tabs.UnusedAndClosedInInterval.Count";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kUnusedTabsInIntervalHistogramNameBase[] = "Tabs.UnusedInInterval.Count";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kUsedAndClosedInIntervalHistogramNameBase[] =
        "Tabs.UsedAndClosedInInterval.Count";
const char TabStatsTracker::UmaStatsReportingDelegate::
    kUsedTabsInIntervalHistogramNameBase[] = "Tabs.UsedInInterval.Count";

const TabStatsDataStore::TabsStats& TabStatsTracker::tab_stats() const {
  return tab_stats_data_store_->tab_stats();
}

TabStatsTracker::TabStatsTracker(PrefService* pref_service)
    : reporting_delegate_(std::make_unique<UmaStatsReportingDelegate>()),
      delegate_(std::make_unique<TabStatsTrackerDelegate>()),
      tab_stats_data_store_(std::make_unique<TabStatsDataStore>(pref_service)),
      daily_event_(
          std::make_unique<DailyEvent>(pref_service,
                                       ::prefs::kTabStatsDailySample,
                                       kTabStatsDailyEventHistogramName)) {
  DCHECK(pref_service);
  // Get the list of existing windows/tabs. There shouldn't be any if this is
  // initialized at startup but this will ensure that the counts stay accurate
  // if the initialization gets moved to after the creation of the first tab.
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list) {
    OnBrowserAdded(browser);
    for (int i = 0; i < browser->tab_strip_model()->count(); ++i)
      OnInitialOrInsertedTab(browser->tab_strip_model()->GetWebContentsAt(i));
    tab_stats_data_store_->UpdateMaxTabsPerWindowIfNeeded(
        static_cast<size_t>(browser->tab_strip_model()->count()));
  }

  browser_list->AddObserver(this);
  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  if (power_monitor)
    power_monitor->AddObserver(this);

  daily_event_->AddObserver(std::make_unique<TabStatsDailyObserver>(
      reporting_delegate_.get(), tab_stats_data_store_.get()));
  // Call the CheckInterval method to see if the data need to be immediately
  // reported.
  daily_event_->CheckInterval();
  timer_.Start(FROM_HERE, kDailyEventIntervalTimeDelta, daily_event_.get(),
               &DailyEvent::CheckInterval);

  // Initialize the interval maps and timers associated with them.
  for (base::TimeDelta interval : kTabUsageReportingIntervals) {
    TabStatsDataStore::TabsStateDuringIntervalMap* interval_map =
        tab_stats_data_store_->AddInterval();
    // Setup the timer associated with this interval.
    std::unique_ptr<base::RepeatingTimer> timer =
        std::make_unique<base::RepeatingTimer>();
    timer->Start(FROM_HERE, interval,
                 Bind(&TabStatsTracker::OnInterval, base::Unretained(this),
                      interval, interval_map));
    usage_interval_timers_.push_back(std::move(timer));
  }

// The native window occlusion calculation is specific to Windows.
#if defined(OS_WIN)
  native_window_occlusion_timer_.Start(
      FROM_HERE, kNativeWindowOcclusionCalculationInterval,
      Bind(&TabStatsTracker::CalculateAndRecordNativeWindowVisibilities,
           base::Unretained(this)));
#endif
}

TabStatsTracker::~TabStatsTracker() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list)
    browser->tab_strip_model()->RemoveObserver(this);

  browser_list->RemoveObserver(this);

  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  if (power_monitor)
    power_monitor->RemoveObserver(this);
}

// static
void TabStatsTracker::SetInstance(std::unique_ptr<TabStatsTracker> instance) {
  DCHECK_EQ(nullptr, g_tab_stats_tracker_instance);
  g_tab_stats_tracker_instance = instance.release();
}

TabStatsTracker* TabStatsTracker::GetInstance() {
  return g_tab_stats_tracker_instance;
}

void TabStatsTracker::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(::prefs::kTabStatsTotalTabCountMax, 0);
  registry->RegisterIntegerPref(::prefs::kTabStatsMaxTabsPerWindow, 0);
  registry->RegisterIntegerPref(::prefs::kTabStatsWindowCountMax, 0);
  DailyEvent::RegisterPref(registry, ::prefs::kTabStatsDailySample);
}

void TabStatsTracker::SetDelegateForTesting(
    std::unique_ptr<TabStatsTrackerDelegate> new_delegate) {
  delegate_ = std::move(new_delegate);
}

void TabStatsTracker::TabStatsDailyObserver::OnDailyEvent(
    DailyEvent::IntervalType type) {
  reporting_delegate_->ReportDailyMetrics(data_store_->tab_stats());
  data_store_->ResetMaximumsToCurrentState();
}

class TabStatsTracker::WebContentsUsageObserver
    : public content::WebContentsObserver {
 public:
  WebContentsUsageObserver(content::WebContents* web_contents,
                           TabStatsTracker* tab_stats_tracker)
      : content::WebContentsObserver(web_contents),
        tab_stats_tracker_(tab_stats_tracker) {}

  // content::WebContentsObserver:
  void DidGetUserInteraction(const blink::WebInputEvent::Type type) override {
    tab_stats_tracker_->tab_stats_data_store()->OnTabInteraction(
        web_contents());
  }

  void OnVisibilityChanged(content::Visibility visibility) override {
    if (visibility == content::Visibility::VISIBLE)
      tab_stats_tracker_->tab_stats_data_store()->OnTabVisible(web_contents());
  }

  void WebContentsDestroyed() override {
    tab_stats_tracker_->OnWebContentsDestroyed(web_contents());

    // The call above will free |this| and so nothing should be done on this
    // object starting from here.
  }

 private:
  TabStatsTracker* tab_stats_tracker_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsUsageObserver);
};

void TabStatsTracker::OnBrowserAdded(Browser* browser) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_stats_data_store_->OnWindowAdded();
  browser->tab_strip_model()->AddObserver(this);
}

void TabStatsTracker::OnBrowserRemoved(Browser* browser) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_stats_data_store_->OnWindowRemoved();
  browser->tab_strip_model()->RemoveObserver(this);
}

void TabStatsTracker::TabInsertedAt(TabStripModel* model,
                                    content::WebContents* web_contents,
                                    int index,
                                    bool foreground) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  OnInitialOrInsertedTab(web_contents);

  tab_stats_data_store_->UpdateMaxTabsPerWindowIfNeeded(
      static_cast<size_t>(model->count()));
}

void TabStatsTracker::TabChangedAt(content::WebContents* web_contents,
                                   int index,
                                   TabChangeType change_type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Ignore 'loading' and 'title' changes, we're only interested in audio here.
  if (change_type != TabChangeType::kAll)
    return;
  if (web_contents->IsCurrentlyAudible())
    tab_stats_data_store_->OnTabAudible(web_contents);
}

void TabStatsTracker::TabReplacedAt(TabStripModel* tab_strip_model,
                                    content::WebContents* old_contents,
                                    content::WebContents* new_contents,
                                    int index) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_stats_data_store_->OnTabReplaced(old_contents, new_contents);
  web_contents_usage_observers_.insert(std::make_pair(
      new_contents,
      std::make_unique<WebContentsUsageObserver>(new_contents, this)));
  web_contents_usage_observers_.erase(old_contents);
}

void TabStatsTracker::OnResume() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  reporting_delegate_->ReportTabCountOnResume(
      tab_stats_data_store_->tab_stats().total_tab_count);
}

void TabStatsTracker::OnInterval(
    base::TimeDelta interval,
    TabStatsDataStore::TabsStateDuringIntervalMap* interval_map) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(interval_map);
  reporting_delegate_->ReportUsageDuringInterval(*interval_map, interval);
  // Reset the interval data.
  tab_stats_data_store_->ResetIntervalData(interval_map);
}

void TabStatsTracker::OnInitialOrInsertedTab(
    content::WebContents* web_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // If we already have a WebContentsObserver for this tab then it means that
  // it's already tracked and it's being dragged into a new window, there's
  // nothing to do here.
  if (!base::ContainsKey(web_contents_usage_observers_, web_contents)) {
    tab_stats_data_store_->OnTabAdded(web_contents);
    web_contents_usage_observers_.insert(std::make_pair(
        web_contents,
        std::make_unique<WebContentsUsageObserver>(web_contents, this)));
  }
}

void TabStatsTracker::OnWebContentsDestroyed(
    content::WebContents* web_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(base::ContainsKey(web_contents_usage_observers_, web_contents));
  web_contents_usage_observers_.erase(
      web_contents_usage_observers_.find(web_contents));
  tab_stats_data_store_->OnTabRemoved(web_contents);
}

void TabStatsTracker::UmaStatsReportingDelegate::ReportTabCountOnResume(
    size_t tab_count) {
  // Don't report the number of tabs on resume if Chrome is running in
  // background with no visible window.
  if (IsChromeBackgroundedWithoutWindows())
    return;
  UMA_HISTOGRAM_COUNTS_10000(kNumberOfTabsOnResumeHistogramName, tab_count);
}

void TabStatsTracker::UmaStatsReportingDelegate::ReportDailyMetrics(
    const TabStatsDataStore::TabsStats& tab_stats) {
  // Don't report the counts if they're equal to 0, this means that Chrome has
  // only been running in the background since the last time the metrics have
  // been reported.
  if (tab_stats.total_tab_count_max == 0)
    return;
  UMA_HISTOGRAM_COUNTS_10000(kMaxTabsInADayHistogramName,
                             tab_stats.total_tab_count_max);
  UMA_HISTOGRAM_COUNTS_10000(kMaxTabsPerWindowInADayHistogramName,
                             tab_stats.max_tab_per_window);
  UMA_HISTOGRAM_COUNTS_10000(kMaxWindowsInADayHistogramName,
                             tab_stats.window_count_max);
}

void TabStatsTracker::UmaStatsReportingDelegate::ReportUsageDuringInterval(
    const TabStatsDataStore::TabsStateDuringIntervalMap& interval_map,
    base::TimeDelta interval) {
  // Counts the number of used/unused tabs during this interval, a tabs counts
  // as unused if it hasn't been interacted with or visible during the duration
  // of the interval.
  size_t used_tabs = 0;
  size_t used_and_closed_tabs = 0;
  size_t unused_tabs = 0;
  size_t unused_and_closed_tabs = 0;
  for (const auto& iter : interval_map) {
    // There's currently no distinction between a visible/audible tab and one
    // that has been interacted with in these metrics.
    // TODO(sebmarchand): Add a metric that track the number of tab that have
    // been visible/audible but not interacted with during an interval,
    // https://crbug.com/800828.
    if (iter.second.interacted_during_interval ||
        iter.second.visible_or_audible_during_interval) {
      if (iter.second.exists_currently)
        ++used_tabs;
      else
        ++used_and_closed_tabs;
    } else {
      if (iter.second.exists_currently)
        ++unused_tabs;
      else
        ++unused_and_closed_tabs;
    }
  }

  std::string used_and_closed_histogram_name = GetIntervalHistogramName(
      UmaStatsReportingDelegate::kUsedAndClosedInIntervalHistogramNameBase,
      interval);
  std::string used_histogram_name = GetIntervalHistogramName(
      UmaStatsReportingDelegate::kUsedTabsInIntervalHistogramNameBase,
      interval);
  std::string unused_and_closed_histogram_name = GetIntervalHistogramName(
      UmaStatsReportingDelegate::kUnusedAndClosedInIntervalHistogramNameBase,
      interval);
  std::string unused_histogram_name = GetIntervalHistogramName(
      UmaStatsReportingDelegate::kUnusedTabsInIntervalHistogramNameBase,
      interval);

  base::UmaHistogramCounts10000(used_and_closed_histogram_name,
                                used_and_closed_tabs);
  base::UmaHistogramCounts10000(used_histogram_name, used_tabs);
  base::UmaHistogramCounts10000(unused_and_closed_histogram_name,
                                unused_and_closed_tabs);
  base::UmaHistogramCounts10000(unused_histogram_name, unused_tabs);
}

// static
std::string
TabStatsTracker::UmaStatsReportingDelegate::GetIntervalHistogramName(
    const char* base_name,
    base::TimeDelta interval) {
  DCHECK(IsValidInterval(interval));
  return base::StringPrintf("%s_%zu", base_name,
                            static_cast<size_t>(interval.InSeconds()));
}

bool TabStatsTracker::UmaStatsReportingDelegate::
    IsChromeBackgroundedWithoutWindows() {
#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
  if (g_browser_process && g_browser_process->background_mode_manager()
                               ->IsBackgroundWithoutWindows()) {
    return true;
  }
#endif  // BUILDFLAG(ENABLE_BACKGROUND_MODE)
  return false;
}

}  // namespace metrics
