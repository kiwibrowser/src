// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SEARCH_PROVIDER_LOGOS_ANDROID_LOGO_SERVICE_IMPL_H_
#define COMPONENTS_SEARCH_PROVIDER_LOGOS_ANDROID_LOGO_SERVICE_IMPL_H_

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/search_provider_logos/logo_common.h"
#include "components/search_provider_logos/logo_service.h"

class GaiaCookieManagerService;
class TemplateURLService;

namespace base {
class Clock;
}  // namespace base

namespace image_fetcher {
class ImageDecoder;
}  // namespace image_fetcher

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace search_provider_logos {

class LogoCache;
class LogoTracker;
class LogoObserver;

class LogoServiceImpl : public LogoService {
 public:
  LogoServiceImpl(
      const base::FilePath& cache_directory,
      GaiaCookieManagerService* cookie_service,
      TemplateURLService* template_url_service,
      std::unique_ptr<image_fetcher::ImageDecoder> image_decoder,
      scoped_refptr<net::URLRequestContextGetter> request_context_getter,
      base::RepeatingCallback<bool()> want_gray_logo_getter);

  ~LogoServiceImpl() override;

  // KeyedService implementation.
  void Shutdown() override;

  // LogoService implementation.
  void GetLogo(LogoCallbacks callbacks) override;
  void GetLogo(LogoObserver* observer) override;

  // Overrides the cache used to store logos.
  void SetLogoCacheForTests(std::unique_ptr<LogoCache> cache);

  // Overrides the clock used to check the time.
  void SetClockForTests(base::Clock* clock);

 private:
  class SigninObserver;

  void InitializeLogoTrackerIfNecessary();

  void SigninStatusChanged();

  // Constructor arguments.
  const base::FilePath cache_directory_;
  TemplateURLService* const template_url_service_;
  const scoped_refptr<net::URLRequestContextGetter> request_context_getter_;

  // Callback to get the type of logo to fetch. Returns whether we want a logo
  // optimized for gray backgrounds or not.
  base::RepeatingCallback<bool()> want_gray_logo_getter_;

  // logo_tracker_ takes ownership if/when it is initialized.
  std::unique_ptr<image_fetcher::ImageDecoder> image_decoder_;

  std::unique_ptr<SigninObserver> signin_observer_;

  // For testing.
  base::Clock* clock_for_test_ = nullptr;

  // For testing. logo_tracker_ takes ownership if/when it is initialized.
  std::unique_ptr<LogoCache> logo_cache_for_test_;

  // Lazily initialized on first call to GetLogo().
  std::unique_ptr<search_provider_logos::LogoTracker> logo_tracker_;

  DISALLOW_COPY_AND_ASSIGN(LogoServiceImpl);
};

}  // namespace search_provider_logos

#endif  // COMPONENTS_SEARCH_PROVIDER_LOGOS_ANDROID_LOGO_SERVICE_IMPL_H_
