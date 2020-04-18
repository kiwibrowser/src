// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_BACKGROUND_NTP_BACKGROUND_DATA_H_
#define CHROME_BROWSER_SEARCH_BACKGROUND_NTP_BACKGROUND_DATA_H_

#include <string>

#include "chrome/browser/search/background/ntp_background.pb.h"
#include "url/gurl.h"

// Background images are organized into collections, according to a theme. This
// struct contains the data required to display information about a collection,
// including a representative image. The complete set of CollectionImages must
// be requested separately, by referencing the identifier for this collection.
struct CollectionInfo {
  CollectionInfo();
  CollectionInfo(const CollectionInfo&);
  CollectionInfo(CollectionInfo&&);
  ~CollectionInfo();

  CollectionInfo& operator=(const CollectionInfo&);
  CollectionInfo& operator=(CollectionInfo&&);

  static CollectionInfo CreateFromProto(
      const ntp::background::Collection& collection);

  // A unique identifier for the collection.
  std::string collection_id;
  // A human-readable name for the collection.
  std::string collection_name;
  // A representative image from the collection.
  GURL preview_image_url;
};

bool operator==(const CollectionInfo& lhs, const CollectionInfo& rhs);
bool operator!=(const CollectionInfo& lhs, const CollectionInfo& rhs);

// Represents an image within a collection. The associated collection_id may be
// used to get CollectionInfo.
struct CollectionImage {
  CollectionImage();
  CollectionImage(const CollectionImage&);
  CollectionImage(CollectionImage&&);
  ~CollectionImage();

  CollectionImage& operator=(const CollectionImage&);
  CollectionImage& operator=(CollectionImage&&);

  // default_image_options are applied to the image.image_url() if options
  // (specifying resolution, cropping, etc) are not already present.
  static CollectionImage CreateFromProto(
      const std::string& collection_id,
      const ntp::background::Image& image,
      const std::string& default_image_options);

  // A unique identifier for the collection the image is in.
  std::string collection_id;
  // A unique identifier for the image.
  uint64_t asset_id;
  // The thumbnail image URL, typically lower resolution than the image_url.
  GURL thumbnail_image_url;
  // The image URL.
  GURL image_url;
  // The attribution list for the image.
  std::vector<std::string> attribution;
};

bool operator==(const CollectionImage& lhs, const CollectionImage& rhs);
bool operator!=(const CollectionImage& lhs, const CollectionImage& rhs);

#endif  // CHROME_BROWSER_SEARCH_BACKGROUND_NTP_BACKGROUND_DATA_H_
