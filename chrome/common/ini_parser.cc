// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/ini_parser.h"

#include <stddef.h>

#include "base/logging.h"
#include "base/strings/string_tokenizer.h"

INIParser::INIParser() : used_(false) {}

INIParser::~INIParser() {}

void INIParser::Parse(const std::string& content) {
  DCHECK(!used_);
  used_ = true;
  base::StringTokenizer tokenizer(content, "\r\n");

  std::string current_section;
  while (tokenizer.GetNext()) {
    std::string line = tokenizer.token();
    if (line.empty()) {
      // Skips the empty line.
      continue;
    }
    if (line[0] == '#' || line[0] == ';') {
      // This line is a comment.
      continue;
    }
    if (line[0] == '[') {
      // It is a section header.
      current_section = line.substr(1);
      size_t end = current_section.rfind(']');
      if (end != std::string::npos)
        current_section.erase(end);
    } else {
      std::string key, value;
      size_t equal = line.find('=');
      if (equal != std::string::npos) {
        key = line.substr(0, equal);
        value = line.substr(equal + 1);
        HandleTriplet(current_section, key, value);
      }
    }
  }
}

DictionaryValueINIParser::DictionaryValueINIParser() {}

DictionaryValueINIParser::~DictionaryValueINIParser() {}

void DictionaryValueINIParser::HandleTriplet(const std::string& section,
                                             const std::string& key,
                                             const std::string& value) {

  // Checks whether the section and key contain a '.' character.
  // Those sections and keys break DictionaryValue's path format when not
  // using the *WithoutPathExpansion methods.
  if (section.find('.') == std::string::npos &&
      key.find('.') == std::string::npos)
    root_.SetString(section + "." + key, value);
}
