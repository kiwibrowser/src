// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_CATALOG_STORE_H_
#define SERVICES_CATALOG_STORE_H_

namespace catalog {

// TODO(rockot): Clean this up now that it's only a namespace for constants.
// Alternatively, re-introduce a Store interface once it makes sense to do so.
class Store {
 public:
  // Value is a string.
  static const char kNameKey[];
  // Value is a string.
  static const char kDisplayNameKey[];
  // Value is a string.
  static const char kSandboxTypeKey[];
  // Value is a dictionary.
  static const char kInterfaceProviderSpecsKey[];
  // Value is a dictionary.
  static const char kInterfaceProviderSpecs_ProvidesKey[];
  // Value is a dictionary.
  static const char kInterfaceProviderSpecs_RequiresKey[];
  // Value is a list.
  static const char kServicesKey[];
  // Value is a dictionary.
  static const char kRequiredFilesKey[];
  // Value is a string.
  static const char kRequiredFilesKey_PathKey[];
  // Value is a string.
  static const char kRequiredFilesKey_PlatformKey[];
  static const char kRequiredFilesKey_PlatformValue_Windows[];
  static const char kRequiredFilesKey_PlatformValue_Linux[];
  static const char kRequiredFilesKey_PlatformValue_MacOSX[];
  static const char kRequiredFilesKey_PlatformValue_Android[];
  static const char kRequiredFilesKey_PlatformValue_Fuchsia[];
};

}  // namespace catalog

#endif  // SERVICES_CATALOG_STORE_H_
