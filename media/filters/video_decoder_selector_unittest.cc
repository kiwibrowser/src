// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "media/base/gmock_callback_support.h"
#include "media/base/mock_filters.h"
#include "media/base/test_helpers.h"
#include "media/filters/decoder_selector.h"
#include "media/filters/decrypting_demuxer_stream.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(OS_ANDROID)
#include "media/filters/decrypting_video_decoder.h"
#endif

using ::testing::_;
using ::testing::IsNull;
using ::testing::NiceMock;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::StrictMock;

// Use anonymous namespace here to prevent the actions to be defined multiple
// times across multiple test files. Sadly we can't use static for them.
namespace {

MATCHER(EncryptedConfig, "") {
  return arg.is_encrypted();
}
MATCHER(ClearConfig, "") {
  return !arg.is_encrypted();
}

}  // namespace

namespace media {

const char kDecoder1[] = "Decoder1";
const char kDecoder2[] = "Decoder2";

class VideoDecoderSelectorTest : public ::testing::Test {
 public:
  enum DecryptorCapability {
    kNoCdm,        // No CDM. Only possible for clear stream.
    kNoDecryptor,  // CDM is available but Decryptor is not supported.
    kDecryptOnly,
    kDecryptAndDecode
  };

  VideoDecoderSelectorTest()
      : traits_(&media_log_),
        demuxer_stream_(
            new StrictMock<MockDemuxerStream>(DemuxerStream::VIDEO)) {
    // |cdm_context_| and |decryptor_| are conditionally created in
    // InitializeDecoderSelector().
  }

  ~VideoDecoderSelectorTest() override { base::RunLoop().RunUntilIdle(); }

  MOCK_METHOD2(OnDecoderSelected,
               void(VideoDecoder*, DecryptingDemuxerStream*));
  MOCK_METHOD1(OnDecoderOneSelected, void(DecryptingDemuxerStream*));
  MOCK_METHOD1(OnDecoderTwoSelected, void(DecryptingDemuxerStream*));

  void MockOnDecoderSelected(std::unique_ptr<VideoDecoder> decoder,
                             std::unique_ptr<DecryptingDemuxerStream> stream) {
    selected_decoder_ = std::move(decoder);
    if (!selected_decoder_) {
      OnDecoderSelected(selected_decoder_.get(), stream.get());
      return;
    }

    if (selected_decoder_->GetDisplayName() == kDecoder1)
      OnDecoderOneSelected(stream.get());
    else if (selected_decoder_->GetDisplayName() == kDecoder2)
      OnDecoderTwoSelected(stream.get());
    else  // DecryptingVideoDecoder selected...
      OnDecoderSelected(selected_decoder_.get(), stream.get());
  }

  void UseClearStream() {
    demuxer_stream_->set_video_decoder_config(TestVideoConfig::Normal());
  }

  void UseEncryptedStream() {
    demuxer_stream_->set_video_decoder_config(
        TestVideoConfig::NormalEncrypted());
  }

  std::vector<std::unique_ptr<VideoDecoder>> CreateVideoDecodersForTest() {
#if !defined(OS_ANDROID)
    all_decoders_.push_back(std::make_unique<DecryptingVideoDecoder>(
        message_loop_.task_runner(), &media_log_));
#endif

    if (decoder_1_)
      testing::Mock::VerifyAndClearExpectations(decoder_1_);
    if (decoder_2_)
      testing::Mock::VerifyAndClearExpectations(decoder_2_);

    if (num_decoders_ > 0) {
      decoder_1_ = new StrictMock<MockVideoDecoder>(kDecoder1);
      all_decoders_.push_back(base::WrapUnique(decoder_1_));

      EXPECT_CALL(*decoder_1_, Initialize(_, _, _, _, _, _))
          .Times(testing::AnyNumber())
          .WillRepeatedly(
              Invoke(this, &VideoDecoderSelectorTest::OnDecoderOneInitialized));
    }

    if (num_decoders_ > 1) {
      decoder_2_ = new StrictMock<MockVideoDecoder>(kDecoder2);
      all_decoders_.push_back(base::WrapUnique(decoder_2_));
      EXPECT_CALL(*decoder_2_, Initialize(_, _, _, _, _, _))
          .Times(testing::AnyNumber())
          .WillRepeatedly(
              Invoke(this, &VideoDecoderSelectorTest::OnDecoderTwoInitialized));
    }

    return std::move(all_decoders_);
  }

  void InitializeDecoderSelector(DecryptorCapability decryptor_capability,
                                 int num_decoders) {
    if (decryptor_capability != kNoCdm) {
      cdm_context_.reset(new StrictMock<MockCdmContext>());

      if (decryptor_capability == kNoDecryptor) {
        EXPECT_CALL(*cdm_context_, GetDecryptor())
            .WillRepeatedly(Return(nullptr));
      } else {
        decryptor_.reset(new NiceMock<MockDecryptor>());
        EXPECT_CALL(*cdm_context_, GetDecryptor())
            .WillRepeatedly(Return(decryptor_.get()));
        EXPECT_CALL(*decryptor_, InitializeVideoDecoder(_, _))
            .WillRepeatedly(
                RunCallback<1>(decryptor_capability == kDecryptAndDecode));
      }
    }

    num_decoders_ = num_decoders;

    decoder_selector_.reset(new VideoDecoderSelector(
        message_loop_.task_runner(),
        base::Bind(&VideoDecoderSelectorTest::CreateVideoDecodersForTest,
                   base::Unretained(this)),
        &media_log_));
  }

  void SelectDecoder() { SelectDecoderWithBlacklist(""); }

  void SelectDecoderWithBlacklist(const std::string& blacklisted_decoder) {
    decoder_selector_->SelectDecoder(
        &traits_, demuxer_stream_.get(), cdm_context_.get(),
        blacklisted_decoder,
        base::Bind(&VideoDecoderSelectorTest::MockOnDecoderSelected,
                   base::Unretained(this)),
        base::Bind(&VideoDecoderSelectorTest::FrameReady,
                   base::Unretained(this)),
        base::Bind(&VideoDecoderSelectorTest::OnWaitingForDecryptionKey,
                   base::Unretained(this)));
    base::RunLoop().RunUntilIdle();
  }

  void SelectDecoderAndDestroy() {
    SelectDecoder();

    EXPECT_CALL(*this, OnDecoderSelected(IsNull(), IsNull()));
    decoder_selector_.reset();
    base::RunLoop().RunUntilIdle();
  }

  void FrameReady(const scoped_refptr<VideoFrame>& frame) { NOTREACHED(); }

  void OnWaitingForDecryptionKey() { NOTREACHED(); }

  MOCK_METHOD6(OnDecoderOneInitialized,
               void(const VideoDecoderConfig& config,
                    bool low_delay,
                    CdmContext* cdm_context,
                    const VideoDecoder::InitCB& init_cb,
                    const VideoDecoder::OutputCB& output_cb,
                    const VideoDecoder::WaitingForDecryptionKeyCB&
                        waiting_for_decryption_key_cb));
  MOCK_METHOD6(OnDecoderTwoInitialized,
               void(const VideoDecoderConfig& config,
                    bool low_delay,
                    CdmContext* cdm_context,
                    const VideoDecoder::InitCB& init_cb,
                    const VideoDecoder::OutputCB& output_cb,
                    const VideoDecoder::WaitingForDecryptionKeyCB&
                        waiting_for_decryption_key_cb));

  MediaLog media_log_;

  // Stream traits specific to video decoding.
  DecoderStreamTraits<DemuxerStream::VIDEO> traits_;

  // Declare |decoder_selector_| after |demuxer_stream_| and |decryptor_| since
  // |demuxer_stream_| and |decryptor_| should outlive |decoder_selector_|.
  std::unique_ptr<StrictMock<MockDemuxerStream>> demuxer_stream_;

  std::unique_ptr<StrictMock<MockCdmContext>> cdm_context_;

  // Use NiceMock since we don't care about most of calls on the decryptor, e.g.
  // RegisterNewKeyCB().
  std::unique_ptr<NiceMock<MockDecryptor>> decryptor_;

  std::unique_ptr<VideoDecoderSelector> decoder_selector_;

  StrictMock<MockVideoDecoder>* decoder_1_ = nullptr;
  StrictMock<MockVideoDecoder>* decoder_2_ = nullptr;
  std::vector<std::unique_ptr<VideoDecoder>> all_decoders_;
  std::unique_ptr<VideoDecoder> selected_decoder_;

  int num_decoders_ = 0;

  base::MessageLoop message_loop_;

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoDecoderSelectorTest);
};

// Tests for clear streams. CDM will not be used for clear streams so
// DecryptorCapability doesn't really matter.

TEST_F(VideoDecoderSelectorTest, ClearStream_NoClearDecoder) {
  UseClearStream();

  // DecoderSelector will not try decrypting decoders for clear stream, even
  // if the CDM is capable of doing decrypt and decode.
  InitializeDecoderSelector(kDecryptAndDecode, 0);

  EXPECT_CALL(*this, OnDecoderSelected(IsNull(), IsNull()));

  SelectDecoder();
}

TEST_F(VideoDecoderSelectorTest, ClearStream_OneClearDecoder) {
  UseClearStream();
  InitializeDecoderSelector(kNoCdm, 1);

  EXPECT_CALL(*this, OnDecoderOneInitialized(ClearConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(true));
  EXPECT_CALL(*this, OnDecoderOneSelected(IsNull()));

  SelectDecoder();
}

TEST_F(VideoDecoderSelectorTest, Destroy_ClearStream_OneClearDecoder) {
  UseClearStream();
  InitializeDecoderSelector(kNoCdm, 1);

  EXPECT_CALL(*this, OnDecoderOneInitialized(ClearConfig(), _, _, _, _, _));

  SelectDecoderAndDestroy();
}

TEST_F(VideoDecoderSelectorTest, ClearStream_MultipleClearDecoder) {
  UseClearStream();
  InitializeDecoderSelector(kNoCdm, 2);

  EXPECT_CALL(*this, OnDecoderOneInitialized(ClearConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderTwoInitialized(ClearConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(true));
  EXPECT_CALL(*this, OnDecoderTwoSelected(IsNull()));

  SelectDecoder();
}

TEST_F(VideoDecoderSelectorTest, Destroy_ClearStream_MultipleClearDecoder) {
  UseClearStream();
  InitializeDecoderSelector(kNoCdm, 2);

  EXPECT_CALL(*this, OnDecoderOneInitialized(ClearConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderTwoInitialized(ClearConfig(), _, _, _, _, _));

  SelectDecoderAndDestroy();
}

TEST_F(VideoDecoderSelectorTest, ClearStream_BlackListedDecoder) {
  UseClearStream();
  InitializeDecoderSelector(kNoCdm, 2);

  // Decoder 1 is blacklisted and will not even be tried.
  EXPECT_CALL(*this, OnDecoderTwoInitialized(ClearConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(true));
  EXPECT_CALL(*this, OnDecoderTwoSelected(IsNull()));

  SelectDecoderWithBlacklist(kDecoder1);
}

// Tests for encrypted streams.

TEST_F(VideoDecoderSelectorTest, EncryptedStream_NoDecryptor_OneClearDecoder) {
  UseEncryptedStream();
  InitializeDecoderSelector(kNoDecryptor, 1);

  EXPECT_CALL(*this, OnDecoderOneInitialized(EncryptedConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderSelected(IsNull(), IsNull()));

  SelectDecoder();
}

TEST_F(VideoDecoderSelectorTest,
       Destroy_EncryptedStream_NoDecryptor_OneClearDecoder) {
  UseEncryptedStream();
  InitializeDecoderSelector(kNoDecryptor, 1);

  EXPECT_CALL(*this, OnDecoderOneInitialized(EncryptedConfig(), _, _, _, _, _));

  SelectDecoderAndDestroy();
}

TEST_F(VideoDecoderSelectorTest, EncryptedStream_NoDecryptor_MultipleDecoders) {
  UseEncryptedStream();
  InitializeDecoderSelector(kNoDecryptor, 2);

  EXPECT_CALL(*this, OnDecoderOneInitialized(EncryptedConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderTwoInitialized(EncryptedConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(true));
  EXPECT_CALL(*this, OnDecoderTwoSelected(IsNull()));

  SelectDecoder();
}

TEST_F(VideoDecoderSelectorTest,
       Destroy_EncryptedStream_NoDecryptor_MultipleDecoders) {
  UseEncryptedStream();
  InitializeDecoderSelector(kNoDecryptor, 2);

  EXPECT_CALL(*this, OnDecoderOneInitialized(EncryptedConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderTwoInitialized(EncryptedConfig(), _, _, _, _, _));

  SelectDecoderAndDestroy();
}

TEST_F(VideoDecoderSelectorTest, EncryptedStream_DecryptOnly_NoDecoder) {
  UseEncryptedStream();
  InitializeDecoderSelector(kDecryptOnly, 0);

  EXPECT_CALL(*this, OnDecoderSelected(IsNull(), IsNull()));

  SelectDecoder();
}

TEST_F(VideoDecoderSelectorTest, EncryptedStream_DecryptOnly_OneClearDecoder) {
  UseEncryptedStream();
  InitializeDecoderSelector(kDecryptOnly, 1);

  // Since we use DecryptingDemuxerStream, the decoder will be initialized with
  // a clear config.
  EXPECT_CALL(*this, OnDecoderOneInitialized(EncryptedConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderOneInitialized(ClearConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(true));
  EXPECT_CALL(*this, OnDecoderOneSelected(NotNull()));

  SelectDecoder();
}

TEST_F(VideoDecoderSelectorTest,
       Destroy_EncryptedStream_DecryptOnly_OneClearDecoder) {
  UseEncryptedStream();
  InitializeDecoderSelector(kDecryptOnly, 1);

  EXPECT_CALL(*this, OnDecoderOneInitialized(EncryptedConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderOneInitialized(ClearConfig(), _, _, _, _, _));

  SelectDecoderAndDestroy();
}

TEST_F(VideoDecoderSelectorTest,
       EncryptedStream_DecryptOnly_MultipleClearDecoder) {
  UseEncryptedStream();
  InitializeDecoderSelector(kDecryptOnly, 2);

  EXPECT_CALL(*this, OnDecoderOneInitialized(EncryptedConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderTwoInitialized(EncryptedConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderOneInitialized(ClearConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderTwoInitialized(ClearConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(true));
  EXPECT_CALL(*this, OnDecoderTwoSelected(NotNull()));

  SelectDecoder();
}

TEST_F(VideoDecoderSelectorTest,
       Destroy_EncryptedStream_DecryptOnly_MultipleClearDecoder) {
  UseEncryptedStream();
  InitializeDecoderSelector(kDecryptOnly, 2);

  EXPECT_CALL(*this, OnDecoderOneInitialized(EncryptedConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderTwoInitialized(EncryptedConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderOneInitialized(ClearConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderTwoInitialized(ClearConfig(), _, _, _, _, _));

  SelectDecoderAndDestroy();
}

TEST_F(VideoDecoderSelectorTest, EncryptedStream_DecryptAndDecode) {
  UseEncryptedStream();
  InitializeDecoderSelector(kDecryptAndDecode, 1);

#if !defined(OS_ANDROID)
  // A DecryptingVideoDecoder will be created and selected. The clear decoder
  // should not be touched at all. No DecryptingDemuxerStream should be
  // created.
  EXPECT_CALL(*this, OnDecoderSelected(NotNull(), IsNull()));
#else
  // A DecryptingDemuxerStream will be created. The clear decoder will be
  // initialized and returned.
  EXPECT_CALL(*this, OnDecoderOneInitialized(EncryptedConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderOneInitialized(ClearConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(true));
  EXPECT_CALL(*this, OnDecoderOneSelected(NotNull()));
#endif

  SelectDecoder();
}

TEST_F(VideoDecoderSelectorTest,
       EncryptedStream_NoDecryptor_BlackListedDecoder) {
  UseEncryptedStream();
  InitializeDecoderSelector(kNoDecryptor, 2);

  EXPECT_CALL(*this, OnDecoderTwoInitialized(EncryptedConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(true));
  EXPECT_CALL(*this, OnDecoderTwoSelected(IsNull()));

  SelectDecoderWithBlacklist(kDecoder1);
}

TEST_F(VideoDecoderSelectorTest,
       EncryptedStream_DecryptOnly_BlackListedDecoder) {
  UseEncryptedStream();
  InitializeDecoderSelector(kDecryptOnly, 2);

  // When DecryptingDemuxerStream is chosen, the blacklist is ignored.
  EXPECT_CALL(*this, OnDecoderOneInitialized(EncryptedConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  // DecoderTwo is blacklisted from the first attempt.

  EXPECT_CALL(*this, OnDecoderOneInitialized(ClearConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderTwoInitialized(ClearConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(true));
  EXPECT_CALL(*this, OnDecoderTwoSelected(NotNull()));

  SelectDecoderWithBlacklist(kDecoder2);
}

TEST_F(VideoDecoderSelectorTest,
       EncryptedStream_DecryptAndDecode_BlackListedDecoder) {
  UseEncryptedStream();
  InitializeDecoderSelector(kDecryptAndDecode, 2);

  // DecryptingVideoDecoder is blacklisted so we'll fallback to use
  // DecryptingDemuxerStream to do decrypt-only.
  EXPECT_CALL(*this, OnDecoderOneInitialized(EncryptedConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderTwoInitialized(EncryptedConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderOneInitialized(ClearConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(false));
  EXPECT_CALL(*this, OnDecoderTwoInitialized(ClearConfig(), _, _, _, _, _))
      .WillOnce(RunCallback<3>(true));
  EXPECT_CALL(*this, OnDecoderTwoSelected(NotNull()));

  // TODO(xhwang): Avoid the hardcoded string here.
  SelectDecoderWithBlacklist("DecryptingVideoDecoder");
}

}  // namespace media
