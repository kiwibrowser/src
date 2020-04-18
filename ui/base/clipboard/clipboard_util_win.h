// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Some helper functions for working with the clipboard and IDataObjects.

#ifndef UI_BASE_CLIPBOARD_CLIPBOARD_UTIL_WIN_H_
#define UI_BASE_CLIPBOARD_CLIPBOARD_UTIL_WIN_H_

#include <shlobj.h>
#include <stddef.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/strings/string16.h"
#include "ui/base/ui_base_export.h"

class GURL;

namespace ui {

class UI_BASE_EXPORT ClipboardUtil {
 public:
  /////////////////////////////////////////////////////////////////////////////
  // These methods check to see if |data_object| has the requested type.
  // Returns true if it does.
  static bool HasUrl(IDataObject* data_object, bool convert_filenames);
  static bool HasFilenames(IDataObject* data_object);
  static bool HasPlainText(IDataObject* data_object);
  static bool HasFileContents(IDataObject* data_object);
  static bool HasHtml(IDataObject* data_object);

  /////////////////////////////////////////////////////////////////////////////
  // Helper methods to extract information from an IDataObject.  These methods
  // return true if the requested data type is found in |data_object|.

  // Only returns true if url->is_valid() is true.
  static bool GetUrl(IDataObject* data_object,
                     GURL* url,
                     base::string16* title,
                     bool convert_filenames);
  static bool GetFilenames(IDataObject* data_object,
                           std::vector<base::string16>* filenames);
  static bool GetPlainText(IDataObject* data_object,
                           base::string16* plain_text);
  static bool GetHtml(IDataObject* data_object,
                      base::string16* text_html,
                      std::string* base_url);
  static bool GetFileContents(IDataObject* data_object,
                              base::string16* filename,
                              std::string* file_contents);
  // This represents custom MIME types a web page might set to transport its
  // own types of data for drag and drop. It is sandboxed in its own CLIPFORMAT
  // to avoid polluting the ::RegisterClipboardFormat() namespace with random
  // strings from web content.
  static bool GetWebCustomData(
      IDataObject* data_object,
      std::unordered_map<base::string16, base::string16>* custom_data);

  // Helper method for converting between MS CF_HTML format and plain
  // text/html.
  static std::string HtmlToCFHtml(const std::string& html,
                                  const std::string& base_url);
  static void CFHtmlToHtml(const std::string& cf_html,
                           std::string* html,
                           std::string* base_url);
  static void CFHtmlExtractMetadata(const std::string& cf_html,
                                    std::string* base_url,
                                    size_t* html_start,
                                    size_t* fragment_start,
                                    size_t* fragment_end);
};

}

#endif  // UI_BASE_CLIPBOARD_CLIPBOARD_UTIL_WIN_H_
