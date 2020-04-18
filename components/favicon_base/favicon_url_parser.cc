// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/favicon_base/favicon_url_parser.h"

#include "base/strings/string_number_conversions.h"
#include "components/favicon_base/favicon_types.h"
#include "net/url_request/url_request.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/gfx/favicon_size.h"

namespace {

// Parameters which can be used in chrome://favicon path. See file
// "chrome/browser/ui/webui/favicon_source.h" for a description of
// what each does.
const char kIconURLParameter[] = "iconurl/";
const char kSizeParameter[] = "size/";

// Returns true if |search| is a substring of |path| which starts at
// |start_index|.
bool HasSubstringAt(const std::string& path,
                    size_t start_index,
                    const std::string& search) {
  return path.compare(start_index, search.length(), search) == 0;
}

}  // namespace

namespace chrome {

bool ParseFaviconPath(const std::string& path,
                      ParsedFaviconPath* parsed) {
  parsed->is_icon_url = false;
  parsed->url = "";
  parsed->size_in_dip = gfx::kFaviconSize;
  parsed->device_scale_factor = 1.0f;
  parsed->path_index = std::string::npos;

  if (path.empty())
    return false;

  size_t parsed_index = 0;
  if (HasSubstringAt(path, parsed_index, kSizeParameter)) {
    parsed_index += strlen(kSizeParameter);

    size_t slash = path.find("/", parsed_index);
    if (slash == std::string::npos)
      return false;

    size_t scale_delimiter = path.find("@", parsed_index);
    std::string size_str;
    std::string scale_str;
    if (scale_delimiter == std::string::npos) {
      // Support the legacy size format of 'size/aa/' where 'aa' is the desired
      // size in DIP for the sake of not regressing the extensions which use it.
      size_str = path.substr(parsed_index, slash - parsed_index);
    } else {
      size_str = path.substr(parsed_index, scale_delimiter - parsed_index);
      scale_str = path.substr(scale_delimiter + 1,
                              slash - scale_delimiter - 1);
    }

    if (!base::StringToInt(size_str, &parsed->size_in_dip))
      return false;

    if (!scale_str.empty())
      webui::ParseScaleFactor(scale_str, &parsed->device_scale_factor);

    parsed_index = slash + 1;
  }

  if (HasSubstringAt(path, parsed_index, kIconURLParameter)) {
    parsed_index += strlen(kIconURLParameter);
    parsed->is_icon_url = true;
    parsed->url = path.substr(parsed_index);
  } else {
    parsed->url = path.substr(parsed_index);
  }

  // The parsed index needs to be returned in order to allow Instant Extended
  // to translate favicon URLs using advanced parameters.
  // Example:
  //   "chrome-search://favicon/size/16@2x/<renderer-id>/<most-visited-id>"
  // would be translated to:
  //   "chrome-search://favicon/size/16@2x/<most-visited-item-with-given-id>".
  parsed->path_index = parsed_index;
  return true;
}

}  // namespace chrome
