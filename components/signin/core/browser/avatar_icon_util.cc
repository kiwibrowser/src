// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/avatar_icon_util.h"

#include <string>

#include "base/stl_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace {

// Separator of URL path components.
const char kURLPathSeparator[] = "/";

// Constants describing image URL format.
// See https://crbug.com/733306#c3 for details.
const size_t kImageURLPathComponentsCount = 6;
const size_t kImageURLPathComponentsCountWithOptions = 7;
const size_t kImageURLPathOptionsComponentPosition = 5;
// Various options that can be embedded in image URL.
const char kImageURLOptionSeparator[] = "-";
const char kImageURLOptionSizePattern[] = R"(s\d+)";
const char kImageURLOptionSizeFormat[] = "s%d";
const char kImageURLOptionSquareCrop[] = "c";
// Option to disable default avatar if user doesn't have a custom one.
const char kImageURLOptionNoSilhouette[] = "ns";

std::string BuildImageURLOptionsString(int image_size,
                                       bool no_silhouette,
                                       const std::string& existing_options) {
  std::vector<std::string> url_options =
      base::SplitString(existing_options, kImageURLOptionSeparator,
                        base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  RE2 size_pattern(kImageURLOptionSizePattern);
  base::EraseIf(url_options, [&size_pattern](const std::string& str) {
    return RE2::FullMatch(str, size_pattern);
  });
  base::Erase(url_options, kImageURLOptionSquareCrop);
  base::Erase(url_options, kImageURLOptionNoSilhouette);

  url_options.push_back(
      base::StringPrintf(kImageURLOptionSizeFormat, image_size));
  url_options.push_back(kImageURLOptionSquareCrop);
  if (no_silhouette)
    url_options.push_back(kImageURLOptionNoSilhouette);
  return base::JoinString(url_options, kImageURLOptionSeparator);
}

}  // namespace

namespace signin {

GURL GetAvatarImageURLWithOptions(const GURL& old_url,
                                  int image_size,
                                  bool no_silhouette) {
  DCHECK(old_url.is_valid());

  std::vector<std::string> components =
      base::SplitString(old_url.path(), kURLPathSeparator,
                        base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  if (components.size() < kImageURLPathComponentsCount ||
      components.size() > kImageURLPathComponentsCountWithOptions ||
      components.back().empty()) {
    return old_url;
  }

  if (components.size() == kImageURLPathComponentsCount) {
    components.insert(
        components.begin() + kImageURLPathOptionsComponentPosition,
        BuildImageURLOptionsString(image_size, no_silhouette, std::string()));
  } else {
    DCHECK_EQ(kImageURLPathComponentsCountWithOptions, components.size());
    std::string options = components.at(kImageURLPathOptionsComponentPosition);
    components[kImageURLPathOptionsComponentPosition] =
        BuildImageURLOptionsString(image_size, no_silhouette, options);
  }

  std::string new_path = base::JoinString(components, kURLPathSeparator);
  GURL::Replacements replacement;
  replacement.SetPathStr(new_path);
  return old_url.ReplaceComponents(replacement);
}

}  // namespace signin
