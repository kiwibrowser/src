// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_TEST_FAKE_LEVELDB_SERVICE_H_
#define CONTENT_BROWSER_DOM_STORAGE_TEST_FAKE_LEVELDB_SERVICE_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/optional.h"
#include "components/services/leveldb/public/interfaces/leveldb.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/file/public/mojom/file_system.mojom.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace content {
namespace test {

// Fake implementation of the LevelDBService. All open calls are saved to
// open_requests(), and all destroy calls are saved to destroy_requests().
// The user can be notified when an open request happens using the
// SetOnOpenCallback, which will be called when any Open* is called.
class FakeLevelDBService : public leveldb::mojom::LevelDBService {
 public:
  struct OpenRequest {
    OpenRequest();
    OpenRequest(bool in_memory,
                std::string dbname,
                std::string memenv_tracking_name,
                leveldb::mojom::LevelDBDatabaseAssociatedRequest request,
                OpenCallback callback);
    OpenRequest(OpenRequest&&);
    ~OpenRequest();

    bool in_memory = false;
    std::string dbname;
    std::string memenv_tracking_name;
    leveldb::mojom::LevelDBDatabaseAssociatedRequest request;
    OpenCallback callback;
  };

  struct DestroyRequest {
    std::string dbname;
  };
  FakeLevelDBService();
  ~FakeLevelDBService() override;

  void Open(filesystem::mojom::DirectoryPtr,
            const std::string& dbname,
            const base::Optional<base::trace_event::MemoryAllocatorDumpGuid>&
                memory_dump_id,
            leveldb::mojom::LevelDBDatabaseAssociatedRequest request,
            OpenCallback callback) override;

  void OpenWithOptions(
      const leveldb_env::Options& options,
      filesystem::mojom::DirectoryPtr,
      const std::string& dbname,
      const base::Optional<base::trace_event::MemoryAllocatorDumpGuid>&
          memory_dump_id,
      leveldb::mojom::LevelDBDatabaseAssociatedRequest request,
      OpenCallback callback) override;

  void OpenInMemory(
      const base::Optional<base::trace_event::MemoryAllocatorDumpGuid>&
          memory_dump_id,
      const std::string& tracking_name,
      leveldb::mojom::LevelDBDatabaseAssociatedRequest request,
      OpenCallback callback) override;

  void Destroy(filesystem::mojom::DirectoryPtr,
               const std::string& dbname,
               DestroyCallback callback) override;

  std::vector<OpenRequest>& open_requests() { return open_requests_; }

  std::vector<DestroyRequest>& destroy_requests() { return destroy_requests_; }

  void SetOnOpenCallback(base::OnceClosure on_open_callback) {
    on_open_callback_ = std::move(on_open_callback);
  }

  void Bind(const std::string& interface_name,
            mojo::ScopedMessagePipeHandle interface_pipe,
            const service_manager::BindSourceInfo& source_info);

 private:
  mojo::BindingSet<leveldb::mojom::LevelDBService> bindings_;

  std::vector<OpenRequest> open_requests_;
  base::OnceClosure on_open_callback_;
  std::vector<DestroyRequest> destroy_requests_;
};

}  // namespace test
}  // namespace content

#endif  // CONTENT_BROWSER_DOM_STORAGE_TEST_FAKE_LEVELDB_SERVICE_H_
