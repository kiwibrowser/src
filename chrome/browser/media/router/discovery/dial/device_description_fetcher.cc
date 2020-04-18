// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/dial/device_description_fetcher.h"

#include "base/strings/stringprintf.h"
#include "chrome/browser/media/router/discovery/dial/dial_device_data.h"
#include "net/http/http_response_headers.h"

constexpr char kApplicationUrlHeaderName[] = "Application-URL";

namespace media_router {

DeviceDescriptionFetcher::DeviceDescriptionFetcher(
    const GURL& device_description_url,
    base::OnceCallback<void(const DialDeviceDescriptionData&)> success_cb,
    base::OnceCallback<void(const std::string&)> error_cb)
    : device_description_url_(device_description_url),
      success_cb_(std::move(success_cb)),
      error_cb_(std::move(error_cb)) {
  DCHECK(device_description_url_.is_valid());
}

DeviceDescriptionFetcher::~DeviceDescriptionFetcher() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void DeviceDescriptionFetcher::Start() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!fetcher_);

  fetcher_ = std::make_unique<DialURLFetcher>(
      base::BindOnce(&DeviceDescriptionFetcher::ProcessResponse,
                     base::Unretained(this)),
      base::BindOnce(&DeviceDescriptionFetcher::ReportError,
                     base::Unretained(this)));

  fetcher_->Get(device_description_url_);
}

void DeviceDescriptionFetcher::ProcessResponse(const std::string& response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(fetcher_);

  const network::ResourceResponseHead* response_info =
      fetcher_->GetResponseHead();
  DCHECK(response_info);

  // NOTE: The uPnP spec requires devices to set a Content-Type: header of
  // text/xml; charset="utf-8" (sec 2.11).  However Chromecast (and possibly
  // other devices) do not comply, so specifically not checking this header.
  std::string app_url_header;
  if (!response_info->headers ||
      !response_info->headers->GetNormalizedHeader(kApplicationUrlHeaderName,
                                                   &app_url_header) ||
      app_url_header.empty()) {
    ReportError(net::Error::OK, "Missing or empty Application-URL:");
    return;
  }

  // Section 5.4 of the DIAL spec implies that the Application URL should not
  // have path, query or fragment...unsure if that can be enforced.
  GURL app_url(app_url_header);
  if (!app_url.is_valid() || !app_url.SchemeIs("http") ||
      !app_url.HostIsIPAddress() ||
      app_url.host() != device_description_url_.host()) {
    ReportError(net::Error::OK,
                base::StringPrintf("Invalid Application-URL: %s",
                                   app_url_header.c_str()));
    return;
  }

  // Remove trailing slash if there is any.
  if (app_url.ExtractFileName().empty()) {
    DVLOG(2) << "App url has trailing slash: " << app_url_header;
    app_url = GURL(app_url_header.substr(0, app_url_header.length() - 1));
  }

  std::move(success_cb_).Run(DialDeviceDescriptionData(response, app_url));
}

void DeviceDescriptionFetcher::ReportError(int response_code,
                                           const std::string& message) {
  std::move(error_cb_).Run(message);
}

}  // namespace media_router
