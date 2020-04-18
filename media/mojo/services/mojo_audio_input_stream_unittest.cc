// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_audio_input_stream.h"

#include <utility>

#include "base/memory/read_only_shared_memory_region.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/sync_socket.h"
#include "media/audio/audio_input_controller.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

namespace {

const double kNewVolume = 0.618;
// Not actually used, but sent from the AudioInputDelegate.
const int kStreamId = 0;
const int kShmemSize = 100;
// const bool kInitiallyMuted = true;
const bool kInitiallyNotMuted = true;

using testing::_;
using testing::Mock;
using testing::NotNull;
using testing::Return;
using testing::SaveArg;
using testing::StrictMock;
using testing::Test;
using AudioInputStream = mojom::AudioInputStream;
using AudioInputStreamPtr = mojo::InterfacePtr<AudioInputStream>;

class TestCancelableSyncSocket : public base::CancelableSyncSocket {
 public:
  TestCancelableSyncSocket() = default;

  void ExpectOwnershipTransfer() { expect_ownership_transfer_ = true; }

  ~TestCancelableSyncSocket() override {
    // When the handle is sent over mojo, mojo takes ownership over it and
    // closes it. We have to make sure we do not also retain the handle in the
    // sync socket, as the sync socket closes the handle on destruction.
    if (expect_ownership_transfer_)
      EXPECT_EQ(handle(), kInvalidHandle);
  }

 private:
  bool expect_ownership_transfer_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestCancelableSyncSocket);
};

class MockDelegate : public AudioInputDelegate {
 public:
  MockDelegate() = default;
  ~MockDelegate() override = default;

  MOCK_METHOD0(GetStreamId, int());
  MOCK_METHOD0(OnRecordStream, void());
  MOCK_METHOD1(OnSetVolume, void(double));
  MOCK_METHOD1(OnSetOutputDeviceForAec, void(const std::string&));
};

class MockDelegateFactory {
 public:
  void PrepareDelegateForCreation(
      std::unique_ptr<AudioInputDelegate> delegate) {
    ASSERT_EQ(nullptr, delegate_);
    delegate_.swap(delegate);
  }

  std::unique_ptr<AudioInputDelegate> CreateDelegate(
      AudioInputDelegate::EventHandler* handler) {
    MockCreateDelegate(handler);
    EXPECT_NE(nullptr, delegate_);
    return std::move(delegate_);
  }

  MOCK_METHOD1(MockCreateDelegate, void(AudioInputDelegate::EventHandler*));

 private:
  std::unique_ptr<AudioInputDelegate> delegate_;
};

class MockDeleter {
 public:
  MOCK_METHOD0(Finished, void());
};

class MockClient : public mojom::AudioInputStreamClient {
 public:
  MockClient() = default;

  void Initialized(mojom::AudioDataPipePtr data_pipe, bool initially_muted) {
    ASSERT_TRUE(data_pipe->shared_memory.is_valid());
    ASSERT_TRUE(data_pipe->socket.is_valid());

    base::PlatformFile fd;
    mojo::UnwrapPlatformFile(std::move(data_pipe->socket), &fd);
    socket_ = std::make_unique<base::CancelableSyncSocket>(fd);
    EXPECT_NE(socket_->handle(), base::CancelableSyncSocket::kInvalidHandle);

    region_ = mojo::UnwrapReadOnlySharedMemoryRegion(
        std::move(data_pipe->shared_memory));
    EXPECT_TRUE(region_.IsValid());

    GotNotification(initially_muted);
  }

  MOCK_METHOD1(GotNotification, void(bool initially_muted));

  MOCK_METHOD1(OnMutedStateChanged, void(bool));

  MOCK_METHOD0(OnError, void());

 private:
  base::ReadOnlySharedMemoryRegion region_;
  std::unique_ptr<base::CancelableSyncSocket> socket_;

  DISALLOW_COPY_AND_ASSIGN(MockClient);
};

std::unique_ptr<AudioInputDelegate> CreateNoDelegate(
    AudioInputDelegate::EventHandler* event_handler) {
  return nullptr;
}

void NotCalled(mojom::AudioDataPipePtr data_pipe, bool initially_muted) {
  EXPECT_TRUE(false) << "The StreamCreated callback was called despite the "
                        "test expecting it not to.";
}

}  // namespace

class MojoAudioInputStreamTest : public Test {
 public:
  MojoAudioInputStreamTest()
      : foreign_socket_(std::make_unique<TestCancelableSyncSocket>()),
        client_binding_(&client_, mojo::MakeRequest(&client_ptr_)) {}

  AudioInputStreamPtr CreateAudioInput() {
    AudioInputStreamPtr p;
    ExpectDelegateCreation();
    impl_ = std::make_unique<MojoAudioInputStream>(
        mojo::MakeRequest(&p), std::move(client_ptr_),
        base::BindOnce(&MockDelegateFactory::CreateDelegate,
                       base::Unretained(&mock_delegate_factory_)),
        base::BindOnce(&MockClient::Initialized, base::Unretained(&client_)),
        base::BindOnce(&MockDeleter::Finished, base::Unretained(&deleter_)));
    EXPECT_TRUE(p.is_bound());
    return p;
  }

 protected:
  void ExpectDelegateCreation() {
    delegate_ = new StrictMock<MockDelegate>();
    mock_delegate_factory_.PrepareDelegateForCreation(
        base::WrapUnique(delegate_));
    EXPECT_TRUE(
        base::CancelableSyncSocket::CreatePair(&local_, foreign_socket_.get()));
    mem_ = base::ReadOnlySharedMemoryRegion::Create(kShmemSize).region;
    EXPECT_TRUE(mem_.IsValid());
    EXPECT_CALL(mock_delegate_factory_, MockCreateDelegate(NotNull()))
        .WillOnce(SaveArg<0>(&delegate_event_handler_));
  }

  base::MessageLoop loop_;
  base::CancelableSyncSocket local_;
  std::unique_ptr<TestCancelableSyncSocket> foreign_socket_;
  base::ReadOnlySharedMemoryRegion mem_;
  StrictMock<MockDelegate>* delegate_ = nullptr;
  AudioInputDelegate::EventHandler* delegate_event_handler_ = nullptr;
  StrictMock<MockDelegateFactory> mock_delegate_factory_;
  StrictMock<MockDeleter> deleter_;
  StrictMock<MockClient> client_;
  media::mojom::AudioInputStreamClientPtr client_ptr_;
  mojo::Binding<media::mojom::AudioInputStreamClient> client_binding_;
  std::unique_ptr<MojoAudioInputStream> impl_;

  DISALLOW_COPY_AND_ASSIGN(MojoAudioInputStreamTest);
};

TEST_F(MojoAudioInputStreamTest, NoDelegate_SignalsError) {
  bool deleter_called = false;
  EXPECT_CALL(client_, OnError());
  mojom::AudioInputStreamPtr stream_ptr;
  MojoAudioInputStream stream(
      mojo::MakeRequest(&stream_ptr), std::move(client_ptr_),
      base::BindOnce(&CreateNoDelegate), base::BindOnce(&NotCalled),
      base::BindOnce([](bool* p) { *p = true; }, &deleter_called));
  EXPECT_FALSE(deleter_called)
      << "Stream shouldn't call the deleter from its constructor.";
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(deleter_called);
}

TEST_F(MojoAudioInputStreamTest, Record_Records) {
  AudioInputStreamPtr audio_input_ptr = CreateAudioInput();
  EXPECT_CALL(*delegate_, OnRecordStream());

  audio_input_ptr->Record();
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoAudioInputStreamTest, SetVolume_SetsVolume) {
  AudioInputStreamPtr audio_input_ptr = CreateAudioInput();
  EXPECT_CALL(*delegate_, OnSetVolume(kNewVolume));

  audio_input_ptr->SetVolume(kNewVolume);
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoAudioInputStreamTest, DestructWithCallPending_Safe) {
  AudioInputStreamPtr audio_input_ptr = CreateAudioInput();
  EXPECT_CALL(client_, GotNotification(kInitiallyNotMuted));
  base::RunLoop().RunUntilIdle();

  ASSERT_NE(nullptr, delegate_event_handler_);
  foreign_socket_->ExpectOwnershipTransfer();
  delegate_event_handler_->OnStreamCreated(kStreamId, std::move(mem_),
                                           std::move(foreign_socket_),
                                           kInitiallyNotMuted);
  audio_input_ptr->Record();
  impl_.reset();
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoAudioInputStreamTest, Created_NotifiesClient) {
  AudioInputStreamPtr audio_input_ptr = CreateAudioInput();
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(client_, GotNotification(kInitiallyNotMuted));

  ASSERT_NE(nullptr, delegate_event_handler_);
  foreign_socket_->ExpectOwnershipTransfer();
  delegate_event_handler_->OnStreamCreated(kStreamId, std::move(mem_),
                                           std::move(foreign_socket_),
                                           kInitiallyNotMuted);

  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoAudioInputStreamTest, SetVolumeTooLarge_Error) {
  AudioInputStreamPtr audio_input_ptr = CreateAudioInput();
  EXPECT_CALL(deleter_, Finished());
  EXPECT_CALL(client_, OnError());

  audio_input_ptr->SetVolume(15);
  base::RunLoop().RunUntilIdle();
  Mock::VerifyAndClear(&deleter_);
}

TEST_F(MojoAudioInputStreamTest, SetVolumeNegative_Error) {
  AudioInputStreamPtr audio_input_ptr = CreateAudioInput();
  EXPECT_CALL(deleter_, Finished());
  EXPECT_CALL(client_, OnError());

  audio_input_ptr->SetVolume(-0.5);
  base::RunLoop().RunUntilIdle();
  Mock::VerifyAndClear(&deleter_);
}

TEST_F(MojoAudioInputStreamTest, DelegateErrorBeforeCreated_PropagatesError) {
  AudioInputStreamPtr audio_input_ptr = CreateAudioInput();
  EXPECT_CALL(deleter_, Finished());
  EXPECT_CALL(client_, OnError());

  ASSERT_NE(nullptr, delegate_event_handler_);
  delegate_event_handler_->OnStreamError(kStreamId);

  base::RunLoop().RunUntilIdle();
  Mock::VerifyAndClear(&deleter_);
}

TEST_F(MojoAudioInputStreamTest, DelegateErrorAfterCreated_PropagatesError) {
  AudioInputStreamPtr audio_input_ptr = CreateAudioInput();
  EXPECT_CALL(client_, GotNotification(kInitiallyNotMuted));
  EXPECT_CALL(deleter_, Finished());
  EXPECT_CALL(client_, OnError());
  base::RunLoop().RunUntilIdle();

  ASSERT_NE(nullptr, delegate_event_handler_);
  foreign_socket_->ExpectOwnershipTransfer();
  delegate_event_handler_->OnStreamCreated(kStreamId, std::move(mem_),
                                           std::move(foreign_socket_),
                                           kInitiallyNotMuted);
  delegate_event_handler_->OnStreamError(kStreamId);

  base::RunLoop().RunUntilIdle();
  Mock::VerifyAndClear(&deleter_);
}

TEST_F(MojoAudioInputStreamTest, RemoteEndGone_Error) {
  AudioInputStreamPtr audio_input_ptr = CreateAudioInput();
  EXPECT_CALL(deleter_, Finished());
  audio_input_ptr.reset();
  base::RunLoop().RunUntilIdle();
  Mock::VerifyAndClear(&deleter_);
}

}  // namespace media
