// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BLOB_STORAGE_CHROME_BLOB_STORAGE_CONTEXT_H_
#define CONTENT_BROWSER_BLOB_STORAGE_CHROME_BLOB_STORAGE_CONTEXT_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner_helpers.h"
#include "content/common/content_export.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "third_party/blink/public/mojom/blob/blob_url_store.mojom.h"

namespace base {
class TaskRunner;
}

namespace network {
class ResourceRequestBody;
}

namespace storage {
class BlobStorageContext;
}

namespace content {
class BlobHandle;
class BrowserContext;
struct ChromeBlobStorageContextDeleter;
class ResourceContext;

// A context class that keeps track of BlobStorageController used by the chrome.
// There is an instance associated with each BrowserContext. There could be
// multiple URLRequestContexts in the same browser context that refers to the
// same instance.
//
// All methods, except the ctor, are expected to be called on
// the IO thread (unless specifically called out in doc comments).
class CONTENT_EXPORT ChromeBlobStorageContext
    : public base::RefCountedThreadSafe<ChromeBlobStorageContext,
                                        ChromeBlobStorageContextDeleter> {
 public:
  ChromeBlobStorageContext();

  // Must be called on the UI thread.
  static ChromeBlobStorageContext* GetFor(
      BrowserContext* browser_context);

  void InitializeOnIOThread(base::FilePath blob_storage_dir,
                            scoped_refptr<base::TaskRunner> file_task_runner);

  storage::BlobStorageContext* context() const { return context_.get(); }

  // Returns a NULL scoped_ptr on failure.
  std::unique_ptr<BlobHandle> CreateMemoryBackedBlob(
      const char* data,
      size_t length,
      const std::string& content_type);

  // Must be called on the UI thread.
  static scoped_refptr<network::SharedURLLoaderFactory>
  URLLoaderFactoryForToken(BrowserContext* browser_context,
                           blink::mojom::BlobURLTokenPtr token);

 protected:
  virtual ~ChromeBlobStorageContext();

 private:
  friend class base::DeleteHelper<ChromeBlobStorageContext>;
  friend class base::RefCountedThreadSafe<ChromeBlobStorageContext,
                                          ChromeBlobStorageContextDeleter>;
  friend struct ChromeBlobStorageContextDeleter;

  void DeleteOnCorrectThread() const;

  std::unique_ptr<storage::BlobStorageContext> context_;
};

struct ChromeBlobStorageContextDeleter {
  static void Destruct(const ChromeBlobStorageContext* context) {
    context->DeleteOnCorrectThread();
  }
};

// Returns the BlobStorageContext associated with the
// ChromeBlobStorageContext instance passed in.
storage::BlobStorageContext* GetBlobStorageContext(
    ChromeBlobStorageContext* blob_storage_context);

using BlobHandles = std::vector<std::unique_ptr<storage::BlobDataHandle>>;

// Attempts to create a vector of BlobDataHandles that ensure any blob data
// associated with |body| isn't cleaned up until the handles are destroyed.
// Returns false on failure. This is used for POST and PUT requests.
bool GetBodyBlobDataHandles(network::ResourceRequestBody* body,
                            ResourceContext* resource_context,
                            BlobHandles* blob_handles);

extern const char kBlobStorageContextKeyName[];

}  // namespace content

#endif  // CONTENT_BROWSER_BLOB_STORAGE_CHROME_BLOB_STORAGE_CONTEXT_H_
