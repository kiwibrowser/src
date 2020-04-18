// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_audio_output_stream.h"

#include <utility>

#include "base/memory/shared_memory.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/sync_socket.h"
#include "media/audio/audio_output_controller.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

namespace {

const double kNewVolume = 0.618;
// Not actually used, but sent from the AudioOutputDelegate.
const int kStreamId = 0;
const int kShmemSize = 100;

using testing::_;
using testing::Mock;
using testing::NotNull;
using testing::Return;
using testing::SaveArg;
using testing::StrictMock;
using testing::Test;
using AudioOutputStream = mojom::AudioOutputStream;
using AudioOutputStreamPtr = mojo::InterfacePtr<AudioOutputStream>;

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

class MockDelegate : public AudioOutputDelegate {
 public:
  MockDelegate() = default;
  ~MockDelegate() override = default;

  MOCK_METHOD0(GetStreamId, int());
  MOCK_METHOD0(OnPlayStream, void());
  MOCK_METHOD0(OnPauseStream, void());
  MOCK_METHOD1(OnSetVolume, void(double));
};

class MockDelegateFactory {
 public:
  void PrepareDelegateForCreation(
      std::unique_ptr<AudioOutputDelegate> delegate) {
    ASSERT_EQ(nullptr, delegate_);
    delegate_.swap(delegate);
  }

  std::unique_ptr<AudioOutputDelegate> CreateDelegate(
      AudioOutputDelegate::EventHandler* handler) {
    MockCreateDelegate(handler);
    EXPECT_NE(nullptr, delegate_);
    return std::move(delegate_);
  }

  MOCK_METHOD1(MockCreateDelegate, void(AudioOutputDelegate::EventHandler*));

 private:
  std::unique_ptr<AudioOutputDelegate> delegate_;
};

class MockDeleter {
 public:
  MOCK_METHOD1(Finished, void(bool));
};

class MockClient {
 public:
  MockClient() = default;

  void Initialize(mojom::AudioDataPipePtr data_pipe) {
    ASSERT_TRUE(data_pipe->shared_memory.is_valid());
    ASSERT_TRUE(data_pipe->socket.is_valid());

    base::PlatformFile fd;
    mojo::UnwrapPlatformFile(std::move(data_pipe->socket), &fd);
    socket_ = std::make_unique<base::CancelableSyncSocket>(fd);
    EXPECT_NE(socket_->handle(), base::CancelableSyncSocket::kInvalidHandle);

    size_t memory_length;
    base::SharedMemoryHandle shmem_handle;
    mojo::UnwrappedSharedMemoryHandleProtection protection;
    EXPECT_EQ(mojo::UnwrapSharedMemoryHandle(
                  std::move(data_pipe->shared_memory), &shmem_handle,
                  &memory_length, &protection),
              MOJO_RESULT_OK);
    EXPECT_EQ(protection,
              mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);
    buffer_ = std::make_unique<base::SharedMemory>(shmem_handle,
                                                   false /* read_only */);

    GotNotification();
  }

  MOCK_METHOD0(GotNotification, void());

 private:
  std::unique_ptr<base::SharedMemory> buffer_;
  std::unique_ptr<base::CancelableSyncSocket> socket_;
};

std::unique_ptr<AudioOutputDelegate> CreateNoDelegate(
    AudioOutputDelegate::EventHandler* event_handler) {
  return nullptr;
}

void NotCalled(mojom::AudioOutputStreamPtr, mojom::AudioDataPipePtr) {
  ADD_FAILURE() << "The StreamCreated callback was called despite the test "
                   "expecting it not to.";
}

}  // namespace

class MojoAudioOutputStreamTest : public Test {
 public:
  MojoAudioOutputStreamTest()
      : foreign_socket_(std::make_unique<TestCancelableSyncSocket>()) {}

  AudioOutputStreamPtr CreateAudioOutput() {
    mojom::AudioOutputStreamPtr p;
    pending_stream_request_ = mojo::MakeRequest(&p);
    ExpectDelegateCreation();
    impl_ = std::make_unique<MojoAudioOutputStream>(
        base::BindOnce(&MockDelegateFactory::CreateDelegate,
                       base::Unretained(&mock_delegate_factory_)),
        base::BindOnce(&MojoAudioOutputStreamTest::CreatedStream,
                       base::Unretained(this)),
        base::BindOnce(&MockDeleter::Finished, base::Unretained(&deleter_)));
    return p;
  }

 protected:
  void CreatedStream(mojom::AudioOutputStreamPtr stream,
                     mojom::AudioDataPipePtr data_pipe) {
    EXPECT_EQ(mojo::FuseMessagePipes(pending_stream_request_.PassMessagePipe(),
                                     stream.PassInterface().PassHandle()),
              MOJO_RESULT_OK);
    client_.Initialize(std::move(data_pipe));
  }

  void ExpectDelegateCreation() {
    delegate_ = new StrictMock<MockDelegate>();
    mock_delegate_factory_.PrepareDelegateForCreation(
        base::WrapUnique(delegate_));
    EXPECT_TRUE(
        base::CancelableSyncSocket::CreatePair(&local_, foreign_socket_.get()));
    mem_ = base::UnsafeSharedMemoryRegion::Create(kShmemSize);
    EXPECT_TRUE(mem_.IsValid());
    EXPECT_CALL(mock_delegate_factory_, MockCreateDelegate(NotNull()))
        .WillOnce(SaveArg<0>(&delegate_event_handler_));
  }

  base::MessageLoop loop_;
  base::CancelableSyncSocket local_;
  std::unique_ptr<TestCancelableSyncSocket> foreign_socket_;
  base::UnsafeSharedMemoryRegion mem_;
  StrictMock<MockDelegate>* delegate_ = nullptr;
  AudioOutputDelegate::EventHandler* delegate_event_handler_ = nullptr;
  StrictMock<MockDelegateFactory> mock_delegate_factory_;
  StrictMock<MockDeleter> deleter_;
  StrictMock<MockClient> client_;
  mojom::AudioOutputStreamRequest pending_stream_request_;
  std::unique_ptr<MojoAudioOutputStream> impl_;
};

TEST_F(MojoAudioOutputStreamTest, NoDelegate_SignalsError) {
  mojom::AudioOutputStreamPtr stream_ptr;
  MojoAudioOutputStream stream(
      base::BindOnce(&CreateNoDelegate), base::BindOnce(&NotCalled),
      base::BindOnce(&MockDeleter::Finished, base::Unretained(&deleter_)));
  EXPECT_CALL(deleter_, Finished(true));
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoAudioOutputStreamTest, Play_Plays) {
  AudioOutputStreamPtr audio_output_ptr = CreateAudioOutput();

  EXPECT_CALL(client_, GotNotification());
  EXPECT_CALL(*delegate_, OnPlayStream());

  delegate_event_handler_->OnStreamCreated(kStreamId, std::move(mem_),
                                           std::move(foreign_socket_));
  audio_output_ptr->Play();
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoAudioOutputStreamTest, Pause_Pauses) {
  AudioOutputStreamPtr audio_output_ptr = CreateAudioOutput();

  EXPECT_CALL(client_, GotNotification());
  EXPECT_CALL(*delegate_, OnPauseStream());

  delegate_event_handler_->OnStreamCreated(kStreamId, std::move(mem_),
                                           std::move(foreign_socket_));
  audio_output_ptr->Pause();
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoAudioOutputStreamTest, SetVolume_SetsVolume) {
  AudioOutputStreamPtr audio_output_ptr = CreateAudioOutput();

  EXPECT_CALL(client_, GotNotification());
  EXPECT_CALL(*delegate_, OnSetVolume(kNewVolume));

  delegate_event_handler_->OnStreamCreated(kStreamId, std::move(mem_),
                                           std::move(foreign_socket_));
  audio_output_ptr->SetVolume(kNewVolume);
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoAudioOutputStreamTest, DestructWithCallPending_Safe) {
  AudioOutputStreamPtr audio_output_ptr = CreateAudioOutput();
  EXPECT_CALL(client_, GotNotification());
  base::RunLoop().RunUntilIdle();

  ASSERT_NE(nullptr, delegate_event_handler_);
  foreign_socket_->ExpectOwnershipTransfer();
  delegate_event_handler_->OnStreamCreated(kStreamId, std::move(mem_),
                                           std::move(foreign_socket_));
  audio_output_ptr->Play();
  impl_.reset();
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoAudioOutputStreamTest, Created_NotifiesClient) {
  AudioOutputStreamPtr audio_output_ptr = CreateAudioOutput();
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(client_, GotNotification());

  ASSERT_NE(nullptr, delegate_event_handler_);
  foreign_socket_->ExpectOwnershipTransfer();
  delegate_event_handler_->OnStreamCreated(kStreamId, std::move(mem_),
                                           std::move(foreign_socket_));

  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoAudioOutputStreamTest, SetVolumeTooLarge_Error) {
  AudioOutputStreamPtr audio_output_ptr = CreateAudioOutput();
  EXPECT_CALL(deleter_, Finished(true));
  EXPECT_CALL(client_, GotNotification());

  delegate_event_handler_->OnStreamCreated(kStreamId, std::move(mem_),
                                           std::move(foreign_socket_));
  audio_output_ptr->SetVolume(15);
  base::RunLoop().RunUntilIdle();
  Mock::VerifyAndClear(&deleter_);
}

TEST_F(MojoAudioOutputStreamTest, SetVolumeNegative_Error) {
  AudioOutputStreamPtr audio_output_ptr = CreateAudioOutput();
  EXPECT_CALL(deleter_, Finished(true));
  EXPECT_CALL(client_, GotNotification());

  delegate_event_handler_->OnStreamCreated(kStreamId, std::move(mem_),
                                           std::move(foreign_socket_));
  audio_output_ptr->SetVolume(-0.5);
  base::RunLoop().RunUntilIdle();
  Mock::VerifyAndClear(&deleter_);
}

TEST_F(MojoAudioOutputStreamTest, DelegateErrorBeforeCreated_PropagatesError) {
  AudioOutputStreamPtr audio_output_ptr = CreateAudioOutput();
  EXPECT_CALL(deleter_, Finished(true));

  ASSERT_NE(nullptr, delegate_event_handler_);
  delegate_event_handler_->OnStreamError(kStreamId);

  base::RunLoop().RunUntilIdle();
  Mock::VerifyAndClear(&deleter_);
}

TEST_F(MojoAudioOutputStreamTest, DelegateErrorAfterCreated_PropagatesError) {
  AudioOutputStreamPtr audio_output_ptr = CreateAudioOutput();
  EXPECT_CALL(client_, GotNotification());
  EXPECT_CALL(deleter_, Finished(true));
  base::RunLoop().RunUntilIdle();

  ASSERT_NE(nullptr, delegate_event_handler_);
  foreign_socket_->ExpectOwnershipTransfer();
  delegate_event_handler_->OnStreamCreated(kStreamId, std::move(mem_),
                                           std::move(foreign_socket_));
  delegate_event_handler_->OnStreamError(kStreamId);

  base::RunLoop().RunUntilIdle();
  Mock::VerifyAndClear(&deleter_);
}

TEST_F(MojoAudioOutputStreamTest, RemoteEndGone_CallsDeleter) {
  AudioOutputStreamPtr audio_output_ptr = CreateAudioOutput();

  EXPECT_CALL(client_, GotNotification());
  EXPECT_CALL(deleter_, Finished(false));

  delegate_event_handler_->OnStreamCreated(kStreamId, std::move(mem_),
                                           std::move(foreign_socket_));
  audio_output_ptr.reset();
  base::RunLoop().RunUntilIdle();
  Mock::VerifyAndClear(&deleter_);
}

}  // namespace media
