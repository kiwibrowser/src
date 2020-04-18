// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_TIMEZONE_TIMEZONE_RESOLVER_H_
#define CHROMEOS_TIMEZONE_TIMEZONE_RESOLVER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "chromeos/chromeos_export.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class PrefService;

namespace chromeos {

struct TimeZoneResponseData;

// This class implements periodic timezone synchronization.
class CHROMEOS_EXPORT TimeZoneResolver {
 public:
  class TimeZoneResolverImpl;

  // This callback will be called when new timezone arrives.
  using ApplyTimeZoneCallback =
      base::Callback<void(const TimeZoneResponseData*)>;

  // chromeos::DelayNetworkCall cannot be used directly due to link
  // restrictions.
  using DelayNetworkCallClosure = base::Callback<void(const base::Closure&)>;

  class Delegate {
   public:
    Delegate();
    virtual ~Delegate();

    // Returns true if TimeZoneResolver should include WiFi data in request.
    virtual bool ShouldSendWiFiGeolocationData() = 0;

    // Returns true if TimeZoneResolver should include Cellular data in request.
    virtual bool ShouldSendCellularGeolocationData() = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  // This is a LocalState preference to store base::Time value of the last
  // request. It is used to limit request rate on browser restart.
  static const char kLastTimeZoneRefreshTime[];

  TimeZoneResolver(Delegate* delegate,
                   scoped_refptr<net::URLRequestContextGetter> context,
                   const GURL& url,
                   const ApplyTimeZoneCallback& apply_timezone,
                   const DelayNetworkCallClosure& delay_network_call,
                   PrefService* local_state);
  ~TimeZoneResolver();

  // Starts periodic timezone refresh.
  void Start();

  // Cancels current request and stops periodic timezone refresh.
  void Stop();

  // Register prefs to LocalState.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  scoped_refptr<net::URLRequestContextGetter> context() const {
    return context_;
  }

  DelayNetworkCallClosure delay_network_call() const {
    return delay_network_call_;
  }

  ApplyTimeZoneCallback apply_timezone() const { return apply_timezone_; }

  PrefService* local_state() const { return local_state_; }

  // Proxy call to Delegate::ShouldSendWiFiGeolocationData().
  bool ShouldSendWiFiGeolocationData() const;

  // Proxy call to Delegate::ShouldSendCellularGeolocationData().
  bool ShouldSendCellularGeolocationData() const;

  // Expose internal fuctions for testing.
  static int MaxRequestsCountForIntervalForTesting(
      const double interval_seconds);
  static int IntervalForNextRequestForTesting(const int requests);

 private:
  Delegate* delegate_;

  scoped_refptr<net::URLRequestContextGetter> context_;
  const GURL url_;

  const ApplyTimeZoneCallback apply_timezone_;
  const DelayNetworkCallClosure delay_network_call_;
  PrefService* local_state_;

  std::unique_ptr<TimeZoneResolverImpl> implementation_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(TimeZoneResolver);
};

}  // namespace chromeos

#endif  // CHROMEOS_TIMEZONE_TIMEZONE_RESOLVER_H_
