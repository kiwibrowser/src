// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FAVICON_CORE_FAVICON_SERVER_FETCHER_PARAMS_H_
#define COMPONENTS_FAVICON_CORE_FAVICON_SERVER_FETCHER_PARAMS_H_

#include "components/favicon_base/favicon_util.h"

class GURL;

namespace favicon {

class FaviconServerFetcherParams {
 public:
  // Platform specific constructors that set default fetch parameters. Any
  // platform not Android nor iOS is considered desktop.
  static std::unique_ptr<FaviconServerFetcherParams> CreateForDesktop(
      const GURL& page_url);
  static std::unique_ptr<FaviconServerFetcherParams> CreateForMobile(
      const GURL& page_url,
      int min_source_size_in_pixel,
      int desired_size_in_pixel);

  ~FaviconServerFetcherParams();

  const GURL& page_url() const { return page_url_; };
  favicon_base::IconType icon_type() const { return icon_type_; };
  int min_source_size_in_pixel() const { return min_source_size_in_pixel_; };
  int desired_size_in_pixel() const { return desired_size_in_pixel_; };
  const std::string& google_server_client_param() const {
    return google_server_client_param_;
  };

 private:
  FaviconServerFetcherParams(const GURL& page_url,
                             favicon_base::IconType icon_type,
                             int min_source_size_in_pixel,
                             int desired_size_in_pixel,
                             const std::string& google_server_client_param);

  const GURL& page_url_;
  favicon_base::IconType icon_type_;
  int min_source_size_in_pixel_;
  int desired_size_in_pixel_;
  std::string google_server_client_param_;

  DISALLOW_COPY_AND_ASSIGN(FaviconServerFetcherParams);
};

}  // namespace favicon

#endif  // COMPONENTS_FAVICON_CORE_FAVICON_SERVER_FETCHER_PARAMS_H_
