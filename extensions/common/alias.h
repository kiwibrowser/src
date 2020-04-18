// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_ALIAS_H_
#define EXTENSIONS_COMMON_ALIAS_H_

#include <string>

namespace extensions {

// Information about an alias.
// Main usage: describing aliases for extension features (extension permissions,
// APIs), which is useful for ensuring backward-compatibility of extension
// features when they get renamed. Old feature name can be defined as an alias
// for the new feature name - this would ensure that the extensions using the
// old feature name don't break.
class Alias {
 public:
  // |name|: The alias name.
  // |real_name|: The real name behind alias.
  Alias(const char* const name, const char* const real_name)
      : name_(name), real_name_(real_name) {}
  ~Alias() {}

  const std::string& name() const { return name_; }

  const std::string& real_name() const { return real_name_; }

 private:
  // The alias name.
  std::string name_;

  // The real name behind the alias.
  std::string real_name_;
};

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_ALIAS_H_
