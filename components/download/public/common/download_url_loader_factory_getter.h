// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_URL_LOADER_FACTORY_GETTER_H_
#define COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_URL_LOADER_FACTORY_GETTER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner_helpers.h"
#include "components/download/public/common/download_export.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace download {

struct DownloadURLLoaderFactoryGetterDeleter;

// Class for retrieving a URLLoaderFactory on IO thread. This class can be
// created on any thread and will be destroyed on the IO thread.
// GetURLLoaderFactory() has to be called on the IO thread.
class COMPONENTS_DOWNLOAD_EXPORT DownloadURLLoaderFactoryGetter
    : public base::RefCountedThreadSafe<DownloadURLLoaderFactoryGetter,
                                        DownloadURLLoaderFactoryGetterDeleter> {
 public:
  DownloadURLLoaderFactoryGetter();

  // Called on the IO thread to get a URLLoaderFactory.
  virtual scoped_refptr<network::SharedURLLoaderFactory>
  GetURLLoaderFactory() = 0;

 protected:
  virtual ~DownloadURLLoaderFactoryGetter();

 private:
  friend class base::DeleteHelper<DownloadURLLoaderFactoryGetter>;
  friend class base::RefCountedThreadSafe<
      DownloadURLLoaderFactoryGetter,
      DownloadURLLoaderFactoryGetterDeleter>;
  friend struct DownloadURLLoaderFactoryGetterDeleter;

  void DeleteOnCorrectThread() const;

  DISALLOW_COPY_AND_ASSIGN(DownloadURLLoaderFactoryGetter);
};

struct DownloadURLLoaderFactoryGetterDeleter {
  static void Destruct(const DownloadURLLoaderFactoryGetter* factory_getter) {
    factory_getter->DeleteOnCorrectThread();
  }
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_URL_LOADER_FACTORY_GETTER_H_
