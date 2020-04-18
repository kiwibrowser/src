// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BLOB_STORAGE_BLOB_DISPATCHER_HOST_H_
#define CONTENT_BROWSER_BLOB_STORAGE_BLOB_DISPATCHER_HOST_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_message_filter.h"

class GURL;

namespace storage {
class BlobStorageContext;
}

namespace content {
class ChromeBlobStorageContext;

// This class's responsibility is to listen for and dispatch blob storage
// messages and handle logistics of blob storage for a single child process.
// When the child process terminates all blob references attributable to
// that process go away upon destruction of the instance.
// This lives in the browser process, is single threaded (IO thread), and there
// is one per child process.
class CONTENT_EXPORT BlobDispatcherHost : public BrowserMessageFilter {
 public:
  BlobDispatcherHost(
      int process_id,
      scoped_refptr<ChromeBlobStorageContext> blob_storage_context);

  // BrowserMessageFilter implementation.
  void OnChannelClosing() override;
  bool OnMessageReceived(const IPC::Message& message) override;

 protected:
  ~BlobDispatcherHost() override;

 private:
  friend class base::RefCountedThreadSafe<BlobDispatcherHost>;

  void OnRegisterPublicBlobURL(const GURL& public_url, const std::string& uuid);
  void OnRevokePublicBlobURL(const GURL& public_url);

  storage::BlobStorageContext* context();

  bool IsUrlRegisteredInHost(const GURL& blob_url);

  // Unregisters all urls that were registered in this host.
  void ClearHostFromBlobStorageContext();

  const int process_id_;

  // The set of public blob urls coined by this consumer.
  std::set<GURL> public_blob_urls_;

  scoped_refptr<ChromeBlobStorageContext> blob_storage_context_;

  DISALLOW_COPY_AND_ASSIGN(BlobDispatcherHost);
};
}  // namespace content
#endif  // CONTENT_BROWSER_BLOB_STORAGE_BLOB_DISPATCHER_HOST_H_
