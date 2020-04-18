// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_ITEMS_COLLECTION_CORE_FAIL_STATE_H_
#define COMPONENTS_OFFLINE_ITEMS_COLLECTION_CORE_FAIL_STATE_H_

namespace offline_items_collection {

// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.components.offline_items_collection
enum class FailState {
  // Enum for reason OfflineItem failed to download.
  NO_FAILURE,           // Download did not fail.
  CANNOT_DOWNLOAD,      // Download cannot be downloaded.
  NETWORK_INSTABILITY,  // Download failed due to poor or unstable network
                        // connection.
};

}  // namespace offline_items_collection

#endif  // COMPONENTS_OFFLINE_ITEMS_COLLECTION_CORE_FAIL_STATE_H_
