// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/extension_localization_peer.h"

#include <stdint.h>

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "content/public/renderer/fixed_received_data.h"
#include "extensions/common/message_bundle.h"
#include "ipc/ipc_sender.h"
#include "ipc/ipc_sync_message.h"
#include "net/base/net_errors.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request_status.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::StrEq;
using testing::Return;

static const char* const kExtensionUrl_1 =
    "chrome-extension://some_id/popup.css";

static const char* const kExtensionUrl_2 =
    "chrome-extension://some_id2/popup.css";

static const char* const kExtensionUrl_3 =
    "chrome-extension://some_id3/popup.css";

void MessageDeleter(IPC::Message* message) {
  delete message;
}

class MockIpcMessageSender : public IPC::Sender {
 public:
  MockIpcMessageSender() {
    ON_CALL(*this, Send(_))
        .WillByDefault(DoAll(Invoke(MessageDeleter), Return(true)));
  }

  ~MockIpcMessageSender() override {}

  MOCK_METHOD1(Send, bool(IPC::Message* message));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockIpcMessageSender);
};

class MockRequestPeer : public content::RequestPeer {
 public:
  MockRequestPeer() {}
  ~MockRequestPeer() override {}

  MOCK_METHOD2(OnUploadProgress, void(uint64_t position, uint64_t size));
  MOCK_METHOD2(OnReceivedRedirect,
               bool(const net::RedirectInfo& redirect_info,
                    const network::ResourceResponseInfo& info));
  MOCK_METHOD1(OnReceivedResponse,
               void(const network::ResourceResponseInfo& info));
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override {}
  MOCK_METHOD2(OnDownloadedData, void(int len, int encoded_data_length));
  void OnReceivedData(
      std::unique_ptr<RequestPeer::ReceivedData> data) override {
    OnReceivedDataInternal(data->payload(), data->length());
  }
  MOCK_METHOD2(OnReceivedDataInternal, void(const char* data, int data_length));
  MOCK_METHOD1(OnTransferSizeUpdated, void(int transfer_size_diff));
  MOCK_METHOD1(OnCompletedRequest,
               void(const network::URLLoaderCompletionStatus& status));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockRequestPeer);
};

}  // namespace

class ExtensionLocalizationPeerTest : public testing::Test {
 protected:
  void SetUp() override {
    sender_.reset(new MockIpcMessageSender());
  }

  void SetUpExtensionLocalizationPeer(const std::string& mime_type,
                                      const GURL& request_url) {
    std::unique_ptr<MockRequestPeer> original_peer(new MockRequestPeer());
    original_peer_ = original_peer.get();
    filter_peer_ = ExtensionLocalizationPeer::CreateExtensionLocalizationPeer(
        std::move(original_peer), sender_.get(), mime_type, request_url);
    ASSERT_TRUE(filter_peer_);
  }

  std::string GetData() {
    return static_cast<ExtensionLocalizationPeer*>(filter_peer_.get())->data_;
  }

  void SetData(const std::string& data) {
    static_cast<ExtensionLocalizationPeer*>(filter_peer_.get())->data_ = data;
  }

  std::unique_ptr<MockIpcMessageSender> sender_;
  MockRequestPeer* original_peer_;
  std::unique_ptr<content::RequestPeer> filter_peer_;
};

TEST_F(ExtensionLocalizationPeerTest, CreateWithWrongMimeType) {
  std::unique_ptr<content::RequestPeer> peer =
      ExtensionLocalizationPeer::CreateExtensionLocalizationPeer(
          nullptr, sender_.get(), "text/html", GURL(kExtensionUrl_1));
  EXPECT_EQ(nullptr, peer);
}

TEST_F(ExtensionLocalizationPeerTest, CreateWithValidInput) {
  SetUpExtensionLocalizationPeer("text/css", GURL(kExtensionUrl_1));
  EXPECT_TRUE(NULL != filter_peer_.get());
}

TEST_F(ExtensionLocalizationPeerTest, OnReceivedData) {
  SetUpExtensionLocalizationPeer("text/css", GURL(kExtensionUrl_1));
  EXPECT_TRUE(GetData().empty());

  const std::string data_chunk("12345");
  filter_peer_->OnReceivedData(std::make_unique<content::FixedReceivedData>(
      data_chunk.data(), data_chunk.length()));

  EXPECT_EQ(data_chunk, GetData());

  filter_peer_->OnReceivedData(std::make_unique<content::FixedReceivedData>(
      data_chunk.data(), data_chunk.length()));
  EXPECT_EQ(data_chunk + data_chunk, GetData());
}

MATCHER_P(IsURLRequestEqual, status, "") { return arg.status() == status; }

TEST_F(ExtensionLocalizationPeerTest, OnCompletedRequestBadURLRequestStatus) {
  SetUpExtensionLocalizationPeer("text/css", GURL(kExtensionUrl_1));

  EXPECT_CALL(*original_peer_, OnReceivedResponse(_));
  network::URLLoaderCompletionStatus status(net::ERR_ABORTED);
  EXPECT_CALL(*original_peer_, OnCompletedRequest(status));

  filter_peer_->OnCompletedRequest(status);
}

TEST_F(ExtensionLocalizationPeerTest, OnCompletedRequestEmptyData) {
  SetUpExtensionLocalizationPeer("text/css", GURL(kExtensionUrl_1));

  EXPECT_CALL(*original_peer_, OnReceivedDataInternal(_, _)).Times(0);
  EXPECT_CALL(*sender_, Send(_)).Times(0);

  EXPECT_CALL(*original_peer_, OnReceivedResponse(_));
  network::URLLoaderCompletionStatus status(net::OK);
  EXPECT_CALL(*original_peer_, OnCompletedRequest(status));

  filter_peer_->OnCompletedRequest(status);
}

TEST_F(ExtensionLocalizationPeerTest, OnCompletedRequestNoCatalogs) {
  SetUpExtensionLocalizationPeer("text/css", GURL(kExtensionUrl_1));

  SetData("some text");

  EXPECT_CALL(*sender_, Send(_));

  std::string data = GetData();
  EXPECT_CALL(*original_peer_, OnReceivedResponse(_)).Times(1);
  EXPECT_CALL(*original_peer_,
              OnReceivedDataInternal(StrEq(data.c_str()), data.length()))
      .Times(1);
  network::URLLoaderCompletionStatus status(net::OK);
  EXPECT_CALL(*original_peer_, OnCompletedRequest(status)).Times(1);

  filter_peer_->OnCompletedRequest(status);

  // Test if Send gets called again (it shouldn't be) when first call returned
  // an empty dictionary.
  SetUpExtensionLocalizationPeer("text/css", GURL(kExtensionUrl_1));
  EXPECT_CALL(*original_peer_, OnReceivedResponse(_)).Times(1);
  EXPECT_CALL(*original_peer_,
              OnReceivedDataInternal(StrEq(data.c_str()), data.length()))
      .Times(1);
  EXPECT_CALL(*original_peer_, OnCompletedRequest(status)).Times(1);
  SetData("some text");
  filter_peer_->OnCompletedRequest(status);
}

TEST_F(ExtensionLocalizationPeerTest, OnCompletedRequestWithCatalogs) {
  SetUpExtensionLocalizationPeer("text/css", GURL(kExtensionUrl_2));

  extensions::L10nMessagesMap messages;
  messages.insert(std::make_pair("text", "new text"));
  extensions::ExtensionToL10nMessagesMap& l10n_messages_map =
      *extensions::GetExtensionToL10nMessagesMap();
  l10n_messages_map["some_id2"] = messages;

  SetData("some __MSG_text__");

  // We already have messages in memory, Send will be skipped.
  EXPECT_CALL(*sender_, Send(_)).Times(0);

  // __MSG_text__ gets replaced with "new text".
  std::string data("some new text");
  EXPECT_CALL(*original_peer_, OnReceivedResponse(_));
  EXPECT_CALL(*original_peer_,
              OnReceivedDataInternal(StrEq(data.c_str()), data.length()));

  network::URLLoaderCompletionStatus status(net::OK);
  EXPECT_CALL(*original_peer_, OnCompletedRequest(status));

  filter_peer_->OnCompletedRequest(status);
}

TEST_F(ExtensionLocalizationPeerTest, OnCompletedRequestReplaceMessagesFails) {
  SetUpExtensionLocalizationPeer("text/css", GURL(kExtensionUrl_3));

  extensions::L10nMessagesMap messages;
  messages.insert(std::make_pair("text", "new text"));
  extensions::ExtensionToL10nMessagesMap& l10n_messages_map =
      *extensions::GetExtensionToL10nMessagesMap();
  l10n_messages_map["some_id3"] = messages;

  std::string message("some __MSG_missing_message__");
  SetData(message);

  // We already have messages in memory, Send will be skipped.
  EXPECT_CALL(*sender_, Send(_)).Times(0);

  // __MSG_missing_message__ is missing, so message stays the same.
  EXPECT_CALL(*original_peer_, OnReceivedResponse(_));
  EXPECT_CALL(*original_peer_,
              OnReceivedDataInternal(StrEq(message.c_str()), message.length()));

  network::URLLoaderCompletionStatus status(net::OK);
  EXPECT_CALL(*original_peer_, OnCompletedRequest(status));

  filter_peer_->OnCompletedRequest(status);
}
