// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_LOCAL_NTP_SOURCE_H_
#define CHROME_BROWSER_SEARCH_LOCAL_NTP_SOURCE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/scoped_observer.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/search/background/ntp_background_service_observer.h"
#include "chrome/browser/search/one_google_bar/one_google_bar_service_observer.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/url_data_source.h"

#if defined(OS_ANDROID)
#error "Instant is only used on desktop";
#endif

struct OneGoogleBarData;
class NtpBackgroundService;
class OneGoogleBarService;
class Profile;

namespace search_provider_logos {
class LogoService;
}  // namespace search_provider_logos

// Serves HTML and resources for the local New Tab page, i.e.
// chrome-search://local-ntp/local-ntp.html.
// WARNING: Due to the threading model of URLDataSource, some methods of this
// class are called on the UI thread, others on the IO thread. All data members
// live on the UI thread, so make sure not to access them from the IO thread!
// To prevent accidental access, all methods that get called on the IO thread
// are implemented as non-member functions.
class LocalNtpSource : public content::URLDataSource,
                       public NtpBackgroundServiceObserver,
                       public OneGoogleBarServiceObserver {
 public:
  explicit LocalNtpSource(Profile* profile);

 private:
  class GoogleSearchProviderTracker;
  class DesktopLogoObserver;

  struct NtpBackgroundRequest {
    NtpBackgroundRequest(
        base::TimeTicks start_time,
        const content::URLDataSource::GotDataCallback& callback);
    NtpBackgroundRequest(const NtpBackgroundRequest&);
    ~NtpBackgroundRequest();

    base::TimeTicks start_time;
    content::URLDataSource::GotDataCallback callback;
  };

  struct OneGoogleBarRequest {
    OneGoogleBarRequest(
        base::TimeTicks start_time,
        const content::URLDataSource::GotDataCallback& callback);
    OneGoogleBarRequest(const OneGoogleBarRequest&);
    ~OneGoogleBarRequest();

    base::TimeTicks start_time;
    content::URLDataSource::GotDataCallback callback;
  };

  ~LocalNtpSource() override;

  // Overridden from content::URLDataSource:
  std::string GetSource() const override;
  void StartDataRequest(
      const std::string& path,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const content::URLDataSource::GotDataCallback& callback) override;
  std::string GetMimeType(const std::string& path) const override;
  bool AllowCaching() const override;
  bool ShouldServiceRequest(const GURL& url,
                            content::ResourceContext* resource_context,
                            int render_process_id) const override;
  std::string GetContentSecurityPolicyScriptSrc() const override;
  std::string GetContentSecurityPolicyChildSrc() const override;

  // Overridden from NtpBackgroundServiceObserver:
  void OnCollectionInfoAvailable() override;
  void OnCollectionImagesAvailable() override;
  void OnNtpBackgroundServiceShuttingDown() override;

  // Overridden from OneGoogleBarServiceObserver:
  void OnOneGoogleBarDataUpdated() override;
  void OnOneGoogleBarServiceShuttingDown() override;

  void ServeOneGoogleBar(const base::Optional<OneGoogleBarData>& data);

  Profile* const profile_;

  std::vector<NtpBackgroundRequest> ntp_background_collections_requests_;
  std::vector<NtpBackgroundRequest> ntp_background_image_info_requests_;

  NtpBackgroundService* ntp_background_service_;

  ScopedObserver<NtpBackgroundService, NtpBackgroundServiceObserver>
      ntp_background_service_observer_;

  std::vector<OneGoogleBarRequest> one_google_bar_requests_;

  OneGoogleBarService* one_google_bar_service_;

  ScopedObserver<OneGoogleBarService, OneGoogleBarServiceObserver>
      one_google_bar_service_observer_;

  search_provider_logos::LogoService* logo_service_;
  std::unique_ptr<DesktopLogoObserver> logo_observer_;

  std::unique_ptr<GoogleSearchProviderTracker> google_tracker_;

  base::WeakPtrFactory<LocalNtpSource> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LocalNtpSource);
};

#endif  // CHROME_BROWSER_SEARCH_LOCAL_NTP_SOURCE_H_
