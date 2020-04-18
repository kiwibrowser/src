// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model_impl/model_type_store_backend.h"

#include <utility>

#include "base/test/histogram_tester.h"
#include "components/sync/protocol/model_type_store_schema_descriptor.pb.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"
#include "third_party/leveldatabase/src/include/leveldb/env.h"
#include "third_party/leveldatabase/src/include/leveldb/options.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"
#include "third_party/leveldatabase/src/include/leveldb/write_batch.h"

using sync_pb::ModelTypeStoreSchemaDescriptor;

namespace syncer {

class ModelTypeStoreBackendTest : public testing::Test {
 public:
  scoped_refptr<ModelTypeStoreBackend> GetOrCreateBackend() {
    std::string path = "/test_db";
    return GetOrCreateBackendWithPath(path);
  }

  scoped_refptr<ModelTypeStoreBackend> GetOrCreateBackendWithPath(
      std::string custom_path) {
    std::unique_ptr<leveldb::Env> in_memory_env =
        ModelTypeStoreBackend::CreateInMemoryEnv();
    std::string path;
    in_memory_env->GetTestDirectory(&path);
    path += custom_path;

    base::Optional<ModelError> error;
    // In-memory store backend works on the same thread as test.
    scoped_refptr<ModelTypeStoreBackend> backend =
        ModelTypeStoreBackend::GetOrCreateBackend(
            path, std::move(in_memory_env), &error);
    EXPECT_TRUE(backend.get());
    EXPECT_FALSE(error) << error->ToString();
    return backend;
  }

  // Create backend with custom env. This function is used in tests that need to
  // prepare env in some way (create files) before passing it to leveldb.
  scoped_refptr<ModelTypeStoreBackend> CreateBackendWithEnv(
      std::unique_ptr<leveldb::Env> env,
      const std::string& path) {
    EXPECT_FALSE(BackendExistsForPath(path));

    base::Optional<ModelError> error;
    scoped_refptr<ModelTypeStoreBackend> backend =
        ModelTypeStoreBackend::GetOrCreateBackend(path, std::move(env), &error);
    EXPECT_TRUE(backend.get());
    EXPECT_FALSE(error) << error->ToString();
    return backend;
  }

  bool BackendExistsForPath(const std::string& path) {
    return ModelTypeStoreBackend::BackendExistsForTest(path);
  }

  std::string GetBackendPath(scoped_refptr<ModelTypeStoreBackend> backend) {
    return backend->path_;
  }

  base::Optional<ModelError> Migrate(
      scoped_refptr<ModelTypeStoreBackend> backend,
      int64_t current_version,
      int64_t desired_version) {
    return backend->Migrate(current_version, desired_version);
  }

  bool Migrate0To1(scoped_refptr<ModelTypeStoreBackend> backend) {
    return backend->Migrate0To1();
  }

  int64_t GetStoreVersion(scoped_refptr<ModelTypeStoreBackend> backend) {
    return backend->GetStoreVersion();
  }

  int64_t LatestVersion() {
    return ModelTypeStoreBackend::kLatestSchemaVersion;
  }

  const char* SchemaId() {
    return ModelTypeStoreBackend::kDBSchemaDescriptorRecordId;
  }

  const char* StoreInitResultHistogramName() {
    return ModelTypeStoreBackend::kStoreInitResultHistogramName;
  }
};

// Test that after record is written to backend it can be read back even after
// backend is destroyed and recreated in the same environment.
TEST_F(ModelTypeStoreBackendTest, WriteThenRead) {
  scoped_refptr<ModelTypeStoreBackend> backend = GetOrCreateBackend();

  // Write record.
  std::unique_ptr<leveldb::WriteBatch> write_batch(new leveldb::WriteBatch());
  write_batch->Put("prefix:id1", "data1");
  base::Optional<ModelError> error =
      backend->WriteModifications(std::move(write_batch));
  ASSERT_FALSE(error) << error->ToString();

  // Read all records with prefix.
  ModelTypeStore::RecordList record_list;
  error = backend->ReadAllRecordsWithPrefix("prefix:", &record_list);
  ASSERT_FALSE(error) << error->ToString();
  ASSERT_EQ(1ul, record_list.size());
  ASSERT_EQ("id1", record_list[0].id);
  ASSERT_EQ("data1", record_list[0].value);
  record_list.clear();

  // Recreate backend and read all records with prefix.
  backend = GetOrCreateBackend();
  error = backend->ReadAllRecordsWithPrefix("prefix:", &record_list);
  ASSERT_FALSE(error) << error->ToString();
  ASSERT_EQ(1ul, record_list.size());
  ASSERT_EQ("id1", record_list[0].id);
  ASSERT_EQ("data1", record_list[0].value);
}

// Test that ReadAllRecordsWithPrefix correclty filters records by prefix.
TEST_F(ModelTypeStoreBackendTest, ReadAllRecordsWithPrefix) {
  scoped_refptr<ModelTypeStoreBackend> backend = GetOrCreateBackend();

  std::unique_ptr<leveldb::WriteBatch> write_batch(new leveldb::WriteBatch());
  write_batch->Put("prefix1:id1", "data1");
  write_batch->Put("prefix2:id2", "data2");
  base::Optional<ModelError> error =
      backend->WriteModifications(std::move(write_batch));
  ASSERT_FALSE(error) << error->ToString();

  ModelTypeStore::RecordList record_list;
  error = backend->ReadAllRecordsWithPrefix("prefix1:", &record_list);
  ASSERT_FALSE(error) << error->ToString();
  ASSERT_EQ(1UL, record_list.size());
  ASSERT_EQ("id1", record_list[0].id);
  ASSERT_EQ("data1", record_list[0].value);
}

// Test that deleted records are correctly marked as milling in results of
// ReadRecordsWithPrefix.
TEST_F(ModelTypeStoreBackendTest, ReadDeletedRecord) {
  scoped_refptr<ModelTypeStoreBackend> backend = GetOrCreateBackend();

  // Create records, ensure they are returned by ReadRecordsWithPrefix.
  std::unique_ptr<leveldb::WriteBatch> write_batch(new leveldb::WriteBatch());
  write_batch->Put("prefix:id1", "data1");
  write_batch->Put("prefix:id2", "data2");
  base::Optional<ModelError> error =
      backend->WriteModifications(std::move(write_batch));
  ASSERT_FALSE(error) << error->ToString();

  ModelTypeStore::IdList id_list;
  ModelTypeStore::IdList missing_id_list;
  ModelTypeStore::RecordList record_list;
  id_list.push_back("id1");
  id_list.push_back("id2");
  error = backend->ReadRecordsWithPrefix("prefix:", id_list, &record_list,
                                         &missing_id_list);
  ASSERT_FALSE(error) << error->ToString();
  ASSERT_EQ(2UL, record_list.size());
  ASSERT_TRUE(missing_id_list.empty());

  // Delete one record.
  write_batch = std::make_unique<leveldb::WriteBatch>();
  write_batch->Delete("prefix:id2");
  error = backend->WriteModifications(std::move(write_batch));
  ASSERT_FALSE(error) << error->ToString();

  // Ensure deleted record id is returned in missing_id_list.
  record_list.clear();
  missing_id_list.clear();
  error = backend->ReadRecordsWithPrefix("prefix:", id_list, &record_list,
                                         &missing_id_list);
  ASSERT_FALSE(error) << error->ToString();
  ASSERT_EQ(1UL, record_list.size());
  ASSERT_EQ("id1", record_list[0].id);
  ASSERT_EQ(1UL, missing_id_list.size());
  ASSERT_EQ("id2", missing_id_list[0]);
}

// Test that DeleteDataAndMetadataForPrefix correctly deletes records by prefix.
TEST_F(ModelTypeStoreBackendTest, DeleteDataAndMetadataForPrefix) {
  scoped_refptr<ModelTypeStoreBackend> backend = GetOrCreateBackend();

  auto write_batch = std::make_unique<leveldb::WriteBatch>();
  write_batch->Put("prefix1:id1", "data1");
  write_batch->Put("prefix2:id2", "data2");
  write_batch->Put("prefix2:id3", "data3");
  write_batch->Put("prefix3:id4", "data4");
  base::Optional<ModelError> error =
      backend->WriteModifications(std::move(write_batch));
  ASSERT_FALSE(error) << error->ToString();

  error = backend->DeleteDataAndMetadataForPrefix("prefix2:");
  EXPECT_FALSE(error) << error->ToString();

  {
    ModelTypeStore::RecordList record_list;
    error = backend->ReadAllRecordsWithPrefix("prefix2:", &record_list);
    EXPECT_FALSE(error) << error->ToString();
    EXPECT_EQ(0UL, record_list.size());
  }

  {
    ModelTypeStore::RecordList record_list;
    error = backend->ReadAllRecordsWithPrefix("prefix1:", &record_list);
    EXPECT_FALSE(error) << error->ToString();
    EXPECT_EQ(1UL, record_list.size());
  }

  {
    ModelTypeStore::RecordList record_list;
    error = backend->ReadAllRecordsWithPrefix("prefix3:", &record_list);
    EXPECT_FALSE(error) << error->ToString();
    EXPECT_EQ(1UL, record_list.size());
  }
}

// Test that only one backend got create when we ask two backend with same path,
// and after de-reference the backend, the backend will be deleted.
TEST_F(ModelTypeStoreBackendTest, TwoSameBackendTest) {
  // Create two backend with same path, check if they are reference to same
  // address.
  scoped_refptr<ModelTypeStoreBackend> backend = GetOrCreateBackend();
  scoped_refptr<ModelTypeStoreBackend> backend_second = GetOrCreateBackend();
  std::string path = GetBackendPath(backend);
  ASSERT_EQ(backend.get(), backend_second.get());

  // Delete one reference, check the real backend still here.
  backend = nullptr;
  ASSERT_FALSE(backend);
  ASSERT_TRUE(backend_second.get());
  ASSERT_TRUE(backend_second->HasOneRef());

  // Delete another reference, check the real backend is deleted.
  backend_second = nullptr;
  ASSERT_FALSE(backend_second);
  ASSERT_FALSE(BackendExistsForPath(path));
}

// Test that two backend got create when we ask two backend with different path,
// and after de-reference two backend, the both backend will be deleted.
TEST_F(ModelTypeStoreBackendTest, TwoDifferentBackendTest) {
  // Create two backend with different path, check if they are reference to
  // different address.
  scoped_refptr<ModelTypeStoreBackend> backend = GetOrCreateBackend();
  scoped_refptr<ModelTypeStoreBackend> backend_second =
      GetOrCreateBackendWithPath("/test_db2");
  std::string path = GetBackendPath(backend);
  ASSERT_NE(backend.get(), backend_second.get());
  ASSERT_TRUE(backend->HasOneRef());
  ASSERT_TRUE(backend_second->HasOneRef());

  // delete one backend, check only one got deleted.
  backend = nullptr;
  ASSERT_FALSE(backend);
  ASSERT_TRUE(backend_second.get());
  ASSERT_TRUE(backend_second->HasOneRef());
  ASSERT_FALSE(BackendExistsForPath(path));

  // delete another backend.
  backend_second = nullptr;
  ASSERT_FALSE(backend_second);
  ASSERT_FALSE(BackendExistsForPath("/test_db2"));
}

// Test that initializing the database migrates it to the latest schema version.
TEST_F(ModelTypeStoreBackendTest, MigrateNoSchemaVersionToLatestVersionTest) {
  scoped_refptr<ModelTypeStoreBackend> backend = GetOrCreateBackend();

  ASSERT_EQ(LatestVersion(), GetStoreVersion(backend));
}

// Test that the 0 to 1 migration succeeds and sets the schema version to 1.
TEST_F(ModelTypeStoreBackendTest, Migrate0To1Test) {
  scoped_refptr<ModelTypeStoreBackend> backend = GetOrCreateBackend();

  std::unique_ptr<leveldb::WriteBatch> write_batch(new leveldb::WriteBatch());
  write_batch->Delete(SchemaId());
  base::Optional<ModelError> error =
      backend->WriteModifications(std::move(write_batch));
  ASSERT_FALSE(error) << error->ToString();

  ASSERT_TRUE(Migrate0To1(backend));
  ASSERT_EQ(1, GetStoreVersion(backend));
}

// Test that migration to an unknown version fails
TEST_F(ModelTypeStoreBackendTest, MigrateWithHigherExistingVersionFails) {
  scoped_refptr<ModelTypeStoreBackend> backend = GetOrCreateBackend();

  base::Optional<ModelError> error =
      Migrate(backend, LatestVersion() + 1, LatestVersion());
  ASSERT_TRUE(error);
  EXPECT_EQ("Schema version too high", error->message());
}

// Tests that initializing store after corruption triggers recovery and results
// in successful store initialization.
TEST_F(ModelTypeStoreBackendTest, RecoverAfterCorruption) {
  base::HistogramTester tester;
  leveldb::Status s;

  // Prepare environment that looks corrupt to leveldb.
  std::unique_ptr<leveldb::Env> env =
      std::make_unique<leveldb::EnvWrapper>(leveldb::Env::Default());

  std::string path;
  env->GetTestDirectory(&path);
  path += "/corrupt_db";

  // Easiest way to simulate leveldb corruption is to create empty CURRENT file.
  {
    s = env->CreateDir(path);
    EXPECT_TRUE(s.ok());
    leveldb::WritableFile* current_file_raw;
    s = env->NewWritableFile(path + "/CURRENT", &current_file_raw);
    EXPECT_TRUE(s.ok());
    current_file_raw->Close();
    delete current_file_raw;
  }

  // CreateBackendWithEnv will ensure backend initialization is successful.
  scoped_refptr<ModelTypeStoreBackend> backend =
      CreateBackendWithEnv(std::move(env), path);

  // Cleanup directory after the test.
  backend = nullptr;
  s = leveldb::DestroyDB(path, leveldb_env::Options());
  EXPECT_TRUE(s.ok()) << s.ToString();

  // Check that both recovery and consecutive initialization are recorded in
  // histograms.
  tester.ExpectBucketCount(StoreInitResultHistogramName(),
                           STORE_INIT_RESULT_SUCCESS, 1);
  tester.ExpectBucketCount(StoreInitResultHistogramName(),
                           STORE_INIT_RESULT_RECOVERED_AFTER_CORRUPTION, 1);
}

}  // namespace syncer
