// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/syncer_proto_util.h"

#include "base/compiler_specific.h"
#include "base/message_loop/message_loop.h"
#include "components/sync/base/cancelation_signal.h"
#include "components/sync/base/model_type_test_util.h"
#include "components/sync/engine_impl/cycle/sync_cycle_context.h"
#include "components/sync/protocol/bookmark_specifics.pb.h"
#include "components/sync/protocol/password_specifics.pb.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/protocol/sync_enums.pb.h"
#include "components/sync/syncable/directory.h"
#include "components/sync/test/engine/mock_connection_manager.h"
#include "components/sync/test/engine/test_directory_setter_upper.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

using sync_pb::ClientToServerMessage;
using sync_pb::CommitResponse_EntryResponse;
using sync_pb::SyncEntity;

namespace syncer {

// Builds a ClientToServerResponse with some data type ids, including
// invalid ones.  GetTypesToMigrate() should return only the valid
// model types.
TEST(SyncerProtoUtil, GetTypesToMigrate) {
  sync_pb::ClientToServerResponse response;
  response.add_migrated_data_type_id(
      GetSpecificsFieldNumberFromModelType(BOOKMARKS));
  response.add_migrated_data_type_id(
      GetSpecificsFieldNumberFromModelType(HISTORY_DELETE_DIRECTIVES));
  response.add_migrated_data_type_id(-1);
  EXPECT_EQ(ModelTypeSet(BOOKMARKS, HISTORY_DELETE_DIRECTIVES),
            GetTypesToMigrate(response));
}

// Builds a ClientToServerResponse_Error with some error data type
// ids, including invalid ones.  ConvertErrorPBToSyncProtocolError() should
// return a SyncProtocolError with only the valid model types.
TEST(SyncerProtoUtil, ConvertErrorPBToSyncProtocolError) {
  sync_pb::ClientToServerResponse_Error error_pb;
  error_pb.set_error_type(sync_pb::SyncEnums::THROTTLED);
  error_pb.add_error_data_type_ids(
      GetSpecificsFieldNumberFromModelType(BOOKMARKS));
  error_pb.add_error_data_type_ids(
      GetSpecificsFieldNumberFromModelType(HISTORY_DELETE_DIRECTIVES));
  error_pb.add_error_data_type_ids(-1);
  SyncProtocolError error = ConvertErrorPBToSyncProtocolError(error_pb);
  EXPECT_EQ(ModelTypeSet(BOOKMARKS, HISTORY_DELETE_DIRECTIVES),
            error.error_data_types);
}

// Tests NameFromSyncEntity and NameFromCommitEntryResponse when only the name
// field is provided.
TEST(SyncerProtoUtil, NameExtractionOneName) {
  SyncEntity one_name_entity;
  CommitResponse_EntryResponse one_name_response;

  const std::string one_name_string("Eggheadednesses");
  one_name_entity.set_name(one_name_string);
  one_name_response.set_name(one_name_string);

  const std::string name_a =
      SyncerProtoUtil::NameFromSyncEntity(one_name_entity);
  EXPECT_EQ(one_name_string, name_a);
}

TEST(SyncerProtoUtil, NameExtractionOneUniqueName) {
  SyncEntity one_name_entity;
  CommitResponse_EntryResponse one_name_response;

  const std::string one_name_string("Eggheadednesses");

  one_name_entity.set_non_unique_name(one_name_string);
  one_name_response.set_non_unique_name(one_name_string);

  const std::string name_a =
      SyncerProtoUtil::NameFromSyncEntity(one_name_entity);
  EXPECT_EQ(one_name_string, name_a);
}

// Tests NameFromSyncEntity and NameFromCommitEntryResponse when both the name
// field and the non_unique_name fields are provided.
// Should prioritize non_unique_name.
TEST(SyncerProtoUtil, NameExtractionTwoNames) {
  SyncEntity two_name_entity;
  CommitResponse_EntryResponse two_name_response;

  const std::string neuro("Neuroanatomists");
  const std::string oxyphen("Oxyphenbutazone");

  two_name_entity.set_name(oxyphen);
  two_name_entity.set_non_unique_name(neuro);

  two_name_response.set_name(oxyphen);
  two_name_response.set_non_unique_name(neuro);

  const std::string name_a =
      SyncerProtoUtil::NameFromSyncEntity(two_name_entity);
  EXPECT_EQ(neuro, name_a);
}

class SyncerProtoUtilTest : public testing::Test {
 public:
  void SetUp() override { dir_maker_.SetUp(); }

  void TearDown() override { dir_maker_.TearDown(); }

  syncable::Directory* directory() { return dir_maker_.directory(); }

  // Helper function to call GetProtocolErrorFromResponse. Allows not adding
  // individual tests as friends to SyncerProtoUtil.
  static SyncProtocolError CallGetProtocolErrorFromResponse(
      const sync_pb::ClientToServerResponse& response,
      syncable::Directory* directory) {
    return SyncerProtoUtil::GetProtocolErrorFromResponse(response, directory);
  }

 protected:
  base::MessageLoop message_loop_;
  TestDirectorySetterUpper dir_maker_;
};

TEST_F(SyncerProtoUtilTest, VerifyResponseBirthday) {
  // Both sides empty
  EXPECT_TRUE(directory()->store_birthday().empty());
  sync_pb::ClientToServerResponse response;
  SyncProtocolError sync_protocol_error;
  response.set_error_code(sync_pb::SyncEnums::SUCCESS);

  sync_protocol_error = CallGetProtocolErrorFromResponse(response, directory());
  EXPECT_EQ(NOT_MY_BIRTHDAY, sync_protocol_error.error_type);
  EXPECT_EQ(DISABLE_SYNC_ON_CLIENT, sync_protocol_error.action);

  // Remote set, local empty
  response.set_store_birthday("flan");
  sync_protocol_error = CallGetProtocolErrorFromResponse(response, directory());
  EXPECT_EQ(SYNC_SUCCESS, sync_protocol_error.error_type);
  EXPECT_EQ(UNKNOWN_ACTION, sync_protocol_error.action);
  EXPECT_EQ(directory()->store_birthday(), "flan");

  // Remote empty, local set.
  response.clear_store_birthday();
  sync_protocol_error = CallGetProtocolErrorFromResponse(response, directory());
  EXPECT_EQ(SYNC_SUCCESS, sync_protocol_error.error_type);
  EXPECT_EQ(UNKNOWN_ACTION, sync_protocol_error.action);
  EXPECT_EQ(directory()->store_birthday(), "flan");

  // Doesn't match
  response.set_store_birthday("meat");
  response.set_error_code(sync_pb::SyncEnums::NOT_MY_BIRTHDAY);
  sync_protocol_error = CallGetProtocolErrorFromResponse(response, directory());
  EXPECT_EQ(NOT_MY_BIRTHDAY, sync_protocol_error.error_type);
  EXPECT_EQ(DISABLE_SYNC_ON_CLIENT, sync_protocol_error.action);

  // Doesn't match. CLIENT_DATA_OBSOLETE error is set.
  response.set_error_code(sync_pb::SyncEnums::CLIENT_DATA_OBSOLETE);
  sync_protocol_error = CallGetProtocolErrorFromResponse(response, directory());
  EXPECT_EQ(CLIENT_DATA_OBSOLETE, sync_protocol_error.error_type);
  EXPECT_EQ(RESET_LOCAL_SYNC_DATA, sync_protocol_error.action);
}

TEST_F(SyncerProtoUtilTest, VerifyDisabledByAdmin) {
  // No error code
  sync_pb::ClientToServerResponse response;
  SyncProtocolError sync_protocol_error;
  directory()->set_store_birthday("flan");
  response.set_error_code(sync_pb::SyncEnums::SUCCESS);

  sync_protocol_error = CallGetProtocolErrorFromResponse(response, directory());
  EXPECT_EQ(SYNC_SUCCESS, sync_protocol_error.error_type);
  EXPECT_EQ(UNKNOWN_ACTION, sync_protocol_error.action);

  // Has error code, but not disabled
  response.set_error_code(sync_pb::SyncEnums::NOT_MY_BIRTHDAY);
  sync_protocol_error = CallGetProtocolErrorFromResponse(response, directory());
  EXPECT_EQ(NOT_MY_BIRTHDAY, sync_protocol_error.error_type);
  EXPECT_NE(UNKNOWN_ACTION, sync_protocol_error.action);

  // Has error code, and is disabled by admin
  response.set_error_code(sync_pb::SyncEnums::DISABLED_BY_ADMIN);
  sync_protocol_error = CallGetProtocolErrorFromResponse(response, directory());
  EXPECT_EQ(DISABLED_BY_ADMIN, sync_protocol_error.error_type);
  EXPECT_EQ(STOP_SYNC_FOR_DISABLED_ACCOUNT, sync_protocol_error.action);
}

TEST_F(SyncerProtoUtilTest, AddRequestBirthday) {
  EXPECT_TRUE(directory()->store_birthday().empty());
  ClientToServerMessage msg;
  SyncerProtoUtil::AddRequestBirthday(directory(), &msg);
  EXPECT_FALSE(msg.has_store_birthday());

  directory()->set_store_birthday("meat");
  SyncerProtoUtil::AddRequestBirthday(directory(), &msg);
  EXPECT_EQ(msg.store_birthday(), "meat");
}

class DummyConnectionManager : public ServerConnectionManager {
 public:
  explicit DummyConnectionManager(CancelationSignal* signal)
      : ServerConnectionManager("unused", 0, false, signal),
        send_error_(false) {}

  bool PostBufferWithCachedAuth(PostBufferParams* params) override {
    if (send_error_) {
      return false;
    }

    sync_pb::ClientToServerResponse response;
    response.SerializeToString(&params->buffer_out);

    return true;
  }

  void set_send_error(bool send) { send_error_ = send; }

 private:
  bool send_error_;
};

TEST_F(SyncerProtoUtilTest, PostAndProcessHeaders) {
  CancelationSignal signal;
  DummyConnectionManager dcm(&signal);
  ClientToServerMessage msg;
  SyncerProtoUtil::SetProtocolVersion(&msg);
  msg.set_share("required");
  msg.set_message_contents(ClientToServerMessage::GET_UPDATES);
  sync_pb::ClientToServerResponse response;

  dcm.set_send_error(true);
  EXPECT_FALSE(
      SyncerProtoUtil::PostAndProcessHeaders(&dcm, nullptr, msg, &response));

  dcm.set_send_error(false);
  EXPECT_TRUE(
      SyncerProtoUtil::PostAndProcessHeaders(&dcm, nullptr, msg, &response));
}

}  // namespace syncer
