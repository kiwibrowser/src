// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/features/feature.h"

#include <map>

#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest.h"

namespace extensions {

// static
Feature::Platform Feature::GetCurrentPlatform() {
#if defined(OS_CHROMEOS)
  return CHROMEOS_PLATFORM;
#elif defined(OS_LINUX)
  return LINUX_PLATFORM;
#elif defined(OS_MACOSX)
  return MACOSX_PLATFORM;
#elif defined(OS_WIN)
  return WIN_PLATFORM;
#else
  return UNSPECIFIED_PLATFORM;
#endif
}

Feature::Availability Feature::IsAvailableToExtension(
    const Extension* extension) const {
  return IsAvailableToManifest(extension->hashed_id(), extension->GetType(),
                               extension->location(),
                               extension->manifest_version());
}

Feature::Feature() : no_parent_(false) {}

Feature::~Feature() {}

void Feature::set_name(base::StringPiece name) {
  name_ = name.as_string();
}

void Feature::set_alias(base::StringPiece alias) {
  alias_ = alias.as_string();
}

void Feature::set_source(base::StringPiece source) {
  source_ = source.as_string();
}

}  // namespace extensions
