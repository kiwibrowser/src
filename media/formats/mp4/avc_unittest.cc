// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "base/macros.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "media/base/decrypt_config.h"
#include "media/base/stream_parser_buffer.h"
#include "media/formats/mp4/avc.h"
#include "media/formats/mp4/box_definitions.h"
#include "media/video/h264_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {
namespace mp4 {

static const uint8_t kNALU1[] = {0x01, 0x02, 0x03};
static const uint8_t kNALU2[] = {0x04, 0x05, 0x06, 0x07};
static const uint8_t kExpected[] = {0x00, 0x00, 0x00, 0x01, 0x01,
                                    0x02, 0x03, 0x00, 0x00, 0x00,
                                    0x01, 0x04, 0x05, 0x06, 0x07};

static const uint8_t kExpectedParamSets[] = {
    0x00, 0x00, 0x00, 0x01, 0x67, 0x12, 0x00, 0x00, 0x00, 0x01,
    0x67, 0x34, 0x00, 0x00, 0x00, 0x01, 0x68, 0x56, 0x78};

static H264NALU::Type StringToNALUType(const std::string& name) {
  if (name == "P")
    return H264NALU::kNonIDRSlice;

  if (name == "I")
    return H264NALU::kIDRSlice;

  if (name == "SEI")
    return H264NALU::kSEIMessage;

  if (name == "SPS")
    return H264NALU::kSPS;

  if (name == "SPSExt")
    return H264NALU::kSPSExt;

  if (name == "PPS")
    return H264NALU::kPPS;

  if (name == "AUD")
    return H264NALU::kAUD;

  if (name == "EOSeq")
    return H264NALU::kEOSeq;

  if (name == "EOStr")
    return H264NALU::kEOStream;

  if (name == "FILL")
    return H264NALU::kFiller;

  if (name == "R14")
    return H264NALU::kReserved14;

  CHECK(false) << "Unexpected name: " << name;
  return H264NALU::kUnspecified;
}

static std::string NALUTypeToString(int type) {
  switch (type) {
    case H264NALU::kNonIDRSlice:
      return "P";
    case H264NALU::kSliceDataA:
      return "SDA";
    case H264NALU::kSliceDataB:
      return "SDB";
    case H264NALU::kSliceDataC:
      return "SDC";
    case H264NALU::kIDRSlice:
      return "I";
    case H264NALU::kSEIMessage:
      return "SEI";
    case H264NALU::kSPS:
      return "SPS";
    case H264NALU::kSPSExt:
      return "SPSExt";
    case H264NALU::kPPS:
      return "PPS";
    case H264NALU::kAUD:
      return "AUD";
    case H264NALU::kEOSeq:
      return "EOSeq";
    case H264NALU::kEOStream:
      return "EOStr";
    case H264NALU::kFiller:
      return "FILL";
    case H264NALU::kReserved14:
      return "R14";

    case H264NALU::kUnspecified:
    case H264NALU::kReserved15:
    case H264NALU::kReserved16:
    case H264NALU::kReserved17:
    case H264NALU::kReserved18:
    case H264NALU::kCodedSliceAux:
    case H264NALU::kCodedSliceExtension:
      CHECK(false) << "Unexpected type: " << type;
      break;
  };

  return "UnsupportedType";
}

static void WriteStartCodeAndNALUType(std::vector<uint8_t>* buffer,
                                      const std::string& nal_unit_type) {
  buffer->push_back(0x00);
  buffer->push_back(0x00);
  buffer->push_back(0x00);
  buffer->push_back(0x01);
  buffer->push_back(StringToNALUType(nal_unit_type));
}

// Input string should be one or more NALU types separated with spaces or
// commas. NALU grouped together and separated by commas are placed into the
// same subsample, NALU groups separated by spaces are placed into separate
// subsamples.
// For example: input string "SPS PPS I" produces Annex B buffer containing
// SPS, PPS and I NALUs, each in a separate subsample. While input string
// "SPS,PPS I" produces Annex B buffer where the first subsample contains SPS
// and PPS NALUs and the second subsample contains the I-slice NALU.
// The output buffer will contain a valid-looking Annex B (it's valid-looking in
// the sense that it has start codes and correct NALU types, but the actual NALU
// payload is junk).
void StringToAnnexB(const std::string& str,
                    std::vector<uint8_t>* buffer,
                    std::vector<SubsampleEntry>* subsamples) {
  DCHECK(!str.empty());

  std::vector<std::string> subsample_specs = base::SplitString(
      str, " ", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  EXPECT_GT(subsample_specs.size(), 0u);

  buffer->clear();
  for (size_t i = 0; i < subsample_specs.size(); ++i) {
    SubsampleEntry entry;
    size_t start = buffer->size();

    std::vector<std::string> subsample_nalus = base::SplitString(
        subsample_specs[i], ",", base::KEEP_WHITESPACE,
        base::SPLIT_WANT_NONEMPTY);
    EXPECT_GT(subsample_nalus.size(), 0u);
    for (size_t j = 0; j < subsample_nalus.size(); ++j) {
      WriteStartCodeAndNALUType(buffer, subsample_nalus[j]);

      // Write junk for the payload since the current code doesn't
      // actually look at it.
      buffer->push_back(0x32);
      buffer->push_back(0x12);
      buffer->push_back(0x67);
    }

    entry.clear_bytes = buffer->size() - start;

    if (subsamples) {
      // Simulate the encrypted bits containing something that looks
      // like a SPS NALU.
      WriteStartCodeAndNALUType(buffer, "SPS");
    }

    entry.cypher_bytes = buffer->size() - start - entry.clear_bytes;

    if (subsamples) {
      subsamples->push_back(entry);
    }
  }
}

std::string AnnexBToString(const std::vector<uint8_t>& buffer,
                           const std::vector<SubsampleEntry>& subsamples) {
  std::stringstream ss;

  H264Parser parser;
  parser.SetEncryptedStream(&buffer[0], buffer.size(), subsamples);

  H264NALU nalu;
  bool first = true;
  size_t current_subsample_index = 0;
  while (parser.AdvanceToNextNALU(&nalu) == H264Parser::kOk) {
    size_t subsample_index = AVC::FindSubsampleIndex(buffer, &subsamples,
                                                     nalu.data);
    if (!first) {
      ss << (subsample_index == current_subsample_index ? "," : " ");
    } else {
      DCHECK_EQ(subsample_index, current_subsample_index);
      first = false;
    }

    ss << NALUTypeToString(nalu.nal_unit_type);
    current_subsample_index = subsample_index;
  }
  return ss.str();
}

class AVCConversionTest : public testing::TestWithParam<int> {
 protected:
  void WriteLength(int length_size, int length, std::vector<uint8_t>* buf) {
    DCHECK_GE(length, 0);
    DCHECK_LE(length, 255);

    for (int i = 1; i < length_size; i++)
      buf->push_back(0);
    buf->push_back(length);
  }

  void MakeInputForLength(int length_size, std::vector<uint8_t>* buf) {
    buf->clear();

    WriteLength(length_size, sizeof(kNALU1), buf);
    buf->insert(buf->end(), kNALU1, kNALU1 + sizeof(kNALU1));

    WriteLength(length_size, sizeof(kNALU2), buf);
    buf->insert(buf->end(), kNALU2, kNALU2 + sizeof(kNALU2));
  }

};

TEST_P(AVCConversionTest, ParseCorrectly) {
  std::vector<uint8_t> buf;
  std::vector<SubsampleEntry> subsamples;
  MakeInputForLength(GetParam(), &buf);
  EXPECT_TRUE(AVC::ConvertFrameToAnnexB(GetParam(), &buf, &subsamples));
  EXPECT_TRUE(AVC::IsValidAnnexB(buf, subsamples));
  EXPECT_EQ(buf.size(), sizeof(kExpected));
  EXPECT_EQ(0, memcmp(kExpected, &buf[0], sizeof(kExpected)));
  EXPECT_EQ("P,SDC", AnnexBToString(buf, subsamples));
}

// Intentionally write NALU sizes that are larger than the buffer.
TEST_P(AVCConversionTest, NALUSizeTooLarge) {
  std::vector<uint8_t> buf;
  WriteLength(GetParam(), 10 * sizeof(kNALU1), &buf);
  buf.insert(buf.end(), kNALU1, kNALU1 + sizeof(kNALU1));
  EXPECT_FALSE(AVC::ConvertFrameToAnnexB(GetParam(), &buf, nullptr));
}

TEST_P(AVCConversionTest, NALUSizeIsZero) {
  std::vector<uint8_t> buf;
  WriteLength(GetParam(), 0, &buf);

  WriteLength(GetParam(), sizeof(kNALU1), &buf);
  buf.insert(buf.end(), kNALU1, kNALU1 + sizeof(kNALU1));

  WriteLength(GetParam(), 0, &buf);

  WriteLength(GetParam(), sizeof(kNALU2), &buf);
  buf.insert(buf.end(), kNALU2, kNALU2 + sizeof(kNALU2));

  EXPECT_FALSE(AVC::ConvertFrameToAnnexB(GetParam(), &buf, nullptr));
}

TEST_P(AVCConversionTest, SubsampleSizesUpdatedAfterAnnexBConversion) {
  std::vector<uint8_t> buf;
  std::vector<SubsampleEntry> subsamples;
  SubsampleEntry subsample;

  // Write the first subsample, consisting of only one NALU
  WriteLength(GetParam(), sizeof(kNALU1), &buf);
  buf.insert(buf.end(), kNALU1, kNALU1 + sizeof(kNALU1));

  subsample.clear_bytes = GetParam() + sizeof(kNALU1);
  subsample.cypher_bytes = 0;
  subsamples.push_back(subsample);

  // Write the second subsample, containing two NALUs
  WriteLength(GetParam(), sizeof(kNALU1), &buf);
  buf.insert(buf.end(), kNALU1, kNALU1 + sizeof(kNALU1));
  WriteLength(GetParam(), sizeof(kNALU2), &buf);
  buf.insert(buf.end(), kNALU2, kNALU2 + sizeof(kNALU2));

  subsample.clear_bytes = 2*GetParam() + sizeof(kNALU1) + sizeof(kNALU2);
  subsample.cypher_bytes = 0;
  subsamples.push_back(subsample);

  // Write the third subsample, containing a single one-byte NALU
  WriteLength(GetParam(), 1, &buf);
  buf.push_back(0);
  subsample.clear_bytes = GetParam() + 1;
  subsample.cypher_bytes = 0;
  subsamples.push_back(subsample);

  EXPECT_TRUE(AVC::ConvertFrameToAnnexB(GetParam(), &buf, &subsamples));
  EXPECT_EQ(subsamples.size(), 3u);
  EXPECT_EQ(subsamples[0].clear_bytes, 4 + sizeof(kNALU1));
  EXPECT_EQ(subsamples[0].cypher_bytes, 0u);
  EXPECT_EQ(subsamples[1].clear_bytes, 8 + sizeof(kNALU1) + sizeof(kNALU2));
  EXPECT_EQ(subsamples[1].cypher_bytes, 0u);
  EXPECT_EQ(subsamples[2].clear_bytes, 4 + 1u);
  EXPECT_EQ(subsamples[2].cypher_bytes, 0u);
}

TEST_P(AVCConversionTest, ParsePartial) {
  std::vector<uint8_t> buf;
  MakeInputForLength(GetParam(), &buf);
  buf.pop_back();
  EXPECT_FALSE(AVC::ConvertFrameToAnnexB(GetParam(), &buf, nullptr));
  // This tests a buffer ending in the middle of a NAL length. For length size
  // of one, this can't happen, so we skip that case.
  if (GetParam() != 1) {
    MakeInputForLength(GetParam(), &buf);
    buf.erase(buf.end() - (sizeof(kNALU2) + 1), buf.end());
    EXPECT_FALSE(AVC::ConvertFrameToAnnexB(GetParam(), &buf, nullptr));
  }
}

TEST_P(AVCConversionTest, ParseEmpty) {
  std::vector<uint8_t> buf;
  EXPECT_TRUE(AVC::ConvertFrameToAnnexB(GetParam(), &buf, nullptr));
  EXPECT_EQ(0u, buf.size());
}

INSTANTIATE_TEST_CASE_P(AVCConversionTestValues,
                        AVCConversionTest,
                        ::testing::Values(1, 2, 4));

TEST_F(AVCConversionTest, ConvertConfigToAnnexB) {
  AVCDecoderConfigurationRecord avc_config;
  avc_config.sps_list.resize(2);
  avc_config.sps_list[0].push_back(0x67);
  avc_config.sps_list[0].push_back(0x12);
  avc_config.sps_list[1].push_back(0x67);
  avc_config.sps_list[1].push_back(0x34);
  avc_config.pps_list.resize(1);
  avc_config.pps_list[0].push_back(0x68);
  avc_config.pps_list[0].push_back(0x56);
  avc_config.pps_list[0].push_back(0x78);

  std::vector<uint8_t> buf;
  std::vector<SubsampleEntry> subsamples;
  EXPECT_TRUE(AVC::ConvertConfigToAnnexB(avc_config, &buf));
  EXPECT_EQ(0, memcmp(kExpectedParamSets, &buf[0],
                      sizeof(kExpectedParamSets)));
  EXPECT_EQ("SPS,SPS,PPS", AnnexBToString(buf, subsamples));
}

// Verify that we can round trip string -> Annex B -> string.
TEST_F(AVCConversionTest, StringConversionFunctions) {
  std::string str =
      "AUD SPS SPSExt SPS PPS SEI SEI R14 I P FILL EOSeq EOStr";
  std::vector<uint8_t> buf;
  std::vector<SubsampleEntry> subsamples;
  StringToAnnexB(str, &buf, &subsamples);
  EXPECT_TRUE(AVC::IsValidAnnexB(buf, subsamples));

  EXPECT_EQ(str, AnnexBToString(buf, subsamples));
}

TEST_F(AVCConversionTest, ValidAnnexBConstructs) {
  const char* test_cases[] = {
    "I",
    "I I I I",
    "AUD I",
    "AUD SPS PPS I",
    "I EOSeq",
    "I EOSeq EOStr",
    "I EOStr",
    "P",
    "P P P P",
    "AUD SPS PPS P",
    "SEI SEI I",
    "SEI SEI R14 I",
    "SPS SPSExt SPS PPS I P",
    "R14 SEI I",
    "AUD,I",
    "AUD,SEI I",
    "AUD,SEI,SPS,PPS,I"
  };

  for (size_t i = 0; i < arraysize(test_cases); ++i) {
    std::vector<uint8_t> buf;
    std::vector<SubsampleEntry> subsamples;
    StringToAnnexB(test_cases[i], &buf, NULL);
    EXPECT_TRUE(AVC::IsValidAnnexB(buf, subsamples)) << "'" << test_cases[i]
                                                     << "' failed";
  }
}

TEST_F(AVCConversionTest, InvalidAnnexBConstructs) {
  static const char* test_cases[] = {
    "AUD",  // No VCL present.
    "AUD,SEI", // No VCL present.
    "SPS PPS",  // No VCL present.
    "SPS PPS AUD I",  // Parameter sets must come after AUD.
    "SPSExt SPS P",  // SPS must come before SPSExt.
    "SPS PPS SPSExt P",  // SPSExt must follow an SPS.
    "EOSeq",  // EOSeq must come after a VCL.
    "EOStr",  // EOStr must come after a VCL.
    "I EOStr EOSeq",  // EOSeq must come before EOStr.
    "I R14",  // Reserved14-18 must come before first VCL.
    "I SEI",  // SEI must come before first VCL.
    "P SPS P", // SPS after first VCL would indicate a new access unit.
  };

  for (size_t i = 0; i < arraysize(test_cases); ++i) {
    std::vector<uint8_t> buf;
    std::vector<SubsampleEntry> subsamples;
    StringToAnnexB(test_cases[i], &buf, NULL);
    EXPECT_FALSE(AVC::IsValidAnnexB(buf, subsamples)) << "'" << test_cases[i]
                                                      << "' failed";
  }
}

typedef struct {
  const char* input;
  const char* expected;
} InsertTestCases;

TEST_F(AVCConversionTest, InsertParamSetsAnnexB) {
  static const InsertTestCases test_cases[] = {
    { "I", "SPS,SPS,PPS,I" },
    { "AUD I", "AUD SPS,SPS,PPS,I" },

    // Cases where param sets in |avc_config| are placed before
    // the existing ones.
    { "SPS,PPS,I", "SPS,SPS,PPS,SPS,PPS,I" },
    { "AUD,SPS,PPS,I", "AUD,SPS,SPS,PPS,SPS,PPS,I" },  // Note: params placed
                                                       // after AUD.

    // One or more NALUs might follow AUD in the first subsample, we need to
    // handle this correctly. Params should be inserted right after AUD.
    { "AUD,SEI I", "AUD,SPS,SPS,PPS,SEI I" },
  };

  AVCDecoderConfigurationRecord avc_config;
  avc_config.sps_list.resize(2);
  avc_config.sps_list[0].push_back(0x67);
  avc_config.sps_list[0].push_back(0x12);
  avc_config.sps_list[1].push_back(0x67);
  avc_config.sps_list[1].push_back(0x34);
  avc_config.pps_list.resize(1);
  avc_config.pps_list[0].push_back(0x68);
  avc_config.pps_list[0].push_back(0x56);
  avc_config.pps_list[0].push_back(0x78);

  for (size_t i = 0; i < arraysize(test_cases); ++i) {
    std::vector<uint8_t> buf;
    std::vector<SubsampleEntry> subsamples;

    StringToAnnexB(test_cases[i].input, &buf, &subsamples);

    EXPECT_TRUE(AVC::InsertParamSetsAnnexB(avc_config, &buf, &subsamples))
        << "'" << test_cases[i].input << "' insert failed.";
    EXPECT_TRUE(AVC::IsValidAnnexB(buf, subsamples))
        << "'" << test_cases[i].input << "' created invalid AnnexB.";
    EXPECT_EQ(test_cases[i].expected, AnnexBToString(buf, subsamples))
        << "'" << test_cases[i].input << "' generated unexpected output.";
  }
}

}  // namespace mp4
}  // namespace media
