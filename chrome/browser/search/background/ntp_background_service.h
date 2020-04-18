// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_BACKGROUND_NTP_BACKGROUND_SERVICE_H_
#define CHROME_BROWSER_SEARCH_BACKGROUND_NTP_BACKGROUND_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "chrome/browser/search/background/ntp_background_data.h"
#include "chrome/browser/search/background/ntp_background_service_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

// A service that connects to backends that provide background image
// information, including collection names, image urls and descriptions.
class NtpBackgroundService : public KeyedService {
 public:
  NtpBackgroundService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const base::Optional<GURL>& collections_api_url_override,
      const base::Optional<GURL>& collection_images_api_url_override,
      const base::Optional<std::string>& image_options_override);
  ~NtpBackgroundService() override;

  // KeyedService implementation.
  void Shutdown() override;

  // Requests an asynchronous fetch from the network. After the update
  // completes, OnCollectionInfoAvailable will be called on the observers.
  void FetchCollectionInfo();

  // Requests an asynchronous fetch of metadata about images in the specified
  // collection. After the update completes, OnCollectionImagesAvailable will be
  // called on the observers. Requests that are made while an asynchronous fetch
  // is in progress will be dropped until the currently active loader completes.
  void FetchCollectionImageInfo(const std::string& collection_id);

  // Add/remove observers. All observers must unregister themselves before the
  // NtpBackgroundService is destroyed.
  void AddObserver(NtpBackgroundServiceObserver* observer);
  void RemoveObserver(NtpBackgroundServiceObserver* observer);

  // Returns the currently cached CollectionInfo, if any.
  const std::vector<CollectionInfo>& collection_info() const {
    return collection_info_;
  }

  // Returns the currently cached CollectionImages, if any.
  const std::vector<CollectionImage>& collection_images() const {
    return collection_images_;
  }

  GURL GetCollectionsLoadURLForTesting() const;
  GURL GetImagesURLForTesting() const;

 private:
  GURL collections_api_url_;
  GURL collection_images_api_url_;
  std::string image_options_;

  // Used to download the proto from the Backdrop service.
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> collections_loader_;
  std::unique_ptr<network::SimpleURLLoader> image_info_loader_;

  base::ObserverList<NtpBackgroundServiceObserver, true> observers_;

  // Callback that processes the response from the FetchCollectionInfo request,
  // refreshing the contents of collection_info_ with server-provided data.
  void OnCollectionInfoFetchComplete(
      const std::unique_ptr<std::string> response_body);

  // Callback that processes the response from the FetchCollectionImages
  // request, refreshing the contents of collection_images_ with
  // server-provided data.
  void OnCollectionImageInfoFetchComplete(
      const std::unique_ptr<std::string> response_body);

  enum class FetchComplete {
    // Indicates that asynchronous fetch of CollectionInfo has completed.
    COLLECTION_INFO,
    // Indicates that asynchronous fetch of CollectionImages has completed.
    COLLECTION_IMAGE_INFO
  };
  void NotifyObservers(FetchComplete fetch_complete);

  std::vector<CollectionInfo> collection_info_;

  std::vector<CollectionImage> collection_images_;
  std::string requested_collection_id_;

  DISALLOW_COPY_AND_ASSIGN(NtpBackgroundService);
};

#endif  // CHROME_BROWSER_SEARCH_BACKGROUND_NTP_BACKGROUND_SERVICE_H_
