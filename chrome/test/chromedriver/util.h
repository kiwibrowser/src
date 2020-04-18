// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_UTIL_H_
#define CHROME_TEST_CHROMEDRIVER_UTIL_H_

#include <string>

namespace base {
class FilePath;
class ListValue;
}

struct Session;
class Status;
class WebView;

// Generates a random, 32-character hexidecimal ID.
std::string GenerateId();

// Send a sequence of key strokes to the active Element in window.
Status SendKeysOnWindow(
    WebView* web_view,
    const base::ListValue* key_list,
    bool release_modifiers,
    int* sticky_modifiers);

// Decodes the given base64-encoded string, after removing any newlines,
// which are required in some base64 standards. Returns true on success.
bool Base64Decode(const std::string& base64, std::string* bytes);

// Unzips the sole file contained in the given zip data |bytes| into
// |unzip_dir|. The zip data may be a normal zip archive or a single zip file
// entry. If the unzip successfully produced one file, returns true and sets
// |file| to the unzipped file.
// TODO(kkania): Remove the ability to parse single zip file entries when
// the current versions of all WebDriver clients send actual zip files.
Status UnzipSoleFile(const base::FilePath& unzip_dir,
                     const std::string& bytes,
                     base::FilePath* file);

// Calls BeforeCommand for each of |session|'s |CommandListener|s.
// If an error is encountered, will mark |session| for deletion and return.
Status NotifyCommandListenersBeforeCommand(Session* session,
                                           const std::string& command_name);

#endif  // CHROME_TEST_CHROMEDRIVER_UTIL_H_
