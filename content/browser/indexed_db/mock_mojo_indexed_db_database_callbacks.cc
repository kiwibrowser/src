// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/mock_mojo_indexed_db_database_callbacks.h"

namespace content {

MockMojoIndexedDBDatabaseCallbacks::MockMojoIndexedDBDatabaseCallbacks()
    : binding_(this) {}
MockMojoIndexedDBDatabaseCallbacks::~MockMojoIndexedDBDatabaseCallbacks() {}

::indexed_db::mojom::DatabaseCallbacksAssociatedPtrInfo
MockMojoIndexedDBDatabaseCallbacks::CreateInterfacePtrAndBind() {
  ::indexed_db::mojom::DatabaseCallbacksAssociatedPtrInfo ptr_info;
  binding_.Bind(::mojo::MakeRequest(&ptr_info));
  return ptr_info;
}

}  // namespace content
