// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_BACKDROP_WALLPAPER_HANDLERS_BACKDROP_WALLPAPER_HANDLERS_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_BACKDROP_WALLPAPER_HANDLERS_BACKDROP_WALLPAPER_HANDLERS_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace extensions {
namespace api {
namespace wallpaper_private {
struct CollectionInfo;
struct ImageInfo;
}  // namespace wallpaper_private
}  // namespace api
}  // namespace extensions

namespace backdrop_wallpaper_handlers {

// Downloads and deserializes the proto for the wallpaper collections info from
// the Backdrop service.
class CollectionInfoFetcher {
 public:
  using OnCollectionsInfoFetched = base::OnceCallback<void(
      bool success,
      const std::vector<extensions::api::wallpaper_private::CollectionInfo>&
          collections_info_list)>;

  CollectionInfoFetcher();
  ~CollectionInfoFetcher();

  // Triggers the start of the downloading and deserializing of the proto.
  void Start(OnCollectionsInfoFetched callback);

  void OnURLFetchComplete(const std::unique_ptr<std::string> response_body);

 private:
  // Used to download the proto from the Backdrop service.
  std::unique_ptr<network::SimpleURLLoader> simple_loader_;

  // The callback upon completion of fetching the collections info.
  OnCollectionsInfoFetched callback_;

  DISALLOW_COPY_AND_ASSIGN(CollectionInfoFetcher);
};

// Downloads and deserializes the proto for the wallpaper images info from the
// Backdrop service.
class ImageInfoFetcher {
 public:
  using OnImagesInfoFetched = base::OnceCallback<void(
      bool success,
      const std::vector<extensions::api::wallpaper_private::ImageInfo>&
          images_info_list)>;

  explicit ImageInfoFetcher(const std::string& collection_id);
  ~ImageInfoFetcher();

  // Triggers the start of the downloading and deserializing of the proto.
  void Start(OnImagesInfoFetched callback);

  void OnURLFetchComplete(std::unique_ptr<std::string> response_body);

 private:
  // Used to download the proto from the Backdrop service.
  std::unique_ptr<network::SimpleURLLoader> simple_loader_;

  // The id of the collection, used as the token to fetch the images info.
  const std::string collection_id_;

  // The callback upon completion of fetching the images info.
  OnImagesInfoFetched callback_;

  DISALLOW_COPY_AND_ASSIGN(ImageInfoFetcher);
};
}  // namespace backdrop_wallpaper_handlers

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_BACKDROP_WALLPAPER_HANDLERS_BACKDROP_WALLPAPER_HANDLERS_H_
