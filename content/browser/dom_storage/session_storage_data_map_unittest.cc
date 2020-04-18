// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/session_storage_data_map.h"

#include <map>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "components/services/leveldb/public/cpp/util.h"
#include "content/test/fake_leveldb_database.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {
namespace {
using leveldb::StdStringToUint8Vector;
using leveldb::Uint8VectorToStdString;

class MockListener : public SessionStorageDataMap::Listener {
 public:
  MockListener() {}
  ~MockListener() override {}
  MOCK_METHOD2(OnDataMapCreation,
               void(const std::vector<uint8_t>& map_id,
                    SessionStorageDataMap* map));
  MOCK_METHOD1(OnDataMapDestruction, void(const std::vector<uint8_t>& map_id));
  MOCK_METHOD1(OnCommitResult, void(leveldb::mojom::DatabaseError error));
};

void GetAllDataCallback(leveldb::mojom::DatabaseError* status_out,
                        std::vector<mojom::KeyValuePtr>* data_out,
                        leveldb::mojom::DatabaseError status,
                        std::vector<mojom::KeyValuePtr> data) {
  *status_out = status;
  *data_out = std::move(data);
}

base::OnceCallback<void(leveldb::mojom::DatabaseError status,
                        std::vector<mojom::KeyValuePtr> data)>
MakeGetAllCallback(leveldb::mojom::DatabaseError* status_out,
                   std::vector<mojom::KeyValuePtr>* data_out) {
  return base::BindOnce(&GetAllDataCallback, status_out, data_out);
}

class GetAllCallback : public mojom::LevelDBWrapperGetAllCallback {
 public:
  static mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo CreateAndBind(
      bool* result,
      base::OnceClosure callback) {
    mojom::LevelDBWrapperGetAllCallbackAssociatedPtr ptr;
    auto request = mojo::MakeRequestAssociatedWithDedicatedPipe(&ptr);
    mojo::MakeStrongAssociatedBinding(
        base::WrapUnique(new GetAllCallback(result, std::move(callback))),
        std::move(request));
    return ptr.PassInterface();
  }

 private:
  GetAllCallback(bool* result, base::OnceClosure callback)
      : result_(result), callback_(std::move(callback)) {}
  void Complete(bool success) override {
    *result_ = success;
    if (callback_)
      std::move(callback_).Run();
  }

  bool* result_;
  base::OnceClosure callback_;
};

class SessionStorageDataMapTest : public testing::Test {
 public:
  SessionStorageDataMapTest()
      : test_origin_(url::Origin::Create(GURL("http://host1.com:1"))),
        database_(&mock_data_) {
    // Should show up in first map.
    mock_data_[StdStringToUint8Vector("map-1-key1")] =
        StdStringToUint8Vector("data1");
    // Dummy data to verify we don't delete everything.
    mock_data_[StdStringToUint8Vector("map-3-key1")] =
        StdStringToUint8Vector("data3");
  }
  ~SessionStorageDataMapTest() override {}

 protected:
  base::test::ScopedTaskEnvironment task_environment_;
  testing::StrictMock<MockListener> listener_;
  url::Origin test_origin_;
  std::map<std::vector<uint8_t>, std::vector<uint8_t>> mock_data_;
  FakeLevelDBDatabase database_;
};

}  // namespace

TEST_F(SessionStorageDataMapTest, BasicEmptyCreation) {
  EXPECT_CALL(listener_,
              OnDataMapCreation(StdStringToUint8Vector("1"), testing::_))
      .Times(1);

  scoped_refptr<SessionStorageDataMap> map = SessionStorageDataMap::Create(
      &listener_,
      base::MakeRefCounted<SessionStorageMetadata::MapData>(1, test_origin_),
      &database_);

  leveldb::mojom::DatabaseError status;
  std::vector<mojom::KeyValuePtr> data;
  bool done = false;
  base::RunLoop loop;
  map->level_db_wrapper()->GetAll(
      GetAllCallback::CreateAndBind(&done, loop.QuitClosure()),
      MakeGetAllCallback(&status, &data));
  loop.Run();

  EXPECT_TRUE(done);
  ASSERT_EQ(1u, data.size());
  EXPECT_EQ(StdStringToUint8Vector("key1"), data[0]->key);
  EXPECT_EQ(StdStringToUint8Vector("data1"), data[0]->value);

  EXPECT_CALL(listener_, OnDataMapDestruction(StdStringToUint8Vector("1")))
      .Times(1);

  // Test data is not cleared on deletion.
  map = nullptr;
  EXPECT_EQ(2u, mock_data_.size());
}

TEST_F(SessionStorageDataMapTest, Clone) {
  EXPECT_CALL(listener_,
              OnDataMapCreation(StdStringToUint8Vector("1"), testing::_))
      .Times(1);

  scoped_refptr<SessionStorageDataMap> map1 = SessionStorageDataMap::Create(
      &listener_,
      base::MakeRefCounted<SessionStorageMetadata::MapData>(1, test_origin_),
      &database_);

  EXPECT_CALL(listener_,
              OnDataMapCreation(StdStringToUint8Vector("2"), testing::_))
      .Times(1);
  // One call on fork.
  EXPECT_CALL(listener_, OnCommitResult(leveldb::mojom::DatabaseError::OK))
      .Times(1);

  scoped_refptr<SessionStorageDataMap> map2 =
      SessionStorageDataMap::CreateClone(
          &listener_,
          base::MakeRefCounted<SessionStorageMetadata::MapData>(2,
                                                                test_origin_),
          map1->level_db_wrapper());

  leveldb::mojom::DatabaseError status;
  std::vector<mojom::KeyValuePtr> data;
  bool done = false;
  base::RunLoop loop;
  map2->level_db_wrapper()->GetAll(
      GetAllCallback::CreateAndBind(&done, loop.QuitClosure()),
      MakeGetAllCallback(&status, &data));
  loop.Run();

  EXPECT_TRUE(done);
  ASSERT_EQ(1u, data.size());
  EXPECT_EQ(StdStringToUint8Vector("key1"), data[0]->key);
  EXPECT_EQ(StdStringToUint8Vector("data1"), data[0]->value);

  // Test that the data was copied.
  EXPECT_EQ(StdStringToUint8Vector("data1"),
            mock_data_[StdStringToUint8Vector("map-2-key1")]);

  EXPECT_CALL(listener_, OnDataMapDestruction(StdStringToUint8Vector("1")))
      .Times(1);
  EXPECT_CALL(listener_, OnDataMapDestruction(StdStringToUint8Vector("2")))
      .Times(1);

  // Test data is not cleared on deletion.
  map1 = nullptr;
  map2 = nullptr;
  EXPECT_EQ(3u, mock_data_.size());
}

}  // namespace content
