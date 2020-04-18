// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/catalog/store.h"

namespace catalog {

// static
const char Store::kNameKey[] = "name";
// static
const char Store::kDisplayNameKey[] = "display_name";
// static
const char Store::kSandboxTypeKey[] = "sandbox_type";
// static
const char Store::kInterfaceProviderSpecsKey[] = "interface_provider_specs";
// static
const char Store::kInterfaceProviderSpecs_ProvidesKey[] = "provides";
// static
const char Store::kInterfaceProviderSpecs_RequiresKey[] = "requires";
// static
const char Store::kServicesKey[] = "services";
// static
const char Store::kRequiredFilesKey[] = "required_files";
// static
const char Store::kRequiredFilesKey_PathKey[] = "path";
// static
const char Store::kRequiredFilesKey_PlatformKey[] = "platform";
// static
const char Store::kRequiredFilesKey_PlatformValue_Windows[] = "windows";
// static
const char Store::kRequiredFilesKey_PlatformValue_Linux[] = "linux";
// static
const char Store::kRequiredFilesKey_PlatformValue_MacOSX[] = "macosx";
// static
const char Store::kRequiredFilesKey_PlatformValue_Android[] = "android";
// static
const char Store::kRequiredFilesKey_PlatformValue_Fuchsia[] = "fuchsia";

}  // namespace catalog
