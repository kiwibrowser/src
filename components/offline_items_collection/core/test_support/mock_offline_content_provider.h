// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_ITEMS_COLLECTION_CORE_TEST_SUPPORT_MOCK_OFFLINE_CONTENT_PROVIDER_H_
#define COMPONENTS_OFFLINE_ITEMS_COLLECTION_CORE_TEST_SUPPORT_MOCK_OFFLINE_CONTENT_PROVIDER_H_

#include "base/observer_list.h"
#include "components/offline_items_collection/core/offline_content_provider.h"
#include "components/offline_items_collection/core/offline_item.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_items_collection {

class MockOfflineContentProvider : public OfflineContentProvider {
 public:
  class MockObserver : public OfflineContentProvider::Observer {
   public:
    MockObserver();
    ~MockObserver() override;

    // OfflineContentProvider::Observer implementation.
    MOCK_METHOD1(OnItemsAdded, void(const OfflineItemList&));
    MOCK_METHOD1(OnItemRemoved, void(const ContentId&));
    MOCK_METHOD1(OnItemUpdated, void(const OfflineItem&));
  };

  MockOfflineContentProvider();
  ~MockOfflineContentProvider() override;

  bool HasObserver(Observer* observer);
  void SetItems(const OfflineItemList& items);
  void NotifyOnItemsAdded(const OfflineItemList& items);
  void NotifyOnItemRemoved(const ContentId& id);
  void NotifyOnItemUpdated(const OfflineItem& item);

  // OfflineContentProvider implementation.
  MOCK_METHOD1(OpenItem, void(const ContentId&));
  MOCK_METHOD1(RemoveItem, void(const ContentId&));
  MOCK_METHOD1(CancelDownload, void(const ContentId&));
  MOCK_METHOD1(PauseDownload, void(const ContentId&));
  MOCK_METHOD2(ResumeDownload, void(const ContentId&, bool));
  MOCK_METHOD2(GetVisualsForItem,
               void(const ContentId&, const VisualsCallback&));
  void GetAllItems(MultipleItemCallback callback) override;
  void GetItemById(const ContentId& id, SingleItemCallback callback) override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

 private:
  base::ObserverList<Observer> observers_;
  OfflineItemList items_;
};

}  // namespace offline_items_collection

#endif  // COMPONENTS_OFFLINE_ITEMS_COLLECTION_CORE_TEST_SUPPORT_MOCK_OFFLINE_CONTENT_PROVIDER_H_
