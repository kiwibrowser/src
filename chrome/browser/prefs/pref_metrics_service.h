// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREFS_PREF_METRICS_SERVICE_H_
#define CHROME_BROWSER_PREFS_PREF_METRICS_SERVICE_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sync_preferences/synced_pref_change_registrar.h"

// PrefMetricsService is responsible for recording prefs-related UMA stats.
class PrefMetricsService : public KeyedService {
 public:
  explicit PrefMetricsService(Profile* profile);
  ~PrefMetricsService() override;

  // Records metrics about the state of the homepage on launch.
  static void RecordHomePageLaunchMetrics(bool show_home_button,
                                          bool homepage_is_ntp,
                                          const GURL& homepage_url);

  class Factory : public BrowserContextKeyedServiceFactory {
   public:
    static Factory* GetInstance();
    static PrefMetricsService* GetForProfile(Profile* profile);
   private:
    friend struct base::DefaultSingletonTraits<Factory>;

    Factory();
    ~Factory() override;

    // BrowserContextKeyedServiceFactory implementation
    KeyedService* BuildServiceInstanceFor(
        content::BrowserContext* profile) const override;
    bool ServiceIsCreatedWithBrowserContext() const override;
    bool ServiceIsNULLWhileTesting() const override;
    content::BrowserContext* GetBrowserContextToUse(
        content::BrowserContext* context) const override;
  };

 private:
  friend class PrefMetricsServiceTest;

  // Function to log a Value to a histogram
  typedef base::Callback<void(const std::string&, const base::Value*)>
      LogHistogramValueCallback;

  // For unit testing only.
  PrefMetricsService(Profile* profile, PrefService* local_settings);

  // Record prefs state on browser context creation.
  void RecordLaunchPrefs();

  // Register callbacks for synced pref changes.
  void RegisterSyncedPrefObservers();

  // Registers a histogram logging callback for a synced pref change.
  void AddPrefObserver(const std::string& path,
                       const std::string& histogram_name_prefix,
                       const LogHistogramValueCallback& callback);

  // Generic callback to observe a synced pref change.
  void OnPrefChanged(const std::string& histogram_name_prefix,
                     const LogHistogramValueCallback& callback,
                     const std::string& path,
                     bool from_sync);

  // Callback for a boolean pref change histogram.
  void LogBooleanPrefChange(const std::string& histogram_name,
                            const base::Value* value);

  // Callback for an integer pref change histogram.
  void LogIntegerPrefChange(int boundary_value,
                            const std::string& histogram_name,
                            const base::Value* value);

  Profile* profile_;
  PrefService* prefs_;
  PrefService* local_state_;

  std::unique_ptr<sync_preferences::SyncedPrefChangeRegistrar>
      synced_pref_change_registrar_;

  base::WeakPtrFactory<PrefMetricsService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrefMetricsService);
};

#endif  // CHROME_BROWSER_PREFS_PREF_METRICS_SERVICE_H_
