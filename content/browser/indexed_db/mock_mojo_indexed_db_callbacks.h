// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_MOCK_MOJO_INDEXED_DB_CALLBACKS_H_
#define CONTENT_BROWSER_INDEXED_DB_MOCK_MOJO_INDEXED_DB_CALLBACKS_H_

#include <stddef.h>
#include <string>

#include "base/macros.h"
#include "content/common/indexed_db/indexed_db.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace content {

class MockMojoIndexedDBCallbacks : public ::indexed_db::mojom::Callbacks {
 public:
  explicit MockMojoIndexedDBCallbacks();
  ~MockMojoIndexedDBCallbacks() override;

  ::indexed_db::mojom::CallbacksAssociatedPtrInfo CreateInterfacePtrAndBind();

  MOCK_METHOD2(Error, void(int32_t code, const base::string16& message));

  MOCK_METHOD1(SuccessStringList,
               void(const std::vector<base::string16>& value));

  MOCK_METHOD1(Blocked, void(int64_t existing_version));

  MOCK_METHOD5(
      MockedUpgradeNeeded,
      void(::indexed_db::mojom::DatabaseAssociatedPtrInfo* database_info,
           int64_t old_version,
           blink::WebIDBDataLoss data_loss,
           const std::string& data_loss_message,
           const content::IndexedDBDatabaseMetadata& metadata));

  // Move-only types not supported by mock methods.
  void UpgradeNeeded(
      ::indexed_db::mojom::DatabaseAssociatedPtrInfo database_info,
      int64_t old_version,
      blink::WebIDBDataLoss data_loss,
      const std::string& data_loss_message,
      const content::IndexedDBDatabaseMetadata& metadata) override {
    MockedUpgradeNeeded(&database_info, old_version, data_loss,
                        data_loss_message, metadata);
  }

  MOCK_METHOD2(
      MockedSuccessDatabase,
      void(::indexed_db::mojom::DatabaseAssociatedPtrInfo* database_info,
           const content::IndexedDBDatabaseMetadata& metadata));
  void SuccessDatabase(
      ::indexed_db::mojom::DatabaseAssociatedPtrInfo database_info,
      const content::IndexedDBDatabaseMetadata& metadata) override {
    MockedSuccessDatabase(&database_info, metadata);
  }

  MOCK_METHOD4(MockedSuccessCursor,
               void(::indexed_db::mojom::CursorAssociatedPtrInfo* cursor,
                    const IndexedDBKey& key,
                    const IndexedDBKey& primary_key,
                    ::indexed_db::mojom::ValuePtr* value));
  void SuccessCursor(::indexed_db::mojom::CursorAssociatedPtrInfo cursor,
                     const IndexedDBKey& key,
                     const IndexedDBKey& primary_key,
                     ::indexed_db::mojom::ValuePtr value) override {
    MockedSuccessCursor(&cursor, key, primary_key, &value);
  }

  MOCK_METHOD1(MockedSuccessValue,
               void(::indexed_db::mojom::ReturnValuePtr* value));
  void SuccessValue(::indexed_db::mojom::ReturnValuePtr value) override {
    MockedSuccessValue(&value);
  }

  MOCK_METHOD3(MockedSuccessCursorContinue,
               void(const IndexedDBKey& key,
                    const IndexedDBKey& primary_key,
                    ::indexed_db::mojom::ValuePtr* value));

  void SuccessCursorContinue(const IndexedDBKey& key,
                             const IndexedDBKey& primary_key,
                             ::indexed_db::mojom::ValuePtr value) override {
    MockedSuccessCursorContinue(key, primary_key, &value);
  }

  MOCK_METHOD3(MockedSuccessCursorPrefetch,
               void(const std::vector<IndexedDBKey>& keys,
                    const std::vector<IndexedDBKey>& primary_keys,
                    std::vector<::indexed_db::mojom::ValuePtr>* values));

  void SuccessCursorPrefetch(
      const std::vector<IndexedDBKey>& keys,
      const std::vector<IndexedDBKey>& primary_keys,
      std::vector<::indexed_db::mojom::ValuePtr> values) override {
    MockedSuccessCursorPrefetch(keys, primary_keys, &values);
  }

  MOCK_METHOD1(MockedSuccessArray,
               void(std::vector<::indexed_db::mojom::ReturnValuePtr>* values));
  void SuccessArray(
      std::vector<::indexed_db::mojom::ReturnValuePtr> values) override {
    MockedSuccessArray(&values);
  }

  MOCK_METHOD1(SuccessKey, void(const IndexedDBKey& key));
  MOCK_METHOD1(SuccessInteger, void(int64_t value));
  MOCK_METHOD0(Success, void());

 private:
  mojo::AssociatedBinding<::indexed_db::mojom::Callbacks> binding_;

  DISALLOW_COPY_AND_ASSIGN(MockMojoIndexedDBCallbacks);
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_MOCK_MOJO_INDEXED_DB_CALLBACKS_H_
