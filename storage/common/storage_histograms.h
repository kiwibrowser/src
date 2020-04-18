// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_COMMON_HISTOGRAMS_H
#define STORAGE_COMMON_HISTOGRAMS_H

#include <string>
#include "storage/common/storage_common_export.h"

namespace storage {

STORAGE_COMMON_EXPORT void RecordBytesWritten(const char* label, int bytes);
STORAGE_COMMON_EXPORT void RecordBytesRead(const char* label, int bytes);

}  // namespace storage

#endif  // STORAGE_COMMON_HISTOGRAMS_H
