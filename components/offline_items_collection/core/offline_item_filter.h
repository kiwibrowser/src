// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_ITEMS_COLLECTION_CORE_OFFLINE_ITEM_FILTER_H_
#define COMPONENTS_OFFLINE_ITEMS_COLLECTION_CORE_OFFLINE_ITEM_FILTER_H_

namespace offline_items_collection {

// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.components.offline_items_collection
enum OfflineItemFilter {
  FILTER_ALL = 0,
  FILTER_PAGE,
  FILTER_VIDEO,
  FILTER_AUDIO,
  FILTER_IMAGE,
  FILTER_DOCUMENT,
  FILTER_OTHER,

  // Maximum value.
  FILTER_BOUNDARY,
};

}  // namespace offline_items_collection

#endif  // COMPONENTS_OFFLINE_ITEMS_COLLECTION_CORE_OFFLINE_ITEM_FILTER_H_
