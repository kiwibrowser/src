// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_ITEMS_COLLECTION_CORE_OFFLINE_ITEM_STATE_H_
#define COMPONENTS_OFFLINE_ITEMS_COLLECTION_CORE_OFFLINE_ITEM_STATE_H_

namespace offline_items_collection {

// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.components.offline_items_collection
enum OfflineItemState {
  IN_PROGRESS = 0,
  PENDING,
  COMPLETE,
  CANCELLED,
  INTERRUPTED,
  FAILED,
  PAUSED,  // TODO(dtrainor): Make sure exposing a PAUSED state does not impact
           // downloads resumption.
  MAX_DOWNLOAD_STATE,
};

}  // namespace offline_items_collection

#endif  // COMPONENTS_OFFLINE_ITEMS_COLLECTION_CORE_OFFLINE_ITEM_STATE_H_
