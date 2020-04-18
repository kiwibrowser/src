// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TEST_FONTCONFIG_UTIL_LINUX_H_
#define BASE_TEST_FONTCONFIG_UTIL_LINUX_H_

#include <stddef.h>

#include <string>

namespace base {
class FilePath;

// Initializes Fontconfig with a custom configuration suitable for tests.
void SetUpFontconfig();

// Deinitializes Fontconfig.
void TearDownFontconfig();

// Instructs Fontconfig to load |path|, an XML configuration file, into the
// current config, returning true on success.
bool LoadConfigFileIntoFontconfig(const FilePath& path);

// Writes |data| to a file in |temp_dir| and passes it to
// LoadConfigFileIntoFontconfig().
bool LoadConfigDataIntoFontconfig(const FilePath& temp_dir,
                                  const std::string& data);

// Returns a Fontconfig <edit> stanza.
std::string CreateFontconfigEditStanza(const std::string& name,
                                       const std::string& type,
                                       const std::string& value);

// Returns a Fontconfig <test> stanza.
std::string CreateFontconfigTestStanza(const std::string& name,
                                       const std::string& op,
                                       const std::string& type,
                                       const std::string& value);

// Returns a Fontconfig <alias> stanza.
std::string CreateFontconfigAliasStanza(const std::string& original_family,
                                        const std::string& preferred_family);

}  // namespace base

#endif  // BASE_TEST_FONTCONFIG_UTIL_LINUX_H_
