// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/session_storage_leveldb_wrapper.h"

#include "base/barrier_closure.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/guid.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/bind_test_util.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread.h"
#include "components/services/leveldb/leveldb_service_impl.h"
#include "components/services/leveldb/public/cpp/util.h"
#include "components/services/leveldb/public/interfaces/leveldb.mojom.h"
#include "content/browser/dom_storage/session_storage_data_map.h"
#include "content/browser/dom_storage/session_storage_metadata.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/test/fake_leveldb_database.h"
#include "content/test/gmock_util.h"
#include "content/test/leveldb_wrapper_test_util.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/bindings/strong_binding_set.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {
namespace {
using leveldb::StdStringToUint8Vector;
using leveldb::Uint8VectorToStdString;
using leveldb::mojom::DatabaseError;

template <typename Interface, typename Impl>
void CreateStrongBindingOnTaskRunner(
    scoped_refptr<base::SequencedTaskRunner> runner,
    mojo::InterfacePtr<Interface>* interface_ptr,
    std::unique_ptr<Impl> interface) {
  runner->PostTask(
      FROM_HERE,
      base::BindOnce(
          base::IgnoreResult(&mojo::MakeStrongBinding<Interface, Impl>),
          std::move(interface), mojo::MakeRequest(interface_ptr)));
}

class MockListener : public SessionStorageDataMap::Listener {
 public:
  MockListener() = default;
  ~MockListener() override = default;
  MOCK_METHOD2(OnDataMapCreation,
               void(const std::vector<uint8_t>& map_id,
                    SessionStorageDataMap* map));
  MOCK_METHOD1(OnDataMapDestruction, void(const std::vector<uint8_t>& map_id));
  MOCK_METHOD1(OnCommitResult, void(DatabaseError error));
};

class SessionStorageLevelDBWrapperTest : public testing::Test {
 public:
  SessionStorageLevelDBWrapperTest()
      : test_namespace_id1_(base::GenerateGUID()),
        test_namespace_id2_(base::GenerateGUID()),
        test_origin1_(url::Origin::Create(GURL("https://host1.com:1/"))),
        test_origin2_(url::Origin::Create(GURL("https://host2.com:2/"))) {
    auto file_runner =
        base::CreateSequencedTaskRunnerWithTraits({base::MayBlock()});
    CreateStrongBindingOnTaskRunner(
        base::CreateSequencedTaskRunnerWithTraits({}), &leveldb_service_,
        std::make_unique<leveldb::LevelDBServiceImpl>(std::move(file_runner)));

    leveldb_service_->OpenInMemory(
        base::nullopt, "SessionStorageLevelDBWrapperTestDatabase",
        mojo::MakeRequest(&leveldb_database_), base::DoNothing());

    leveldb_database_->Put(StdStringToUint8Vector("map-0-key1"),
                           StdStringToUint8Vector("data1"), base::DoNothing());

    std::vector<leveldb::mojom::BatchedOperationPtr> save_operations =
        metadata_.SetupNewDatabase();
    auto map_id = metadata_.RegisterNewMap(
        metadata_.GetOrCreateNamespaceEntry(test_namespace_id1_), test_origin1_,
        &save_operations);
    DCHECK(map_id->KeyPrefix() == StdStringToUint8Vector("map-0-"));
    leveldb_database_->Write(std::move(save_operations), base::DoNothing());
  }
  ~SessionStorageLevelDBWrapperTest() override = default;

  scoped_refptr<SessionStorageMetadata::MapData> RegisterNewAreaMap(
      SessionStorageMetadata::NamespaceEntry namespace_entry,
      const url::Origin& origin) {
    std::vector<leveldb::mojom::BatchedOperationPtr> save_operations;
    auto map_data =
        metadata_.RegisterNewMap(namespace_entry, origin, &save_operations);
    leveldb_database_->Write(std::move(save_operations), base::DoNothing());
    return map_data;
  }

  SessionStorageLevelDBWrapper::RegisterNewAreaMap
  GetRegisterNewAreaMapCallback() {
    return base::BindRepeating(
        &SessionStorageLevelDBWrapperTest::RegisterNewAreaMap,
        base::Unretained(this));
  }

 protected:
  base::test::ScopedTaskEnvironment environment_;
  const std::string test_namespace_id1_;
  const std::string test_namespace_id2_;
  const url::Origin test_origin1_;
  const url::Origin test_origin2_;
  leveldb::mojom::LevelDBServicePtr leveldb_service_;
  leveldb::mojom::LevelDBDatabaseAssociatedPtr leveldb_database_;
  SessionStorageMetadata metadata_;

  testing::StrictMock<MockListener> listener_;
};
}  // namespace

TEST_F(SessionStorageLevelDBWrapperTest, BasicUsage) {
  EXPECT_CALL(listener_,
              OnDataMapCreation(StdStringToUint8Vector("0"), testing::_))
      .Times(1);

  auto ss_leveldb_impl = std::make_unique<SessionStorageLevelDBWrapper>(
      metadata_.GetOrCreateNamespaceEntry(test_namespace_id1_), test_origin1_,
      SessionStorageDataMap::Create(
          &listener_,
          metadata_.GetOrCreateNamespaceEntry(test_namespace_id1_)
              ->second[test_origin1_],
          leveldb_database_.get()),
      GetRegisterNewAreaMapCallback());

  mojom::LevelDBWrapperAssociatedPtr ss_leveldb;
  ss_leveldb_impl->Bind(
      mojo::MakeRequestAssociatedWithDedicatedPipe(&ss_leveldb));

  std::vector<mojom::KeyValuePtr> data;
  DatabaseError status = test::GetAllSync(ss_leveldb.get(), &data);
  EXPECT_EQ(DatabaseError::OK, status);
  ASSERT_EQ(1ul, data.size());
  EXPECT_TRUE(base::ContainsValue(
      data, mojom::KeyValue::New(StdStringToUint8Vector("key1"),
                                 StdStringToUint8Vector("data1"))));

  EXPECT_CALL(listener_, OnDataMapDestruction(StdStringToUint8Vector("0")))
      .Times(1);
}

TEST_F(SessionStorageLevelDBWrapperTest, Cloning) {
  EXPECT_CALL(listener_,
              OnDataMapCreation(StdStringToUint8Vector("0"), testing::_))
      .Times(1);

  auto ss_leveldb_impl1 = std::make_unique<SessionStorageLevelDBWrapper>(
      metadata_.GetOrCreateNamespaceEntry(test_namespace_id1_), test_origin1_,
      SessionStorageDataMap::Create(
          &listener_,
          metadata_.GetOrCreateNamespaceEntry(test_namespace_id1_)
              ->second[test_origin1_],
          leveldb_database_.get()),
      GetRegisterNewAreaMapCallback());

  // Perform a shallow clone.
  std::vector<leveldb::mojom::BatchedOperationPtr> save_operations;
  metadata_.RegisterShallowClonedNamespace(
      metadata_.GetOrCreateNamespaceEntry(test_namespace_id1_),
      metadata_.GetOrCreateNamespaceEntry(test_namespace_id2_),
      &save_operations);
  leveldb_database_->Write(std::move(save_operations), base::DoNothing());
  auto ss_leveldb_impl2 = ss_leveldb_impl1->Clone(
      metadata_.GetOrCreateNamespaceEntry(test_namespace_id2_));

  mojom::LevelDBWrapperAssociatedPtr ss_leveldb1;
  ss_leveldb_impl1->Bind(
      mojo::MakeRequestAssociatedWithDedicatedPipe(&ss_leveldb1));
  mojom::LevelDBWrapperAssociatedPtr ss_leveldb2;
  ss_leveldb_impl2->Bind(
      mojo::MakeRequestAssociatedWithDedicatedPipe(&ss_leveldb2));

  // Same maps are used.
  EXPECT_EQ(ss_leveldb_impl1->data_map(), ss_leveldb_impl2->data_map());

  // The |Put| call will fork the maps.
  EXPECT_CALL(listener_,
              OnDataMapCreation(StdStringToUint8Vector("1"), testing::_))
      .Times(1);
  EXPECT_CALL(listener_, OnCommitResult(DatabaseError::OK))
      .Times(testing::AnyNumber());
  EXPECT_TRUE(test::PutSync(ss_leveldb2.get(), StdStringToUint8Vector("key2"),
                            StdStringToUint8Vector("data2"), base::nullopt,
                            ""));

  // The maps were forked on the above put.
  EXPECT_NE(ss_leveldb_impl1->data_map(), ss_leveldb_impl2->data_map());

  // Check map 1 data.
  std::vector<mojom::KeyValuePtr> data;
  DatabaseError status = test::GetAllSync(ss_leveldb1.get(), &data);
  EXPECT_EQ(DatabaseError::OK, status);
  ASSERT_EQ(1ul, data.size());
  EXPECT_TRUE(base::ContainsValue(
      data, mojom::KeyValue::New(StdStringToUint8Vector("key1"),
                                 StdStringToUint8Vector("data1"))));

  // Check map 2 data.
  data.clear();
  status = test::GetAllSync(ss_leveldb2.get(), &data);
  EXPECT_EQ(DatabaseError::OK, status);
  ASSERT_EQ(2ul, data.size());
  EXPECT_TRUE(base::ContainsValue(
      data, mojom::KeyValue::New(StdStringToUint8Vector("key1"),
                                 StdStringToUint8Vector("data1"))));
  EXPECT_TRUE(base::ContainsValue(
      data, mojom::KeyValue::New(StdStringToUint8Vector("key2"),
                                 StdStringToUint8Vector("data2"))));

  EXPECT_CALL(listener_, OnDataMapDestruction(StdStringToUint8Vector("0")))
      .Times(1);
  EXPECT_CALL(listener_, OnDataMapDestruction(StdStringToUint8Vector("1")))
      .Times(1);

  ss_leveldb_impl1 = nullptr;
  ss_leveldb_impl2 = nullptr;
}

TEST_F(SessionStorageLevelDBWrapperTest, ObserverTransfer) {
  EXPECT_CALL(listener_,
              OnDataMapCreation(StdStringToUint8Vector("0"), testing::_))
      .Times(1);

  auto ss_leveldb_impl1 = std::make_unique<SessionStorageLevelDBWrapper>(
      metadata_.GetOrCreateNamespaceEntry(test_namespace_id1_), test_origin1_,
      SessionStorageDataMap::Create(
          &listener_,
          metadata_.GetOrCreateNamespaceEntry(test_namespace_id1_)
              ->second[test_origin1_],
          leveldb_database_.get()),
      GetRegisterNewAreaMapCallback());

  // Create the mojo binding.
  mojom::LevelDBWrapperAssociatedPtr ss_leveldb1;
  ss_leveldb_impl1->Bind(
      mojo::MakeRequestAssociatedWithDedicatedPipe(&ss_leveldb1));

  // Create the observer, and attach it to the mojo bound implementation.
  testing::StrictMock<test::MockLevelDBObserver> mock_observer;
  mojo::AssociatedBinding<mojom::LevelDBObserver> observer_binding(
      &mock_observer);
  mojom::LevelDBObserverAssociatedPtrInfo observer_ptr_info;
  observer_binding.Bind(mojo::MakeRequest(&observer_ptr_info));
  ss_leveldb1->AddObserver(std::move(observer_ptr_info));

  // Perform a shallow clone.
  std::vector<leveldb::mojom::BatchedOperationPtr> save_operations;
  metadata_.RegisterShallowClonedNamespace(
      metadata_.GetOrCreateNamespaceEntry(test_namespace_id1_),
      metadata_.GetOrCreateNamespaceEntry(test_namespace_id2_),
      &save_operations);
  leveldb_database_->Write(std::move(save_operations), base::DoNothing());
  auto ss_leveldb_impl2 = ss_leveldb_impl1->Clone(
      metadata_.GetOrCreateNamespaceEntry(test_namespace_id2_));
  mojom::LevelDBWrapperAssociatedPtr ss_leveldb2;
  ss_leveldb_impl2->Bind(
      mojo::MakeRequestAssociatedWithDedicatedPipe(&ss_leveldb2));

  // Wait for our future commits to finish.
  base::RunLoop commit_waiters;
  auto barrier = base::BarrierClosure(3, commit_waiters.QuitClosure());
  EXPECT_CALL(listener_, OnCommitResult(DatabaseError::OK))
      .Times(3)
      .WillRepeatedly(base::test::RunClosure(barrier));

  // Do a change on the new interface. There should be no observation.
  EXPECT_CALL(listener_,
              OnDataMapCreation(StdStringToUint8Vector("1"), testing::_))
      .Times(1);
  EXPECT_TRUE(test::PutSync(ss_leveldb2.get(), StdStringToUint8Vector("key2"),
                            StdStringToUint8Vector("data2"), base::nullopt,
                            ""));
  ss_leveldb_impl2->data_map()->level_db_wrapper()->ScheduleImmediateCommit();

  // Do a change on the old interface.
  EXPECT_CALL(mock_observer,
              KeyChanged(StdStringToUint8Vector("key1"),
                         StdStringToUint8Vector("data2"),
                         StdStringToUint8Vector("data1"), "ss_leveldb1"))
      .Times(1);
  EXPECT_TRUE(test::PutSync(ss_leveldb1.get(), StdStringToUint8Vector("key1"),
                            StdStringToUint8Vector("data2"),
                            StdStringToUint8Vector("data1"), "ss_leveldb1"));
  ss_leveldb_impl1->data_map()->level_db_wrapper()->ScheduleImmediateCommit();

  // Wait for the commits.
  commit_waiters.Run();

  EXPECT_CALL(listener_, OnDataMapDestruction(StdStringToUint8Vector("0")))
      .Times(1);
  EXPECT_CALL(listener_, OnDataMapDestruction(StdStringToUint8Vector("1")))
      .Times(1);

  ss_leveldb_impl1 = nullptr;
  ss_leveldb_impl2 = nullptr;
}

TEST_F(SessionStorageLevelDBWrapperTest, DeleteAllOnShared) {
  EXPECT_CALL(listener_,
              OnDataMapCreation(StdStringToUint8Vector("0"), testing::_))
      .Times(1);

  auto ss_leveldb_impl1 = std::make_unique<SessionStorageLevelDBWrapper>(
      metadata_.GetOrCreateNamespaceEntry(test_namespace_id1_), test_origin1_,
      SessionStorageDataMap::Create(
          &listener_,
          metadata_.GetOrCreateNamespaceEntry(test_namespace_id1_)
              ->second[test_origin1_],
          leveldb_database_.get()),
      GetRegisterNewAreaMapCallback());

  // Perform a shallow clone.
  std::vector<leveldb::mojom::BatchedOperationPtr> save_operations;
  metadata_.RegisterShallowClonedNamespace(
      metadata_.GetOrCreateNamespaceEntry(test_namespace_id1_),
      metadata_.GetOrCreateNamespaceEntry(test_namespace_id2_),
      &save_operations);
  leveldb_database_->Write(std::move(save_operations), base::DoNothing());
  auto ss_leveldb_impl2 = ss_leveldb_impl1->Clone(
      metadata_.GetOrCreateNamespaceEntry(test_namespace_id2_));

  mojom::LevelDBWrapperAssociatedPtr ss_leveldb1;
  ss_leveldb_impl1->Bind(
      mojo::MakeRequestAssociatedWithDedicatedPipe(&ss_leveldb1));
  mojom::LevelDBWrapperAssociatedPtr ss_leveldb2;
  ss_leveldb_impl2->Bind(
      mojo::MakeRequestAssociatedWithDedicatedPipe(&ss_leveldb2));

  // Same maps are used.
  EXPECT_EQ(ss_leveldb_impl1->data_map(), ss_leveldb_impl2->data_map());

  // Create the observer, attach to the first namespace.
  testing::StrictMock<test::MockLevelDBObserver> mock_observer;
  mojo::AssociatedBinding<mojom::LevelDBObserver> observer_binding(
      &mock_observer);
  mojom::LevelDBObserverAssociatedPtrInfo observer_ptr_info;
  observer_binding.Bind(mojo::MakeRequest(&observer_ptr_info));
  ss_leveldb1->AddObserver(std::move(observer_ptr_info));

  // The |DeleteAll| call will fork the maps, and the observer should see a
  // DeleteAll.
  EXPECT_CALL(listener_,
              OnDataMapCreation(StdStringToUint8Vector("1"), testing::_))
      .Times(1);
  // There should be no commits, as we don't actually have to change any data.
  // |ss_leveldb_impl1| should just switch to a new, empty map.
  EXPECT_CALL(listener_, OnCommitResult(DatabaseError::OK)).Times(0);
  EXPECT_CALL(mock_observer, AllDeleted("source")).Times(1);
  EXPECT_TRUE(test::DeleteAllSync(ss_leveldb1.get(), "source"));

  // The maps were forked on the above call.
  EXPECT_NE(ss_leveldb_impl1->data_map(), ss_leveldb_impl2->data_map());

  EXPECT_CALL(listener_, OnDataMapDestruction(StdStringToUint8Vector("0")))
      .Times(1);
  EXPECT_CALL(listener_, OnDataMapDestruction(StdStringToUint8Vector("1")))
      .Times(1);

  ss_leveldb_impl1 = nullptr;
  ss_leveldb_impl2 = nullptr;
}

}  // namespace content
