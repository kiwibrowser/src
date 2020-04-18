// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_WEB_DATABASE_HOST_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_WEB_DATABASE_HOST_IMPL_H_

#include <string>

#include "base/strings/string16.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "storage/browser/database/database_tracker.h"
#include "third_party/blink/public/platform/modules/webdatabase/web_database.mojom.h"

namespace url {
class Origin;
}  // namespace url

namespace content {

class WebDatabaseHostImpl : public blink::mojom::WebDatabaseHost,
                            public storage::DatabaseTracker::Observer {
 public:
  WebDatabaseHostImpl(int proess_id,
                      scoped_refptr<storage::DatabaseTracker> db_tracker);
  ~WebDatabaseHostImpl() override;

  static void Create(int process_id,
                     scoped_refptr<storage::DatabaseTracker> db_tracker,
                     blink::mojom::WebDatabaseHostRequest request);

 private:
  // blink::mojom::WebDatabaseHost:
  void OpenFile(const base::string16& vfs_file_name,
                int32_t desired_flags,
                OpenFileCallback callback) override;

  void DeleteFile(const base::string16& vfs_file_name,
                  bool sync_dir,
                  DeleteFileCallback callback) override;

  void GetFileAttributes(const base::string16& vfs_file_name,
                         GetFileAttributesCallback callback) override;

  void GetFileSize(const base::string16& vfs_file_name,
                   GetFileSizeCallback callback) override;

  void SetFileSize(const base::string16& vfs_file_name,
                   int64_t expected_size,
                   SetFileSizeCallback callback) override;

  void GetSpaceAvailable(const url::Origin& origin,
                         GetSpaceAvailableCallback callback) override;

  void Opened(const url::Origin& origin,
              const base::string16& database_name,
              const base::string16& database_description,
              int64_t estimated_size) override;

  void Modified(const url::Origin& origin,
                const base::string16& database_name) override;

  void Closed(const url::Origin& origin,
              const base::string16& database_name) override;

  void HandleSqliteError(const url::Origin& origin,
                         const base::string16& database_name,
                         int32_t error) override;

  // DatabaseTracker::Observer callbacks (tracker sequence)
  void OnDatabaseSizeChanged(const std::string& origin_identifier,
                             const base::string16& database_name,
                             int64_t database_size) override;
  void OnDatabaseScheduledForDeletion(
      const std::string& origin_identifier,
      const base::string16& database_name) override;

  void DatabaseDeleteFile(const base::string16& vfs_file_name,
                          bool sync_dir,
                          DeleteFileCallback callback,
                          int reschedule_count);

  // Helper function to get the mojo interface for the WebDatabase on the
  // render process. Creates the WebDatabase connection if it does not already
  // exist.
  blink::mojom::WebDatabase& GetWebDatabase();

  // Our render process host ID, used to bind to the correct render process.
  const int process_id_;

  // True if and only if this instance was added as an observer
  // to DatabaseTracker.
  bool observer_added_;

  // Keeps track of all DB connections opened by this renderer
  storage::DatabaseConnections database_connections_;

  // Interface to the render process WebDatabase.
  blink::mojom::WebDatabasePtr database_provider_;

  // The database tracker for the current browser context.
  const scoped_refptr<storage::DatabaseTracker> db_tracker_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_WEB_DATABASE_HOST_IMPL_H_
