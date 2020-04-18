// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_TOOLS_INDEXING_TOOL_H_
#define COMPONENTS_SUBRESOURCE_FILTER_TOOLS_INDEXING_TOOL_H_

#include "base/command_line.h"
#include "base/files/file_path.h"

namespace subresource_filter {

// Given |unindexed_path|, which is a path to an unindexed ruleset, writes the
// indexed (flatbuffer) version to |indexed_path|. Returns false if there was
// something wrong with the given paths.
bool IndexAndWriteRuleset(const base::FilePath& unindexed_path,
                          const base::FilePath& indexed_path);

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_TOOLS_INDEXING_TOOL_H_
