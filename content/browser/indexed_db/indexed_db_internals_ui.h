// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_INTERNALS_UI_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_INTERNALS_UI_H_

#include <stddef.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/download/public/common/download_interrupt_reasons.h"
#include "content/public/browser/indexed_db_context.h"
#include "content/public/browser/web_ui_controller.h"

namespace base {
class ListValue;
}

namespace url {
class Origin;
}

namespace download {
class DownloadItem;
}

namespace content {

class IndexedDBContextImpl;
class StoragePartition;

// The implementation for the chrome://indexeddb-internals page.
class IndexedDBInternalsUI : public WebUIController {
 public:
  explicit IndexedDBInternalsUI(WebUI* web_ui);
  ~IndexedDBInternalsUI() override;

 private:
  void GetAllOrigins(const base::ListValue* args);
  void GetAllOriginsOnIndexedDBThread(scoped_refptr<IndexedDBContext> context,
                                      const base::FilePath& context_path);
  void OnOriginsReady(std::unique_ptr<base::ListValue> origins,
                      const base::FilePath& path);

  void AddContextFromStoragePartition(StoragePartition* partition);

  void DownloadOriginData(const base::ListValue* args);
  void DownloadOriginDataOnIndexedDBThread(
      const base::FilePath& partition_path,
      const scoped_refptr<IndexedDBContextImpl> context,
      const url::Origin& origin);
  void OnDownloadDataReady(const base::FilePath& partition_path,
                           const url::Origin& origin,
                           const base::FilePath temp_path,
                           const base::FilePath zip_path,
                           size_t connection_count);
  void OnDownloadStarted(const base::FilePath& partition_path,
                         const url::Origin& origin,
                         const base::FilePath& temp_path,
                         size_t connection_count,
                         download::DownloadItem* item,
                         download::DownloadInterruptReason interrupt_reason);

  void ForceCloseOrigin(const base::ListValue* args);
  void ForceCloseOriginOnIndexedDBThread(
      const base::FilePath& partition_path,
      const scoped_refptr<IndexedDBContextImpl> context,
      const url::Origin& origin);
  void OnForcedClose(const base::FilePath& partition_path,
                     const url::Origin& origin,
                     size_t connection_count);
  bool GetOriginContext(const base::FilePath& path,
                        const url::Origin& origin,
                        scoped_refptr<IndexedDBContextImpl>* context);
  bool GetOriginData(const base::ListValue* args,
                     base::FilePath* path,
                     url::Origin* origin,
                     scoped_refptr<IndexedDBContextImpl>* context);

  DISALLOW_COPY_AND_ASSIGN(IndexedDBInternalsUI);
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_INTERNALS_UI_H_
