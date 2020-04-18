// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_JOB_FACTORY_H_
#define COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_JOB_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/download/public/common/download_create_info.h"
#include "components/download/public/common/download_export.h"

namespace net {
class URLRequestContextGetter;
}

namespace download {

class DownloadItem;
class DownloadJob;
class DownloadRequestHandleInterface;
class DownloadURLLoaderFactoryGetter;

// Factory class to create different kinds of DownloadJob.
class COMPONENTS_DOWNLOAD_EXPORT DownloadJobFactory {
 public:
  static std::unique_ptr<DownloadJob> CreateJob(
      DownloadItem* download_item,
      std::unique_ptr<DownloadRequestHandleInterface> req_handle,
      const DownloadCreateInfo& create_info,
      bool is_save_package_download,
      scoped_refptr<download::DownloadURLLoaderFactoryGetter>
          url_loader_factory_getter,
      net::URLRequestContextGetter* url_request_context_getter);

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadJobFactory);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_JOB_FACTORY_H_
