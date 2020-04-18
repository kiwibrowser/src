// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FAVICON_BASE_FAVICON_URL_PARSER_H_
#define COMPONENTS_FAVICON_BASE_FAVICON_URL_PARSER_H_

#include <stddef.h>

#include <string>

namespace chrome {

struct ParsedFaviconPath {
  // Whether the URL has the "iconurl" parameter.
  bool is_icon_url;

  // The URL from which the favicon is being requested.
  std::string url;

  // The size of the requested favicon in dip.
  int size_in_dip;

  // The device scale factor of the requested favicon.
  float device_scale_factor;

  // The index of the first character (relative to the path) where the the URL
  // from which the favicon is being requested is located.
  size_t path_index;
};

// Parses |path|, which should be in the format described at the top of the
// file "chrome/browser/ui/webui/favicon_source.h". Returns true if |path| could
// be parsed. The result of the parsing will be stored in a ParsedFaviconPath
// struct.
bool ParseFaviconPath(const std::string& path, ParsedFaviconPath* parsed);

}  // namespace chrome

#endif  // COMPONENTS_FAVICON_BASE_FAVICON_URL_PARSER_H_
