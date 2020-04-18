// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE_DOWNLOAD_TASK_TYPES_H_
#define COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE_DOWNLOAD_TASK_TYPES_H_

namespace download {

// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.components.download
enum class DownloadTaskType {
  // Task to invoke download service to take various download actions.
  DOWNLOAD_TASK = 0,

  // Task to remove unnecessary files from the system.
  CLEANUP_TASK = 1,
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE_DOWNLOAD_TASK_TYPES_H_
