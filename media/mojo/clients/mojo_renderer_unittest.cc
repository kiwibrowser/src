// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/test_message_loop.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/timer/elapsed_timer.h"
#include "media/base/cdm_config.h"
#include "media/base/cdm_context.h"
#include "media/base/gmock_callback_support.h"
#include "media/base/mock_filters.h"
#include "media/base/test_helpers.h"
#include "media/cdm/default_cdm_factory.h"
#include "media/mojo/clients/mojo_renderer.h"
#include "media/mojo/common/media_type_converters.h"
#include "media/mojo/interfaces/content_decryption_module.mojom.h"
#include "media/mojo/interfaces/renderer.mojom.h"
#include "media/mojo/services/mojo_cdm_service.h"
#include "media/mojo/services/mojo_cdm_service_context.h"
#include "media/mojo/services/mojo_renderer_service.h"
#include "media/renderers/video_overlay_factory.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::StrictMock;

namespace media {

namespace {
const int64_t kStartPlayingTimeInMs = 100;
const char kClearKeyKeySystem[] = "org.w3.clearkey";

ACTION_P2(GetMediaTime, start_time, elapsed_timer) {
  return start_time + elapsed_timer->Elapsed();
}

void WaitFor(base::TimeDelta duration) {
  base::RunLoop run_loop;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), duration);
  run_loop.Run();
}
}  // namespace

class MojoRendererTest : public ::testing::Test {
 public:
  MojoRendererTest()
      : mojo_cdm_service_(&cdm_factory_, &mojo_cdm_service_context_),
        cdm_binding_(&mojo_cdm_service_) {
    std::unique_ptr<StrictMock<MockRenderer>> mock_renderer(
        new StrictMock<MockRenderer>());
    mock_renderer_ = mock_renderer.get();

    mojom::RendererPtr remote_renderer;
    renderer_binding_ = MojoRendererService::Create(
        &mojo_cdm_service_context_, std::move(mock_renderer),
        MojoRendererService::InitiateSurfaceRequestCB(),
        mojo::MakeRequest(&remote_renderer));

    mojo_renderer_.reset(
        new MojoRenderer(message_loop_.task_runner(),
                         std::unique_ptr<VideoOverlayFactory>(nullptr), nullptr,
                         std::move(remote_renderer)));

    // CreateAudioStream() and CreateVideoStream() overrides expectations for
    // expected non-NULL streams.
    EXPECT_CALL(demuxer_, GetAllStreams()).WillRepeatedly(Return(streams_));

    EXPECT_CALL(*mock_renderer_, GetMediaTime())
        .WillRepeatedly(Return(base::TimeDelta()));
  }

  ~MojoRendererTest() override = default;

  void Destroy() {
    mojo_renderer_.reset();
    base::RunLoop().RunUntilIdle();
  }

  // Completion callbacks.
  MOCK_METHOD1(OnInitialized, void(PipelineStatus));
  MOCK_METHOD0(OnFlushed, void());
  MOCK_METHOD1(OnCdmAttached, void(bool));
  MOCK_METHOD1(OnSurfaceRequestToken, void(const base::UnguessableToken&));

  std::unique_ptr<StrictMock<MockDemuxerStream>> CreateStream(
      DemuxerStream::Type type) {
    std::unique_ptr<StrictMock<MockDemuxerStream>> stream(
        new StrictMock<MockDemuxerStream>(type));
    return stream;
  }

  void CreateAudioStream() {
    audio_stream_ = CreateStream(DemuxerStream::AUDIO);
    audio_stream_->set_audio_decoder_config(TestAudioConfig::Normal());
    streams_.push_back(audio_stream_.get());
    EXPECT_CALL(demuxer_, GetAllStreams()).WillRepeatedly(Return(streams_));
  }

  void CreateVideoStream(bool is_encrypted = false) {
    video_stream_ = CreateStream(DemuxerStream::VIDEO);
    video_stream_->set_video_decoder_config(
        is_encrypted ? TestVideoConfig::NormalEncrypted()
                     : TestVideoConfig::Normal());
    std::vector<DemuxerStream*> streams;
    streams_.push_back(audio_stream_.get());
    EXPECT_CALL(demuxer_, GetAllStreams()).WillRepeatedly(Return(streams_));
  }

  void InitializeAndExpect(PipelineStatus status) {
    DVLOG(1) << __func__ << ": " << status;
    EXPECT_CALL(*this, OnInitialized(status));
    mojo_renderer_->Initialize(
        &demuxer_, &renderer_client_,
        base::Bind(&MojoRendererTest::OnInitialized, base::Unretained(this)));
    base::RunLoop().RunUntilIdle();
  }

  void Initialize() {
    CreateAudioStream();
    EXPECT_CALL(*mock_renderer_, Initialize(_, _, _))
        .WillOnce(DoAll(SaveArg<1>(&remote_renderer_client_),
                        RunCallback<2>(PIPELINE_OK)));
    InitializeAndExpect(PIPELINE_OK);
  }

  void Flush() {
    DVLOG(1) << __func__;
    // Flush callback should always be fired.
    EXPECT_CALL(*this, OnFlushed());
    mojo_renderer_->Flush(
        base::Bind(&MojoRendererTest::OnFlushed, base::Unretained(this)));
    base::RunLoop().RunUntilIdle();
  }

  void SetCdmAndExpect(bool success) {
    DVLOG(1) << __func__;
    // Set CDM callback should always be fired.
    EXPECT_CALL(*this, OnCdmAttached(success));
    mojo_renderer_->SetCdm(
        &cdm_context_,
        base::Bind(&MojoRendererTest::OnCdmAttached, base::Unretained(this)));
    base::RunLoop().RunUntilIdle();
  }

  // Simulates a connection error at the client side by killing the service.
  // Note that |mock_renderer_| will also be destroyed, do NOT expect anything
  // on it. Otherwise the test will crash.
  void ConnectionError() {
    DVLOG(1) << __func__;
    DCHECK(renderer_binding_);
    renderer_binding_->Close();
    base::RunLoop().RunUntilIdle();
  }

  void OnCdmCreated(mojom::CdmPromiseResultPtr result,
                    int cdm_id,
                    mojom::DecryptorPtr decryptor) {
    EXPECT_TRUE(result->success);
    EXPECT_NE(CdmContext::kInvalidCdmId, cdm_id);
    cdm_context_.set_cdm_id(cdm_id);
  }

  void CreateCdm() {
    cdm_binding_.Bind(mojo::MakeRequest(&remote_cdm_));
    remote_cdm_->Initialize(
        kClearKeyKeySystem, url::Origin::Create(GURL("https://www.test.com")),
        CdmConfig(),
        base::Bind(&MojoRendererTest::OnCdmCreated, base::Unretained(this)));
    base::RunLoop().RunUntilIdle();
  }

  void StartPlayingFrom(base::TimeDelta start_time) {
    EXPECT_CALL(*mock_renderer_, StartPlayingFrom(start_time));
    mojo_renderer_->StartPlayingFrom(start_time);
    EXPECT_EQ(start_time, mojo_renderer_->GetMediaTime());
    base::RunLoop().RunUntilIdle();
  }

  void Play() {
    StartPlayingFrom(base::TimeDelta::FromMilliseconds(kStartPlayingTimeInMs));
  }

  // Fixture members.
  base::TestMessageLoop message_loop_;

  // The MojoRenderer that we are testing.
  std::unique_ptr<MojoRenderer> mojo_renderer_;

  // Client side mocks and helpers.
  StrictMock<MockRendererClient> renderer_client_;
  StrictMock<MockCdmContext> cdm_context_;
  mojom::ContentDecryptionModulePtr remote_cdm_;

  // Client side mock demuxer and demuxer streams.
  StrictMock<MockDemuxer> demuxer_;
  std::unique_ptr<StrictMock<MockDemuxerStream>> audio_stream_;
  std::unique_ptr<StrictMock<MockDemuxerStream>> video_stream_;
  std::vector<DemuxerStream*> streams_;

  // Service side bindings (declaration order is critical).
  MojoCdmServiceContext mojo_cdm_service_context_;
  DefaultCdmFactory cdm_factory_;
  MojoCdmService mojo_cdm_service_;
  mojo::Binding<mojom::ContentDecryptionModule> cdm_binding_;

  // Service side mocks and helpers.
  StrictMock<MockRenderer>* mock_renderer_;
  RendererClient* remote_renderer_client_;

  mojo::StrongBindingPtr<mojom::Renderer> renderer_binding_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MojoRendererTest);
};

TEST_F(MojoRendererTest, Initialize_Success) {
  Initialize();
}

TEST_F(MojoRendererTest, Initialize_Failure) {
  CreateAudioStream();
  // Mojo Renderer only expects a boolean result, which will be translated
  // to PIPELINE_OK or PIPELINE_ERROR_INITIALIZATION_FAILED.
  EXPECT_CALL(*mock_renderer_, Initialize(_, _, _))
      .WillOnce(RunCallback<2>(PIPELINE_ERROR_ABORT));
  InitializeAndExpect(PIPELINE_ERROR_INITIALIZATION_FAILED);
}

TEST_F(MojoRendererTest, Initialize_BeforeConnectionError) {
  CreateAudioStream();
  EXPECT_CALL(*mock_renderer_, Initialize(_, _, _))
      .WillOnce(InvokeWithoutArgs(this, &MojoRendererTest::ConnectionError));
  InitializeAndExpect(PIPELINE_ERROR_INITIALIZATION_FAILED);
}

TEST_F(MojoRendererTest, Initialize_AfterConnectionError) {
  ConnectionError();
  CreateAudioStream();
  InitializeAndExpect(PIPELINE_ERROR_INITIALIZATION_FAILED);
}

TEST_F(MojoRendererTest, Flush_Success) {
  Initialize();

  EXPECT_CALL(*mock_renderer_, Flush(_)).WillOnce(RunClosure<0>());
  Flush();
}

TEST_F(MojoRendererTest, Flush_ConnectionError) {
  Initialize();

  // Upon connection error, OnError() should be called once and only once.
  EXPECT_CALL(renderer_client_, OnError(PIPELINE_ERROR_DECODE)).Times(1);
  EXPECT_CALL(*mock_renderer_, Flush(_))
      .WillOnce(InvokeWithoutArgs(this, &MojoRendererTest::ConnectionError));
  Flush();
}

TEST_F(MojoRendererTest, Flush_AfterConnectionError) {
  Initialize();

  // Upon connection error, OnError() should be called once and only once.
  EXPECT_CALL(renderer_client_, OnError(PIPELINE_ERROR_DECODE)).Times(1);
  ConnectionError();

  Flush();
}

TEST_F(MojoRendererTest, SetCdm_Success) {
  Initialize();
  CreateCdm();
  EXPECT_CALL(*mock_renderer_, SetCdm(_, _)).WillOnce(RunCallback<1>(true));
  SetCdmAndExpect(true);
}

TEST_F(MojoRendererTest, SetCdm_Failure) {
  Initialize();
  CreateCdm();
  EXPECT_CALL(*mock_renderer_, SetCdm(_, _)).WillOnce(RunCallback<1>(false));
  SetCdmAndExpect(false);
}

TEST_F(MojoRendererTest, SetCdm_InvalidCdmId) {
  Initialize();
  SetCdmAndExpect(false);
}

TEST_F(MojoRendererTest, SetCdm_NonExistCdmId) {
  Initialize();
  cdm_context_.set_cdm_id(1);
  SetCdmAndExpect(false);
}

TEST_F(MojoRendererTest, SetCdm_BeforeInitialize) {
  CreateCdm();
  EXPECT_CALL(*mock_renderer_, SetCdm(_, _)).WillOnce(RunCallback<1>(true));
  SetCdmAndExpect(true);
}

TEST_F(MojoRendererTest, SetCdm_AfterInitializeAndConnectionError) {
  CreateCdm();
  Initialize();
  EXPECT_CALL(renderer_client_, OnError(PIPELINE_ERROR_DECODE)).Times(1);
  ConnectionError();
  SetCdmAndExpect(false);
}

TEST_F(MojoRendererTest, SetCdm_AfterConnectionErrorAndBeforeInitialize) {
  CreateCdm();
  // Initialize() is not called so RendererClient::OnError() is not expected.
  ConnectionError();
  SetCdmAndExpect(false);
  InitializeAndExpect(PIPELINE_ERROR_INITIALIZATION_FAILED);
}

TEST_F(MojoRendererTest, SetCdm_BeforeInitializeAndConnectionError) {
  CreateCdm();
  EXPECT_CALL(*mock_renderer_, SetCdm(_, _)).WillOnce(RunCallback<1>(true));
  SetCdmAndExpect(true);
  // Initialize() is not called so RendererClient::OnError() is not expected.
  ConnectionError();
  CreateAudioStream();
  InitializeAndExpect(PIPELINE_ERROR_INITIALIZATION_FAILED);
}

TEST_F(MojoRendererTest, StartPlayingFrom) {
  Initialize();
  Play();
}

TEST_F(MojoRendererTest, GetMediaTime) {
  Initialize();
  EXPECT_EQ(base::TimeDelta(), mojo_renderer_->GetMediaTime());

  const base::TimeDelta kSleepTime = base::TimeDelta::FromMilliseconds(500);
  const base::TimeDelta kStartTime =
      base::TimeDelta::FromMilliseconds(kStartPlayingTimeInMs);

  // Media time should not advance since playback rate is 0.
  EXPECT_CALL(*mock_renderer_, SetPlaybackRate(0));
  EXPECT_CALL(*mock_renderer_, StartPlayingFrom(kStartTime));
  EXPECT_CALL(*mock_renderer_, GetMediaTime())
      .WillRepeatedly(Return(kStartTime));
  mojo_renderer_->SetPlaybackRate(0);
  mojo_renderer_->StartPlayingFrom(kStartTime);
  WaitFor(kSleepTime);
  EXPECT_EQ(kStartTime, mojo_renderer_->GetMediaTime());

  // Media time should now advance since playback rate is > 0.
  std::unique_ptr<base::ElapsedTimer> elapsed_timer(new base::ElapsedTimer);
  EXPECT_CALL(*mock_renderer_, SetPlaybackRate(1.0));
  EXPECT_CALL(*mock_renderer_, GetMediaTime())
      .WillRepeatedly(GetMediaTime(kStartTime, elapsed_timer.get()));
  mojo_renderer_->SetPlaybackRate(1.0);
  WaitFor(kSleepTime);
  EXPECT_GT(mojo_renderer_->GetMediaTime(), kStartTime);

  // Flushing should pause media-time updates.
  EXPECT_CALL(*mock_renderer_, Flush(_)).WillOnce(RunClosure<0>());
  Flush();
  base::TimeDelta pause_time = mojo_renderer_->GetMediaTime();
  EXPECT_GT(pause_time, kStartTime);
  WaitFor(kSleepTime);
  EXPECT_EQ(pause_time, mojo_renderer_->GetMediaTime());
  Destroy();
}

// When |initiate_surface_request_cb_| is not set, the client should not call
// InitiateScopedSurfaceRequest(). Otherwise, it will cause the pipe to be
// closed and MojoRendererService destroyed.
TEST_F(MojoRendererTest, InitiateScopedSurfaceRequest) {
  Initialize();
  DCHECK(renderer_binding_);

  EXPECT_CALL(renderer_client_, OnError(PIPELINE_ERROR_DECODE)).Times(1);

  mojo_renderer_->InitiateScopedSurfaceRequest(base::Bind(
      &MojoRendererTest::OnSurfaceRequestToken, base::Unretained(this)));
  base::RunLoop().RunUntilIdle();

  DCHECK(!renderer_binding_);
}

TEST_F(MojoRendererTest, OnBufferingStateChange) {
  Initialize();
  Play();

  EXPECT_CALL(renderer_client_, OnBufferingStateChange(BUFFERING_HAVE_ENOUGH))
      .Times(1);
  remote_renderer_client_->OnBufferingStateChange(BUFFERING_HAVE_ENOUGH);

  EXPECT_CALL(renderer_client_, OnBufferingStateChange(BUFFERING_HAVE_NOTHING))
      .Times(1);
  remote_renderer_client_->OnBufferingStateChange(BUFFERING_HAVE_NOTHING);

  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoRendererTest, OnEnded) {
  Initialize();
  Play();

  EXPECT_CALL(renderer_client_, OnEnded()).Times(1);
  remote_renderer_client_->OnEnded();
  base::RunLoop().RunUntilIdle();
}

// TODO(xhwang): Add tests for all RendererClient methods.

TEST_F(MojoRendererTest, Destroy_PendingInitialize) {
  CreateAudioStream();
  EXPECT_CALL(*mock_renderer_, Initialize(_, _, _))
      .WillRepeatedly(RunCallback<2>(PIPELINE_ERROR_ABORT));
  EXPECT_CALL(*this, OnInitialized(PIPELINE_ERROR_INITIALIZATION_FAILED));
  mojo_renderer_->Initialize(
      &demuxer_, &renderer_client_,
      base::Bind(&MojoRendererTest::OnInitialized, base::Unretained(this)));
  Destroy();
}

TEST_F(MojoRendererTest, Destroy_PendingFlush) {
  EXPECT_CALL(*mock_renderer_, SetCdm(_, _))
      .WillRepeatedly(RunCallback<1>(true));
  EXPECT_CALL(*this, OnCdmAttached(false));
  mojo_renderer_->SetCdm(
      &cdm_context_,
      base::Bind(&MojoRendererTest::OnCdmAttached, base::Unretained(this)));
  Destroy();
}

TEST_F(MojoRendererTest, Destroy_PendingSetCdm) {
  Initialize();

  EXPECT_CALL(*mock_renderer_, Flush(_)).WillRepeatedly(RunClosure<0>());
  EXPECT_CALL(*this, OnFlushed());
  mojo_renderer_->Flush(
      base::Bind(&MojoRendererTest::OnFlushed, base::Unretained(this)));
  Destroy();
}

// TODO(xhwang): Add more tests on OnError. For example, ErrorDuringFlush,
// ErrorAfterFlush etc.

TEST_F(MojoRendererTest, ErrorDuringPlayback) {
  Initialize();

  EXPECT_CALL(renderer_client_, OnError(PIPELINE_ERROR_DECODE)).Times(1);

  Play();
  remote_renderer_client_->OnError(PIPELINE_ERROR_DECODE);
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(*mock_renderer_, SetPlaybackRate(0.0)).Times(1);
  mojo_renderer_->SetPlaybackRate(0.0);
  Flush();
}

}  // namespace media
