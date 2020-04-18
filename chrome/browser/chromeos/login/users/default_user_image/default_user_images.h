// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_USERS_DEFAULT_USER_IMAGE_DEFAULT_USER_IMAGES_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_USERS_DEFAULT_USER_IMAGE_DEFAULT_USER_IMAGES_H_

#include <stddef.h>

#include <memory>
#include <string>

#include "base/strings/string16.h"
#include "chromeos/chromeos_export.h"

namespace base {
class ListValue;
}

namespace gfx {
class ImageSkia;
}

namespace chromeos {
namespace default_user_image {

// Returns the URL to a default user image with the specified index. If the
// index is invalid, returns the default user image for index 0 (anonymous
// avatar image).
CHROMEOS_EXPORT std::string GetDefaultImageUrl(int index);

// Checks if the given URL points to one of the default images. If it is,
// returns true and its index through |image_id|. If not, returns false.
CHROMEOS_EXPORT bool IsDefaultImageUrl(const std::string& url, int* image_id);

// Returns bitmap of default user image with specified index.
CHROMEOS_EXPORT const gfx::ImageSkia& GetDefaultImage(int index);

// Resource IDs of default user images.
CHROMEOS_EXPORT extern const int kDefaultImageResourceIDs[];

// Number of default images.
CHROMEOS_EXPORT extern const int kDefaultImagesCount;

// The starting index of default images available for selection. Note that
// existing users may have images with smaller indices.
CHROMEOS_EXPORT extern const int kFirstDefaultImageIndex;

/// Histogram values. ////////////////////////////////////////////////////////

// Histogram value for user image taken from file.
CHROMEOS_EXPORT extern const int kHistogramImageFromFile;

// Histogram value for user image taken from camera.
CHROMEOS_EXPORT extern const int kHistogramImageFromCamera;

// Histogram value a previously used image from camera/file.
CHROMEOS_EXPORT extern const int kHistogramImageOld;

// Histogram value for user image from G+ profile.
CHROMEOS_EXPORT extern const int kHistogramImageFromProfile;

// Number of possible histogram values for user images.
CHROMEOS_EXPORT extern const int kHistogramImagesCount;

// Returns the histogram value corresponding to the given default image index.
CHROMEOS_EXPORT int GetDefaultImageHistogramValue(int index);

// Returns a random default image index.
CHROMEOS_EXPORT int GetRandomDefaultImageIndex();

// Returns true if |index| is a valid default image index.
CHROMEOS_EXPORT bool IsValidIndex(int index);

// Returns true if |index| is a in the current set of default images.
CHROMEOS_EXPORT bool IsInCurrentImageSet(int index);

// Returns a list of dictionary values with url, author, website, and title
// properties set for each default user image. If |all| is true then returns
// the complete list of default images, otherwise only returns the current list.
CHROMEOS_EXPORT std::unique_ptr<base::ListValue> GetAsDictionary(bool all);

// Returns the index of the first default image to make available for selection
// from GetAsDictionary when |all| is true. The last image to make available is
// always the last image in the Dictionary.
CHROMEOS_EXPORT int GetFirstDefaultImage();

}  // namespace default_user_image
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_USERS_DEFAULT_USER_IMAGE_DEFAULT_USER_IMAGES_H_
