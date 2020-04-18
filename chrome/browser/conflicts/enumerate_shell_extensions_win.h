// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFLICTS_ENUMERATE_SHELL_EXTENSIONS_WIN_H_
#define CHROME_BROWSER_CONFLICTS_ENUMERATE_SHELL_EXTENSIONS_WIN_H_

#include <stdint.h>

#include "base/callback_forward.h"

namespace base {
class FilePath;
}

// The path to the registry key where shell extensions are registered.
extern const wchar_t kShellExtensionRegistryKey[];

// Enumerates registered shell extensions, and invokes |callback| once per shell
// extension found. Must be called on a blocking sequence.
// TODO(pmonette): Move this implementation detail to the .cc file when
//                 enumerate_modules_model.cc gets deleted.
void EnumerateShellExtensionPaths(
    const base::RepeatingCallback<void(const base::FilePath&)>& callback);

// Finds shell extensions installed on the computer by enumerating the registry.
// In addition to the file path, the SizeOfImage and TimeDateStamp of the module
// is returned via the |on_shell_extension_enumerated| callback.
using OnShellExtensionEnumeratedCallback =
    base::RepeatingCallback<void(const base::FilePath&, uint32_t, uint32_t)>;
void EnumerateShellExtensions(
    OnShellExtensionEnumeratedCallback on_shell_extension_enumerated,
    base::OnceClosure on_enumeration_finished);

#endif  // CHROME_BROWSER_CONFLICTS_ENUMERATE_SHELL_EXTENSIONS_WIN_H_
