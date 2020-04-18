// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INSTALL_VERIFICATION_WIN_MODULE_IDS_H_
#define CHROME_BROWSER_INSTALL_VERIFICATION_WIN_MODULE_IDS_H_

#include <stddef.h>

#include <map>
#include <string>

#include "base/strings/string_piece.h"

typedef std::map<std::string, size_t> ModuleIDs;

// Parses a list of additional modules to verify. The data format is a series of
// lines. Each line starts with a decimal ID, then a module name digest,
// separated by a space. Lines are terminated by \r and/or \n. Invalid lines are
// ignored.
//
// The result is a map of module name digests to module IDs.
void ParseAdditionalModuleIDs(
    const base::StringPiece& raw_data,
    ModuleIDs* module_ids);

// Loads standard module IDs and additional module IDs from a resource.
void LoadModuleIDs(ModuleIDs* module_ids);

#endif  // CHROME_BROWSER_INSTALL_VERIFICATION_WIN_MODULE_IDS_H_
