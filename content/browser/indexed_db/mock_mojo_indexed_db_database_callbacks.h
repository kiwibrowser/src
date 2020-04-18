// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_MOCK_MOJO_INDEXED_DB_DATABASE_CALLBACKS_H_
#define CONTENT_BROWSER_INDEXED_DB_MOCK_MOJO_INDEXED_DB_DATABASE_CALLBACKS_H_

#include <stdint.h>

#include "base/macros.h"
#include "content/common/indexed_db/indexed_db.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace content {

class MockMojoIndexedDBDatabaseCallbacks
    : public ::indexed_db::mojom::DatabaseCallbacks {
 public:
  MockMojoIndexedDBDatabaseCallbacks();
  ~MockMojoIndexedDBDatabaseCallbacks() override;

  ::indexed_db::mojom::DatabaseCallbacksAssociatedPtrInfo
  CreateInterfacePtrAndBind();

  MOCK_METHOD0(ForcedClose, void());
  MOCK_METHOD2(VersionChange, void(int64_t old_version, int64_t new_version));
  MOCK_METHOD3(Abort,
               void(int64_t transaction_id,
                    int32_t code,
                    const base::string16& message));
  MOCK_METHOD1(Complete, void(int64_t transaction_id));

  MOCK_METHOD1(MockedChanges,
               void(::indexed_db::mojom::ObserverChangesPtr* changes));
  void Changes(::indexed_db::mojom::ObserverChangesPtr changes) override {
    MockedChanges(&changes);
  }

 private:
  mojo::AssociatedBinding<::indexed_db::mojom::DatabaseCallbacks> binding_;

  DISALLOW_COPY_AND_ASSIGN(MockMojoIndexedDBDatabaseCallbacks);
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_MOCK_MOJO_INDEXED_DB_DATABASE_CALLBACKS_H_
