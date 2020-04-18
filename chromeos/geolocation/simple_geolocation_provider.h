// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_GEOLOCATION_SIMPLE_GEOLOCATION_PROVIDER_H_
#define CHROMEOS_GEOLOCATION_SIMPLE_GEOLOCATION_PROVIDER_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/geolocation/simple_geolocation_request.h"
#include "url/gurl.h"

namespace net {
class URLRequestContextGetter;
}

namespace chromeos {

// This class implements Google Maps Geolocation API.
//
// SimpleGeolocationProvider must be created and used on the same thread.
//
// Note: this should probably be a singleton to monitor requests rate.
// But as it is used only diring ChromeOS Out-of-Box, it can be owned by
// WizardController for now.
class CHROMEOS_EXPORT SimpleGeolocationProvider {
 public:
  SimpleGeolocationProvider(net::URLRequestContextGetter* url_context_getter,
                            const GURL& url);
  virtual ~SimpleGeolocationProvider();

  // Initiates new request. If |send_wifi_access_points|, WiFi AP information
  // will be added to the request, similarly for |send_cell_towers| and Cell
  // Tower information. See SimpleGeolocationRequest for the description of
  // the other parameters.
  void RequestGeolocation(base::TimeDelta timeout,
                          bool send_wifi_access_points,
                          bool send_cell_towers,
                          SimpleGeolocationRequest::ResponseCallback callback);

  // Returns default geolocation service URL.
  static GURL DefaultGeolocationProviderURL();

 private:
  friend class TestGeolocationAPIURLFetcherCallback;

  // Geolocation response callback. Deletes request from requests_.
  void OnGeolocationResponse(
      SimpleGeolocationRequest* request,
      SimpleGeolocationRequest::ResponseCallback callback,
      const Geoposition& geoposition,
      bool server_error,
      const base::TimeDelta elapsed);

  scoped_refptr<net::URLRequestContextGetter> url_context_getter_;

  // URL of the Google Maps Geolocation API.
  const GURL url_;

  // Requests in progress.
  // SimpleGeolocationProvider owns all requests, so this vector is deleted on
  // destroy.
  std::vector<std::unique_ptr<SimpleGeolocationRequest>> requests_;

  // Creation and destruction should happen on the same thread.
  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(SimpleGeolocationProvider);
};

}  // namespace chromeos

#endif  // CHROMEOS_GEOLOCATION_SIMPLE_GEOLOCATION_PROVIDER_H_
