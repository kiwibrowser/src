// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <string.h>

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/containers/queue.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "media/base/test_data_util.h"
#include "media/gpu/h264_decoder.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::Expectation;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::MakeMatcher;
using ::testing::Matcher;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::Mock;
using ::testing::Return;

namespace media {
namespace {

const std::string kBaselineFrame0 = "bear-320x192-baseline-frame-0.h264";
const std::string kBaselineFrame1 = "bear-320x192-baseline-frame-1.h264";
const std::string kBaselineFrame2 = "bear-320x192-baseline-frame-2.h264";
const std::string kBaselineFrame3 = "bear-320x192-baseline-frame-3.h264";
const std::string kHighFrame0 = "bear-320x192-high-frame-0.h264";
const std::string kHighFrame1 = "bear-320x192-high-frame-1.h264";
const std::string kHighFrame2 = "bear-320x192-high-frame-2.h264";
const std::string kHighFrame3 = "bear-320x192-high-frame-3.h264";

class MockH264Accelerator : public H264Decoder::H264Accelerator {
 public:
  MockH264Accelerator() = default;

  MOCK_METHOD0(CreateH264Picture, scoped_refptr<H264Picture>());
  MOCK_METHOD1(SubmitDecode, bool(const scoped_refptr<H264Picture>& pic));
  MOCK_METHOD1(OutputPicture, bool(const scoped_refptr<H264Picture>& pic));

  bool SubmitFrameMetadata(const H264SPS* sps,
                           const H264PPS* pps,
                           const H264DPB& dpb,
                           const H264Picture::Vector& ref_pic_listp0,
                           const H264Picture::Vector& ref_pic_listb0,
                           const H264Picture::Vector& ref_pic_listb1,
                           const scoped_refptr<H264Picture>& pic) override {
    return true;
  }

  bool SubmitSlice(const H264PPS* pps,
                   const H264SliceHeader* slice_hdr,
                   const H264Picture::Vector& ref_pic_list0,
                   const H264Picture::Vector& ref_pic_list1,
                   const scoped_refptr<H264Picture>& pic,
                   const uint8_t* data,
                   size_t size) override {
    return true;
  }

  void Reset() override {}
};

// Test H264Decoder by feeding different of h264 frame sequences and make
// sure it behaves as expected.
class H264DecoderTest : public ::testing::Test {
 public:
  H264DecoderTest() = default;

  void SetUp() override;

  // Sets the bitstreams to be decoded, frame by frame. The content of each
  // file is the encoded bitstream of a single video frame.
  void SetInputFrameFiles(const std::vector<std::string>& frame_files);

  // Keeps decoding the input bitstream set at |SetInputFrameFiles| until the
  // decoder has consumed all bitstreams or returned from
  // |H264Decoder::Decode|. Returns the same result as |H264Decoder::Decode|.
  AcceleratedVideoDecoder::DecodeResult Decode();

 protected:
  std::unique_ptr<H264Decoder> decoder_;
  MockH264Accelerator* accelerator_;

 private:
  base::queue<std::string> input_frame_files_;
  std::string bitstream_;
};

void H264DecoderTest::SetUp() {
  auto mock_accelerator = std::make_unique<MockH264Accelerator>();
  accelerator_ = mock_accelerator.get();
  decoder_.reset(new H264Decoder(std::move(mock_accelerator)));

  // Sets default behaviors for mock methods for convenience.
  ON_CALL(*accelerator_, CreateH264Picture()).WillByDefault(Invoke([]() {
    return new H264Picture();
  }));
  ON_CALL(*accelerator_, SubmitDecode(_)).WillByDefault(Return(true));
  ON_CALL(*accelerator_, OutputPicture(_)).WillByDefault(Return(true));
}

void H264DecoderTest::SetInputFrameFiles(
    const std::vector<std::string>& input_frame_files) {
  for (auto f : input_frame_files)
    input_frame_files_.push(f);
}

AcceleratedVideoDecoder::DecodeResult H264DecoderTest::Decode() {
  while (true) {
    auto result = decoder_->Decode();
    int32_t bitstream_id = 0;
    if (result != AcceleratedVideoDecoder::kRanOutOfStreamData ||
        input_frame_files_.empty())
      return result;
    auto input_file = GetTestDataFilePath(input_frame_files_.front());
    input_frame_files_.pop();
    CHECK(base::ReadFileToString(input_file, &bitstream_));
    decoder_->SetStream(bitstream_id++,
                        reinterpret_cast<const uint8_t*>(bitstream_.data()),
                        bitstream_.size());
  }
}

// To have better description on mismatch.
class WithPocMatcher
    : public MatcherInterface<const scoped_refptr<H264Picture>&> {
 public:
  explicit WithPocMatcher(int expected_poc) : expected_poc_(expected_poc) {}

  bool MatchAndExplain(const scoped_refptr<H264Picture>& p,
                       MatchResultListener* listener) const override {
    if (p->pic_order_cnt == expected_poc_)
      return true;
    *listener << "with poc: " << p->pic_order_cnt;
    return false;
  }

  void DescribeTo(std::ostream* os) const override {
    *os << "with poc " << expected_poc_;
  }

 private:
  int expected_poc_;
};

inline Matcher<const scoped_refptr<H264Picture>&> WithPoc(int expected_poc) {
  return MakeMatcher(new WithPocMatcher(expected_poc));
}

// Test Cases

TEST_F(H264DecoderTest, DecodeSingleFrame) {
  SetInputFrameFiles({kBaselineFrame0});
  ASSERT_EQ(AcceleratedVideoDecoder::kAllocateNewSurfaces, Decode());
  EXPECT_EQ(gfx::Size(320, 192), decoder_->GetPicSize());
  EXPECT_LE(9u, decoder_->GetRequiredNumOfPictures());

  EXPECT_CALL(*accelerator_, CreateH264Picture()).WillOnce(Return(nullptr));
  ASSERT_EQ(AcceleratedVideoDecoder::kRanOutOfSurfaces, Decode());
  ASSERT_TRUE(Mock::VerifyAndClearExpectations(&*accelerator_));

  {
    InSequence sequence;
    EXPECT_CALL(*accelerator_, CreateH264Picture());
    EXPECT_CALL(*accelerator_, SubmitDecode(_));
    EXPECT_CALL(*accelerator_, OutputPicture(_));
  }
  ASSERT_EQ(AcceleratedVideoDecoder::kRanOutOfStreamData, Decode());
  ASSERT_TRUE(decoder_->Flush());
}

TEST_F(H264DecoderTest, SkipNonIDRFrames) {
  SetInputFrameFiles({kBaselineFrame1, kBaselineFrame2, kBaselineFrame0});
  ASSERT_EQ(AcceleratedVideoDecoder::kAllocateNewSurfaces, Decode());
  EXPECT_EQ(gfx::Size(320, 192), decoder_->GetPicSize());
  EXPECT_LE(9u, decoder_->GetRequiredNumOfPictures());
  {
    InSequence sequence;
    EXPECT_CALL(*accelerator_, CreateH264Picture());
    EXPECT_CALL(*accelerator_, SubmitDecode(_));
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(0)));
  }
  ASSERT_EQ(AcceleratedVideoDecoder::kRanOutOfStreamData, Decode());
  ASSERT_TRUE(decoder_->Flush());
}

TEST_F(H264DecoderTest, DecodeProfileBaseline) {
  SetInputFrameFiles({
      kBaselineFrame0, kBaselineFrame1, kBaselineFrame2, kBaselineFrame3,
  });
  ASSERT_EQ(AcceleratedVideoDecoder::kAllocateNewSurfaces, Decode());
  EXPECT_EQ(gfx::Size(320, 192), decoder_->GetPicSize());
  EXPECT_LE(9u, decoder_->GetRequiredNumOfPictures());

  EXPECT_CALL(*accelerator_, CreateH264Picture()).Times(4);
  Expectation decode_poc0, decode_poc2, decode_poc4, decode_poc6;
  {
    InSequence decode_order;
    decode_poc0 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(0)));
    decode_poc2 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(2)));
    decode_poc4 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(4)));
    decode_poc6 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(6)));
  }
  {
    InSequence display_order;
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(0))).After(decode_poc0);
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(2))).After(decode_poc2);
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(4))).After(decode_poc4);
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(6))).After(decode_poc6);
  }
  ASSERT_EQ(AcceleratedVideoDecoder::kRanOutOfStreamData, Decode());
  ASSERT_TRUE(decoder_->Flush());
}

TEST_F(H264DecoderTest, DecodeProfileHigh) {
  SetInputFrameFiles({kHighFrame0, kHighFrame1, kHighFrame2, kHighFrame3});
  ASSERT_EQ(AcceleratedVideoDecoder::kAllocateNewSurfaces, Decode());
  EXPECT_EQ(gfx::Size(320, 192), decoder_->GetPicSize());
  EXPECT_LE(16u, decoder_->GetRequiredNumOfPictures());

  // Two pictures will be kept in DPB for reordering. The first picture should
  // be outputted after feeding the third frame.
  EXPECT_CALL(*accelerator_, CreateH264Picture()).Times(4);
  Expectation decode_poc0, decode_poc2, decode_poc4, decode_poc6;
  {
    InSequence decode_order;
    decode_poc0 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(0)));
    decode_poc4 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(4)));
    decode_poc2 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(2)));
    decode_poc6 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(6)));
  }
  {
    InSequence display_order;
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(0))).After(decode_poc0);
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(2))).After(decode_poc2);
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(4))).After(decode_poc4);
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(6))).After(decode_poc6);
  }
  ASSERT_EQ(AcceleratedVideoDecoder::kRanOutOfStreamData, Decode());
  ASSERT_TRUE(decoder_->Flush());
}

TEST_F(H264DecoderTest, SwitchBaselineToHigh) {
  SetInputFrameFiles({
      kBaselineFrame0, kHighFrame0, kHighFrame1, kHighFrame2, kHighFrame3,
  });
  ASSERT_EQ(AcceleratedVideoDecoder::kAllocateNewSurfaces, Decode());
  EXPECT_EQ(gfx::Size(320, 192), decoder_->GetPicSize());
  EXPECT_LE(9u, decoder_->GetRequiredNumOfPictures());

  EXPECT_CALL(*accelerator_, CreateH264Picture());
  {
    InSequence sequence;
    EXPECT_CALL(*accelerator_, SubmitDecode(_));
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(0)));
  }
  ASSERT_EQ(AcceleratedVideoDecoder::kAllocateNewSurfaces, Decode());
  EXPECT_EQ(gfx::Size(320, 192), decoder_->GetPicSize());
  EXPECT_LE(16u, decoder_->GetRequiredNumOfPictures());

  ASSERT_TRUE(Mock::VerifyAndClearExpectations(&*accelerator_));

  EXPECT_CALL(*accelerator_, CreateH264Picture()).Times(4);
  Expectation decode_poc0, decode_poc2, decode_poc4, decode_poc6;
  {
    InSequence decode_order;
    decode_poc0 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(0)));
    decode_poc4 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(4)));
    decode_poc2 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(2)));
    decode_poc6 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(6)));
  }
  {
    InSequence display_order;
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(0))).After(decode_poc0);
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(2))).After(decode_poc2);
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(4))).After(decode_poc4);
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(6))).After(decode_poc6);
  }
  ASSERT_EQ(AcceleratedVideoDecoder::kRanOutOfStreamData, Decode());
  ASSERT_TRUE(decoder_->Flush());
}

TEST_F(H264DecoderTest, SwitchHighToBaseline) {
  SetInputFrameFiles({
      kHighFrame0, kBaselineFrame0, kBaselineFrame1, kBaselineFrame2,
      kBaselineFrame3,
  });
  ASSERT_EQ(AcceleratedVideoDecoder::kAllocateNewSurfaces, Decode());
  EXPECT_EQ(gfx::Size(320, 192), decoder_->GetPicSize());
  EXPECT_LE(16u, decoder_->GetRequiredNumOfPictures());

  EXPECT_CALL(*accelerator_, CreateH264Picture());
  {
    InSequence sequence;
    EXPECT_CALL(*accelerator_, SubmitDecode(_));
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(0)));
  }
  ASSERT_EQ(AcceleratedVideoDecoder::kAllocateNewSurfaces, Decode());
  EXPECT_EQ(gfx::Size(320, 192), decoder_->GetPicSize());
  EXPECT_LE(9u, decoder_->GetRequiredNumOfPictures());

  ASSERT_TRUE(Mock::VerifyAndClearExpectations(&*accelerator_));

  EXPECT_CALL(*accelerator_, CreateH264Picture()).Times(4);
  Expectation decode_poc0, decode_poc2, decode_poc4, decode_poc6;
  {
    InSequence decode_order;
    decode_poc0 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(0)));
    decode_poc2 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(2)));
    decode_poc4 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(4)));
    decode_poc6 = EXPECT_CALL(*accelerator_, SubmitDecode(WithPoc(6)));
  }
  {
    InSequence display_order;
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(0))).After(decode_poc0);
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(2))).After(decode_poc2);
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(4))).After(decode_poc4);
    EXPECT_CALL(*accelerator_, OutputPicture(WithPoc(6))).After(decode_poc6);
  }
  ASSERT_EQ(AcceleratedVideoDecoder::kRanOutOfStreamData, Decode());
  ASSERT_TRUE(decoder_->Flush());
}

}  // namespace
}  // namespace media
