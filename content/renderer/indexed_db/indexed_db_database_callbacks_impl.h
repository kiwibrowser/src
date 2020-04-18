// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INDEXED_DB_INDEXED_DB_DATABASE_CALLBACKS_IMPL_H_
#define CONTENT_RENDERER_INDEXED_DB_INDEXED_DB_DATABASE_CALLBACKS_IMPL_H_

#include "base/single_thread_task_runner.h"
#include "content/common/indexed_db/indexed_db.mojom.h"

namespace blink {
class WebIDBDatabaseCallbacks;
}

namespace content {

class IndexedDBDatabaseCallbacksImpl
    : public indexed_db::mojom::DatabaseCallbacks {
 public:
  explicit IndexedDBDatabaseCallbacksImpl(
      std::unique_ptr<blink::WebIDBDatabaseCallbacks> callbacks,
      scoped_refptr<base::SingleThreadTaskRunner> callback_runner);
  ~IndexedDBDatabaseCallbacksImpl() override;

  // indexed_db::mojom::DatabaseCallbacks implementation
  void ForcedClose() override;
  void VersionChange(int64_t old_version, int64_t new_version) override;
  void Abort(int64_t transaction_id,
             int32_t code,
             const base::string16& message) override;
  void Complete(int64_t transaction_id) override;
  void Changes(indexed_db::mojom::ObserverChangesPtr changes) override;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> callback_runner_;
  blink::WebIDBDatabaseCallbacks* callbacks_;

  DISALLOW_COPY_AND_ASSIGN(IndexedDBDatabaseCallbacksImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_INDEXED_DB_INDEXED_DB_DATABASE_CALLBACKS_IMPL_H_
