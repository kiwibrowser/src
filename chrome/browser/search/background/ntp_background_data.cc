// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/background/ntp_background_data.h"

CollectionInfo::CollectionInfo() = default;
CollectionInfo::CollectionInfo(const CollectionInfo&) = default;
CollectionInfo::CollectionInfo(CollectionInfo&&) = default;
CollectionInfo::~CollectionInfo() = default;

CollectionInfo& CollectionInfo::operator=(const CollectionInfo&) = default;
CollectionInfo& CollectionInfo::operator=(CollectionInfo&&) = default;

bool operator==(const CollectionInfo& lhs, const CollectionInfo& rhs) {
  return lhs.collection_id == rhs.collection_id &&
         lhs.collection_name == rhs.collection_name &&
         lhs.preview_image_url == rhs.preview_image_url;
}

bool operator!=(const CollectionInfo& lhs, const CollectionInfo& rhs) {
  return !(lhs == rhs);
}

CollectionInfo CollectionInfo::CreateFromProto(
    const ntp::background::Collection& collection) {
  CollectionInfo collection_info;
  collection_info.collection_id = collection.collection_id();
  collection_info.collection_name = collection.collection_name();
  // Use the first preview image as the representative one for the collection.
  if (collection.preview_size() > 0 && collection.preview(0).has_image_url()) {
    collection_info.preview_image_url = GURL(collection.preview(0).image_url());
  }

  return collection_info;
}

CollectionImage::CollectionImage() = default;
CollectionImage::CollectionImage(const CollectionImage&) = default;
CollectionImage::CollectionImage(CollectionImage&&) = default;
CollectionImage::~CollectionImage() = default;

CollectionImage& CollectionImage::operator=(const CollectionImage&) = default;
CollectionImage& CollectionImage::operator=(CollectionImage&&) = default;

bool operator==(const CollectionImage& lhs, const CollectionImage& rhs) {
  return lhs.collection_id == rhs.collection_id &&
         lhs.asset_id == rhs.asset_id &&
         lhs.thumbnail_image_url == rhs.thumbnail_image_url &&
         lhs.image_url == rhs.image_url && lhs.attribution == rhs.attribution;
}

bool operator!=(const CollectionImage& lhs, const CollectionImage& rhs) {
  return !(lhs == rhs);
}

CollectionImage CollectionImage::CreateFromProto(
    const std::string& collection_id,
    const ntp::background::Image& image,
    const std::string& default_image_options) {
  CollectionImage collection_image;
  collection_image.collection_id = collection_id;
  collection_image.asset_id = image.asset_id();
  // Without options added to the image, it is 512x512.
  collection_image.thumbnail_image_url = GURL(image.image_url());
  // TODO(ramyan): Request resolution from service, instead of setting it here.
  collection_image.image_url = GURL(
      image.image_url() + ((image.image_url().find('=') == std::string::npos)
                               ? default_image_options
                               : std::string("")));
  for (const auto& attribution : image.attribution()) {
    collection_image.attribution.push_back(attribution.text());
  }

  return collection_image;
}
