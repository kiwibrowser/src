// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/quota_policy_channel_id_store.h"

#include <vector>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/cookies/cookie_util.h"
#include "net/ssl/ssl_client_cert_type.h"
#include "net/test/cert_test_util.h"
#include "net/test/channel_id_test_util.h"
#include "net/test/test_data_directory.h"
#include "sql/statement.h"
#include "storage/browser/test/mock_special_storage_policy.h"
#include "testing/gtest/include/gtest/gtest.h"

const base::FilePath::CharType kTestChannelIDFilename[] =
    FILE_PATH_LITERAL("ChannelID");

class QuotaPolicyChannelIDStoreTest : public testing::Test {
 public:
  void Load(std::vector<std::unique_ptr<net::DefaultChannelIDStore::ChannelID>>*
                channel_ids) {
    base::RunLoop run_loop;
    store_->Load(base::Bind(&QuotaPolicyChannelIDStoreTest::OnLoaded,
                            base::Unretained(this),
                            &run_loop));
    run_loop.Run();
    channel_ids->swap(channel_ids_);
    channel_ids_.clear();
  }

  void OnLoaded(
      base::RunLoop* run_loop,
      std::unique_ptr<
          std::vector<std::unique_ptr<net::DefaultChannelIDStore::ChannelID>>>
          channel_ids) {
    channel_ids_.swap(*channel_ids);
    run_loop->Quit();
  }

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    store_ = new QuotaPolicyChannelIDStore(
        temp_dir_.GetPath().Append(kTestChannelIDFilename),
        base::ThreadTaskRunnerHandle::Get(), NULL);
    std::vector<std::unique_ptr<net::DefaultChannelIDStore::ChannelID>>
        channel_ids;
    Load(&channel_ids);
    ASSERT_EQ(0u, channel_ids.size());
  }

  void TearDown() override {
    store_ = NULL;
    base::RunLoop().RunUntilIdle();
  }

  base::ScopedTempDir temp_dir_;
  scoped_refptr<QuotaPolicyChannelIDStore> store_;
  std::vector<std::unique_ptr<net::DefaultChannelIDStore::ChannelID>>
      channel_ids_;
  base::MessageLoop loop_;
};

// Test if data is stored as expected in the QuotaPolicy database.
TEST_F(QuotaPolicyChannelIDStoreTest, TestPersistence) {
  std::unique_ptr<crypto::ECPrivateKey> goog_key(
      crypto::ECPrivateKey::Create());
  std::unique_ptr<crypto::ECPrivateKey> foo_key(crypto::ECPrivateKey::Create());
  store_->AddChannelID(net::DefaultChannelIDStore::ChannelID(
      "google.com", base::Time::FromInternalValue(1), goog_key->Copy()));
  store_->AddChannelID(net::DefaultChannelIDStore::ChannelID(
      "foo.com", base::Time::FromInternalValue(3), foo_key->Copy()));

  std::vector<std::unique_ptr<net::DefaultChannelIDStore::ChannelID>>
      channel_ids;
  // Replace the store effectively destroying the current one and forcing it
  // to write its data to disk. Then we can see if after loading it again it
  // is still there.
  store_ = NULL;
  // Make sure we wait until the destructor has run.
  base::RunLoop().RunUntilIdle();
  store_ = new QuotaPolicyChannelIDStore(
      temp_dir_.GetPath().Append(kTestChannelIDFilename),
      base::ThreadTaskRunnerHandle::Get(), NULL);

  // Reload and test for persistence
  Load(&channel_ids);
  ASSERT_EQ(2U, channel_ids.size());
  net::DefaultChannelIDStore::ChannelID* goog_channel_id;
  net::DefaultChannelIDStore::ChannelID* foo_channel_id;
  if (channel_ids[0]->server_identifier() == "google.com") {
    goog_channel_id = channel_ids[0].get();
    foo_channel_id = channel_ids[1].get();
  } else {
    goog_channel_id = channel_ids[1].get();
    foo_channel_id = channel_ids[0].get();
  }
  ASSERT_EQ("google.com", goog_channel_id->server_identifier());
  EXPECT_TRUE(net::KeysEqual(goog_key.get(), goog_channel_id->key()));
  ASSERT_EQ(1, goog_channel_id->creation_time().ToInternalValue());
  ASSERT_EQ("foo.com", foo_channel_id->server_identifier());
  EXPECT_TRUE(net::KeysEqual(foo_key.get(), foo_channel_id->key()));
  ASSERT_EQ(3, foo_channel_id->creation_time().ToInternalValue());

  // Now delete the channel ID and check persistence again.
  store_->DeleteChannelID(*channel_ids[0]);
  store_->DeleteChannelID(*channel_ids[1]);
  store_ = NULL;
  // Make sure we wait until the destructor has run.
  base::RunLoop().RunUntilIdle();
  channel_ids.clear();
  store_ = new QuotaPolicyChannelIDStore(
      temp_dir_.GetPath().Append(kTestChannelIDFilename),
      base::ThreadTaskRunnerHandle::Get(), NULL);

  // Reload and check if the channel ID has been removed.
  Load(&channel_ids);
  ASSERT_EQ(0U, channel_ids.size());
}

// Test if data is stored as expected in the QuotaPolicy database.
TEST_F(QuotaPolicyChannelIDStoreTest, TestPolicy) {
  store_->AddChannelID(net::DefaultChannelIDStore::ChannelID(
      "google.com", base::Time::FromInternalValue(1),
      crypto::ECPrivateKey::Create()));
  store_->AddChannelID(net::DefaultChannelIDStore::ChannelID(
      "nonpersistent.com", base::Time::FromInternalValue(3),
      crypto::ECPrivateKey::Create()));

  std::vector<std::unique_ptr<net::DefaultChannelIDStore::ChannelID>>
      channel_ids;
  // Replace the store effectively destroying the current one and forcing it
  // to write its data to disk. Then we can see if after loading it again it
  // is still there.
  store_ = NULL;
  // Make sure we wait until the destructor has run.
  base::RunLoop().RunUntilIdle();
  // Specify storage policy that makes "nonpersistent.com" session only.
  scoped_refptr<content::MockSpecialStoragePolicy> storage_policy =
      new content::MockSpecialStoragePolicy();
  storage_policy->AddSessionOnly(
      net::cookie_util::CookieOriginToURL("nonpersistent.com", true));
  // Reload store, it should still have both channel IDs.
  store_ = new QuotaPolicyChannelIDStore(
      temp_dir_.GetPath().Append(kTestChannelIDFilename),
      base::ThreadTaskRunnerHandle::Get(), storage_policy);
  Load(&channel_ids);
  ASSERT_EQ(2U, channel_ids.size());

  // Add another two channel IDs before closing the store. Because additions are
  // delayed and committed to disk in batches, these will not be committed until
  // the store is destroyed, which is after the policy is applied. The pending
  // operation pruning logic should prevent the "nonpersistent.com" ID from
  // being committed to disk.
  store_->AddChannelID(net::DefaultChannelIDStore::ChannelID(
      "nonpersistent.com", base::Time::FromInternalValue(5),
      crypto::ECPrivateKey::Create()));
  store_->AddChannelID(net::DefaultChannelIDStore::ChannelID(
      "persistent.com", base::Time::FromInternalValue(7),
      crypto::ECPrivateKey::Create()));

  // Now close the store, and the nonpersistent.com channel IDs should be
  // deleted according to policy.
  store_ = NULL;
  // Make sure we wait until the destructor has run.
  base::RunLoop().RunUntilIdle();
  channel_ids.clear();
  store_ = new QuotaPolicyChannelIDStore(
      temp_dir_.GetPath().Append(kTestChannelIDFilename),
      base::ThreadTaskRunnerHandle::Get(), NULL);

  // Reload and check that the nonpersistent.com channel IDs have been removed.
  Load(&channel_ids);
  ASSERT_EQ(2U, channel_ids.size());
  for (const auto& id : channel_ids) {
    ASSERT_NE("nonpersistent.com", id->server_identifier());
  }
}
