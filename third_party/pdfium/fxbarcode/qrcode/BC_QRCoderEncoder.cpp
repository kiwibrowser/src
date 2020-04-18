// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
// Original code is licensed as follows:
/*
 * Copyright 2008 ZXing authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fxbarcode/qrcode/BC_QRCoderEncoder.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "fxbarcode/BC_UtilCodingConvert.h"
#include "fxbarcode/common/BC_CommonByteArray.h"
#include "fxbarcode/common/BC_CommonByteMatrix.h"
#include "fxbarcode/common/reedsolomon/BC_ReedSolomon.h"
#include "fxbarcode/common/reedsolomon/BC_ReedSolomonGF256.h"
#include "fxbarcode/qrcode/BC_QRCoder.h"
#include "fxbarcode/qrcode/BC_QRCoderBitVector.h"
#include "fxbarcode/qrcode/BC_QRCoderBlockPair.h"
#include "fxbarcode/qrcode/BC_QRCoderECBlocks.h"
#include "fxbarcode/qrcode/BC_QRCoderMaskUtil.h"
#include "fxbarcode/qrcode/BC_QRCoderMatrixUtil.h"
#include "fxbarcode/qrcode/BC_QRCoderMode.h"
#include "fxbarcode/qrcode/BC_QRCoderVersion.h"
#include "third_party/base/ptr_util.h"

using ModeStringPair = std::pair<CBC_QRCoderMode*, ByteString>;

namespace {

// This is a mapping for an ASCII table, starting at an index of 32.
const int8_t g_alphaNumericTable[] = {
    36, -1, -1, -1, 37, 38, -1, -1, -1, -1, 39, 40, -1, 41, 42, 43,  // 32-47
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  44, -1, -1, -1, -1, -1,  // 48-63
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,  // 64-79
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};

int32_t GetAlphaNumericCode(int32_t code) {
  if (code < 32)
    return -1;
  size_t code_index = static_cast<size_t>(code - 32);
  if (code_index >= FX_ArraySize(g_alphaNumericTable))
    return -1;
  return g_alphaNumericTable[code_index];
}

void AppendNumericBytes(const ByteString& content,
                        CBC_QRCoderBitVector* bits,
                        int32_t& e) {
  int32_t length = content.GetLength();
  int32_t i = 0;
  while (i < length) {
    int32_t num1 = content[i] - '0';
    if (i + 2 < length) {
      int32_t num2 = content[i + 1] - '0';
      int32_t num3 = content[i + 2] - '0';
      bits->AppendBits(num1 * 100 + num2 * 10 + num3, 10);
      i += 3;
    } else if (i + 1 < length) {
      int32_t num2 = content[i + 1] - '0';
      bits->AppendBits(num1 * 10 + num2, 7);
      i += 2;
    } else {
      bits->AppendBits(num1, 4);
      i++;
    }
  }
}

void AppendAlphaNumericBytes(const ByteString& content,
                             CBC_QRCoderBitVector* bits,
                             int32_t& e) {
  int32_t length = content.GetLength();
  int32_t i = 0;
  while (i < length) {
    int32_t code1 = GetAlphaNumericCode(content[i]);
    if (code1 == -1) {
      e = BCExceptionInvalidateCharacter;
      return;
    }
    if (i + 1 < length) {
      int32_t code2 = GetAlphaNumericCode(content[i + 1]);
      if (code2 == -1) {
        e = BCExceptionInvalidateCharacter;
        return;
      }
      bits->AppendBits(code1 * 45 + code2, 11);
      i += 2;
    } else {
      bits->AppendBits(code1, 6);
      i++;
    }
  }
}

void AppendGBKBytes(const ByteString& content,
                    CBC_QRCoderBitVector* bits,
                    int32_t& e) {
  int32_t length = content.GetLength();
  uint32_t value = 0;
  for (int32_t i = 0; i < length; i += 2) {
    value = (uint32_t)(content[i] << 8 | content[i + 1]);
    if (value <= 0xAAFE && value >= 0xA1A1) {
      value -= 0xA1A1;
    } else if (value <= 0xFAFE && value >= 0xB0A1) {
      value -= 0xA6A1;
    } else {
      e = BCExceptionInvalidateCharacter;
      return;
    }
    value = (uint32_t)((value >> 8) * 0x60) + (uint32_t)(value & 0xff);
    bits->AppendBits(value, 13);
  }
}

void Append8BitBytes(const ByteString& content,
                     CBC_QRCoderBitVector* bits,
                     ByteString encoding,
                     int32_t& e) {
  for (size_t i = 0; i < content.GetLength(); i++)
    bits->AppendBits(content[i], 8);
}

void AppendKanjiBytes(const ByteString& content,
                      CBC_QRCoderBitVector* bits,
                      int32_t& e) {
  std::vector<uint8_t> bytes;
  uint32_t value = 0;
  for (size_t i = 0; i < bytes.size(); i += 2) {
    value = (uint32_t)((content[i] << 8) | content[i + 1]);
    if (value <= 0x9ffc && value >= 0x8140) {
      value -= 0x8140;
    } else if (value <= 0xebbf && value >= 0xe040) {
      value -= 0xc140;
    } else {
      e = BCExceptionInvalidateCharacter;
      return;
    }
    value = (uint32_t)((value >> 8) * 0xc0) + (uint32_t)(value & 0xff);
    bits->AppendBits(value, 13);
  }
}

void AppendModeInfo(CBC_QRCoderMode* mode, CBC_QRCoderBitVector* bits) {
  bits->AppendBits(mode->GetBits(), 4);
  if (mode == CBC_QRCoderMode::sGBK)
    bits->AppendBits(1, 4);
}

bool AppendLengthInfo(int32_t numLetters,
                      int32_t version,
                      CBC_QRCoderMode* mode,
                      CBC_QRCoderBitVector* bits) {
  int32_t e = BCExceptionNO;
  const auto* qcv = CBC_QRCoderVersion::GetVersionForNumber(version);
  if (!qcv)
    return false;
  int32_t numBits = mode->GetCharacterCountBits(qcv->GetVersionNumber(), e);
  if (e != BCExceptionNO)
    return false;
  if (numBits > ((1 << numBits) - 1))
    return true;

  if (mode == CBC_QRCoderMode::sGBK)
    bits->AppendBits(numLetters / 2, numBits);
  bits->AppendBits(numLetters, numBits);
  return true;
}

void AppendBytes(const ByteString& content,
                 CBC_QRCoderMode* mode,
                 CBC_QRCoderBitVector* bits,
                 ByteString encoding,
                 int32_t& e) {
  if (mode == CBC_QRCoderMode::sNUMERIC)
    AppendNumericBytes(content, bits, e);
  else if (mode == CBC_QRCoderMode::sALPHANUMERIC)
    AppendAlphaNumericBytes(content, bits, e);
  else if (mode == CBC_QRCoderMode::sBYTE)
    Append8BitBytes(content, bits, encoding, e);
  else if (mode == CBC_QRCoderMode::sKANJI)
    AppendKanjiBytes(content, bits, e);
  else if (mode == CBC_QRCoderMode::sGBK)
    AppendGBKBytes(content, bits, e);
  else
    e = BCExceptionUnsupportedMode;
}

bool InitQRCode(int32_t numInputBytes,
                const CBC_QRCoderErrorCorrectionLevel* ecLevel,
                CBC_QRCoderMode* mode,
                CBC_QRCoder* qrCode) {
  qrCode->SetECLevel(ecLevel);
  qrCode->SetMode(mode);
  for (int32_t i = 1; i <= CBC_QRCoderVersion::kMaxVersion; ++i) {
    const auto* version = CBC_QRCoderVersion::GetVersionForNumber(i);
    int32_t numBytes = version->GetTotalCodeWords();
    const auto* ecBlocks = version->GetECBlocksForLevel(*ecLevel);
    int32_t numEcBytes = ecBlocks->GetTotalECCodeWords();
    int32_t numRSBlocks = ecBlocks->GetNumBlocks();
    int32_t numDataBytes = numBytes - numEcBytes;
    if (numDataBytes >= numInputBytes + 3) {
      qrCode->SetVersion(i);
      qrCode->SetNumTotalBytes(numBytes);
      qrCode->SetNumDataBytes(numDataBytes);
      qrCode->SetNumRSBlocks(numRSBlocks);
      qrCode->SetNumECBytes(numEcBytes);
      qrCode->SetMatrixWidth(version->GetDimensionForVersion());
      return true;
    }
  }
  return false;
}

std::unique_ptr<CBC_CommonByteArray> GenerateECBytes(
    CBC_CommonByteArray* dataBytes,
    int32_t numEcBytesInBlock) {
  int32_t numDataBytes = dataBytes->Size();
  std::vector<int32_t> toEncode(numDataBytes + numEcBytesInBlock);
  for (int32_t i = 0; i < numDataBytes; ++i)
    toEncode[i] = dataBytes->At(i);
  CBC_ReedSolomonEncoder encode(CBC_ReedSolomonGF256::QRCodeField);
  encode.Init();
  if (!encode.Encode(&toEncode, numEcBytesInBlock))
    return nullptr;
  auto ecBytes = pdfium::MakeUnique<CBC_CommonByteArray>(numEcBytesInBlock);
  for (int32_t i = 0; i < numEcBytesInBlock; ++i)
    ecBytes->Set(i, toEncode[numDataBytes + i]);
  return ecBytes;
}

int32_t GetSpanByVersion(CBC_QRCoderMode* modeFirst,
                         CBC_QRCoderMode* modeSecond,
                         int32_t versionNum,
                         int32_t& e) {
  if (versionNum == 0)
    return 0;

  if (modeFirst == CBC_QRCoderMode::sALPHANUMERIC &&
      modeSecond == CBC_QRCoderMode::sBYTE) {
    if (versionNum >= 1 && versionNum <= 9)
      return 11;
    if (versionNum >= 10 && versionNum <= 26)
      return 15;
    if (versionNum >= 27 && versionNum <= CBC_QRCoderVersion::kMaxVersion)
      return 16;
    e = BCExceptionNoSuchVersion;
    return 0;
  }
  if (modeSecond == CBC_QRCoderMode::sALPHANUMERIC &&
      modeFirst == CBC_QRCoderMode::sNUMERIC) {
    if (versionNum >= 1 && versionNum <= 9)
      return 13;
    if (versionNum >= 10 && versionNum <= 26)
      return 15;
    if (versionNum >= 27 && versionNum <= CBC_QRCoderVersion::kMaxVersion)
      return 17;
    e = BCExceptionNoSuchVersion;
    return 0;
  }
  if (modeSecond == CBC_QRCoderMode::sBYTE &&
      modeFirst == CBC_QRCoderMode::sNUMERIC) {
    if (versionNum >= 1 && versionNum <= 9)
      return 6;
    if (versionNum >= 10 && versionNum <= 26)
      return 8;
    if (versionNum >= 27 && versionNum <= CBC_QRCoderVersion::kMaxVersion)
      return 9;
    e = BCExceptionNoSuchVersion;
    return 0;
  }
  return -1;
}

int32_t CalculateMaskPenalty(CBC_CommonByteMatrix* matrix) {
  return CBC_QRCoderMaskUtil::ApplyMaskPenaltyRule1(matrix) +
         CBC_QRCoderMaskUtil::ApplyMaskPenaltyRule2(matrix) +
         CBC_QRCoderMaskUtil::ApplyMaskPenaltyRule3(matrix) +
         CBC_QRCoderMaskUtil::ApplyMaskPenaltyRule4(matrix);
}

int32_t ChooseMaskPattern(CBC_QRCoderBitVector* bits,
                          const CBC_QRCoderErrorCorrectionLevel* ecLevel,
                          int32_t version,
                          CBC_CommonByteMatrix* matrix,
                          int32_t& e) {
  int32_t minPenalty = 65535;
  int32_t bestMaskPattern = -1;
  for (int32_t maskPattern = 0; maskPattern < CBC_QRCoder::kNumMaskPatterns;
       maskPattern++) {
    CBC_QRCoderMatrixUtil::BuildMatrix(bits, ecLevel, version, maskPattern,
                                       matrix, e);
    if (e != BCExceptionNO)
      return 0;
    int32_t penalty = CalculateMaskPenalty(matrix);
    if (penalty < minPenalty) {
      minPenalty = penalty;
      bestMaskPattern = maskPattern;
    }
  }
  return bestMaskPattern;
}

void GetNumDataBytesAndNumECBytesForBlockID(int32_t numTotalBytes,
                                            int32_t numDataBytes,
                                            int32_t numRSBlocks,
                                            int32_t blockID,
                                            int32_t& numDataBytesInBlock,
                                            int32_t& numECBytesInBlock) {
  if (blockID >= numRSBlocks)
    return;

  int32_t numRsBlocksInGroup2 = numTotalBytes % numRSBlocks;
  int32_t numRsBlocksInGroup1 = numRSBlocks - numRsBlocksInGroup2;
  int32_t numTotalBytesInGroup1 = numTotalBytes / numRSBlocks;
  int32_t numTotalBytesInGroup2 = numTotalBytesInGroup1 + 1;
  int32_t numDataBytesInGroup1 = numDataBytes / numRSBlocks;
  int32_t numDataBytesInGroup2 = numDataBytesInGroup1 + 1;
  int32_t numEcBytesInGroup1 = numTotalBytesInGroup1 - numDataBytesInGroup1;
  int32_t numEcBytesInGroup2 = numTotalBytesInGroup2 - numDataBytesInGroup2;
  if (blockID < numRsBlocksInGroup1) {
    numDataBytesInBlock = numDataBytesInGroup1;
    numECBytesInBlock = numEcBytesInGroup1;
  } else {
    numDataBytesInBlock = numDataBytesInGroup2;
    numECBytesInBlock = numEcBytesInGroup2;
  }
}

bool TerminateBits(int32_t numDataBytes, CBC_QRCoderBitVector* bits) {
  size_t capacity = numDataBytes << 3;
  if (bits->Size() > capacity)
    return false;

  for (int32_t i = 0; i < 4 && bits->Size() < capacity; ++i)
    bits->AppendBit(0);

  int32_t numBitsInLastByte = bits->Size() % 8;
  if (numBitsInLastByte > 0) {
    int32_t numPaddingBits = 8 - numBitsInLastByte;
    for (int32_t j = 0; j < numPaddingBits; ++j)
      bits->AppendBit(0);
  }

  if (bits->Size() % 8 != 0)
    return false;

  int32_t numPaddingBytes = numDataBytes - bits->sizeInBytes();
  for (int32_t k = 0; k < numPaddingBytes; ++k)
    bits->AppendBits(k % 2 ? 0x11 : 0xec, 8);
  return bits->Size() == capacity;
}

void MergeString(std::vector<ModeStringPair>* result,
                 int32_t versionNum,
                 int32_t& e) {
  size_t mergeNum = 0;
  for (size_t i = 0; i + 1 < result->size(); i++) {
    auto& element1 = (*result)[i];
    auto& element2 = (*result)[i + 1];
    if (element1.first == CBC_QRCoderMode::sALPHANUMERIC) {
      int32_t tmp = GetSpanByVersion(CBC_QRCoderMode::sALPHANUMERIC,
                                     CBC_QRCoderMode::sBYTE, versionNum, e);
      if (e != BCExceptionNO)
        return;
      if (element2.first == CBC_QRCoderMode::sBYTE && tmp >= 0 &&
          element1.second.GetLength() < static_cast<size_t>(tmp)) {
        element2.second = element1.second + element2.second;
        result->erase(result->begin() + i);
        i--;
        mergeNum++;
      }
    } else if (element1.first == CBC_QRCoderMode::sBYTE) {
      if (element2.first == CBC_QRCoderMode::sBYTE) {
        element1.second += element2.second;
        result->erase(result->begin() + i + 1);
        i--;
        mergeNum++;
      }
    } else if (element1.first == CBC_QRCoderMode::sNUMERIC) {
      int32_t tmp = GetSpanByVersion(CBC_QRCoderMode::sNUMERIC,
                                     CBC_QRCoderMode::sBYTE, versionNum, e);
      if (e != BCExceptionNO)
        return;
      if (element2.first == CBC_QRCoderMode::sBYTE && tmp >= 0 &&
          element1.second.GetLength() < static_cast<size_t>(tmp)) {
        element2.second = element1.second + element2.second;
        result->erase(result->begin() + i);
        i--;
        mergeNum++;
      }
      tmp = GetSpanByVersion(CBC_QRCoderMode::sNUMERIC,
                             CBC_QRCoderMode::sALPHANUMERIC, versionNum, e);
      if (e != BCExceptionNO)
        return;
      if (element2.first == CBC_QRCoderMode::sALPHANUMERIC && tmp >= 0 &&
          element1.second.GetLength() < static_cast<size_t>(tmp)) {
        element2.second = element1.second + element2.second;
        result->erase(result->begin() + i);
        i--;
        mergeNum++;
      }
    }
  }
  if (mergeNum != 0)
    MergeString(result, versionNum, e);
}

void SplitString(const ByteString& content,
                 std::vector<ModeStringPair>* result) {
  size_t index = 0;
  while (index < content.GetLength()) {
    uint8_t c = static_cast<uint8_t>(content[index]);
    if (!((c >= 0xA1 && c <= 0xAA) || (c >= 0xB0 && c <= 0xFA)))
      break;
    index += 2;
  }
  if (index)
    result->push_back({CBC_QRCoderMode::sGBK, content.Left(index)});
  if (index >= content.GetLength())
    return;

  size_t flag = index;
  while (GetAlphaNumericCode(content[index]) == -1 &&
         index < content.GetLength()) {
    uint8_t c = static_cast<uint8_t>(content[index]);
    if (((c >= 0xA1 && c <= 0xAA) || (c >= 0xB0 && c <= 0xFA)))
      break;
#if _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
    bool high = !!IsDBCSLeadByte(content[index]);
#else
    bool high = content[index] > 127;
#endif
    ++index;
    if (high)
      ++index;
  }
  if (index != flag) {
    result->push_back(
        {CBC_QRCoderMode::sBYTE, content.Mid(flag, index - flag)});
  }
  flag = index;
  if (index >= content.GetLength())
    return;

  while (index < content.GetLength() && isdigit(content[index]))
    ++index;

  if (index != flag) {
    result->push_back(
        {CBC_QRCoderMode::sNUMERIC, content.Mid(flag, index - flag)});
  }
  flag = index;
  if (index >= content.GetLength())
    return;

  while (index < content.GetLength() &&
         GetAlphaNumericCode(content[index]) != -1) {
    ++index;
  }
  if (index != flag) {
    result->push_back(
        {CBC_QRCoderMode::sALPHANUMERIC, content.Mid(flag, index - flag)});
  }
  flag = index;
  if (index < content.GetLength())
    SplitString(content.Right(content.GetLength() - index), result);
}

CBC_QRCoderMode* ChooseMode(const ByteString& content, ByteString encoding) {
  if (encoding.Compare("SHIFT_JIS") == 0)
    return CBC_QRCoderMode::sKANJI;

  bool hasNumeric = false;
  bool hasAlphaNumeric = false;
  for (size_t i = 0; i < content.GetLength(); i++) {
    if (isdigit(content[i])) {
      hasNumeric = true;
    } else if (GetAlphaNumericCode(content[i]) != -1) {
      hasAlphaNumeric = true;
    } else {
      return CBC_QRCoderMode::sBYTE;
    }
  }
  if (hasAlphaNumeric)
    return CBC_QRCoderMode::sALPHANUMERIC;
  if (hasNumeric)
    return CBC_QRCoderMode::sNUMERIC;
  return CBC_QRCoderMode::sBYTE;
}

bool InterleaveWithECBytes(CBC_QRCoderBitVector* bits,
                           int32_t numTotalBytes,
                           int32_t numDataBytes,
                           int32_t numRSBlocks,
                           CBC_QRCoderBitVector* result) {
  ASSERT(numTotalBytes >= 0);
  ASSERT(numDataBytes >= 0);
  if (bits->sizeInBytes() != static_cast<size_t>(numDataBytes))
    return false;

  int32_t dataBytesOffset = 0;
  int32_t maxNumDataBytes = 0;
  int32_t maxNumEcBytes = 0;
  std::vector<CBC_QRCoderBlockPair> blocks(numRSBlocks);
  for (int32_t i = 0; i < numRSBlocks; i++) {
    int32_t numDataBytesInBlock;
    int32_t numEcBytesInBlosk;
    GetNumDataBytesAndNumECBytesForBlockID(numTotalBytes, numDataBytes,
                                           numRSBlocks, i, numDataBytesInBlock,
                                           numEcBytesInBlosk);
    auto dataBytes = pdfium::MakeUnique<CBC_CommonByteArray>();
    dataBytes->Set(bits->GetArray(), dataBytesOffset, numDataBytesInBlock);
    std::unique_ptr<CBC_CommonByteArray> ecBytes =
        GenerateECBytes(dataBytes.get(), numEcBytesInBlosk);
    if (!ecBytes)
      return false;

    maxNumDataBytes = std::max(maxNumDataBytes, dataBytes->Size());
    maxNumEcBytes = std::max(maxNumEcBytes, ecBytes->Size());
    blocks[i].SetData(std::move(dataBytes), std::move(ecBytes));
    dataBytesOffset += numDataBytesInBlock;
  }
  if (numDataBytes != dataBytesOffset)
    return false;

  for (int32_t x = 0; x < maxNumDataBytes; x++) {
    for (size_t j = 0; j < blocks.size(); j++) {
      const CBC_CommonByteArray* dataBytes = blocks[j].GetDataBytes();
      if (x < dataBytes->Size())
        result->AppendBits(dataBytes->At(x), 8);
    }
  }
  for (int32_t y = 0; y < maxNumEcBytes; y++) {
    for (size_t l = 0; l < blocks.size(); l++) {
      const CBC_CommonByteArray* ecBytes = blocks[l].GetErrorCorrectionBytes();
      if (y < ecBytes->Size())
        result->AppendBits(ecBytes->At(y), 8);
    }
  }
  return static_cast<size_t>(numTotalBytes) == result->sizeInBytes();
}

}  // namespace

CBC_QRCoderEncoder::CBC_QRCoderEncoder() {}

CBC_QRCoderEncoder::~CBC_QRCoderEncoder() {}

// static
bool CBC_QRCoderEncoder::Encode(const WideString& content,
                                const CBC_QRCoderErrorCorrectionLevel* ecLevel,
                                CBC_QRCoder* qrCode) {
  ByteString encoding = "utf8";
  ByteString utf8Data;
  CBC_UtilCodingConvert::UnicodeToUTF8(content, utf8Data);
  CBC_QRCoderMode* mode = ChooseMode(utf8Data, encoding);
  CBC_QRCoderBitVector dataBits;
  int32_t e = BCExceptionNO;
  AppendBytes(utf8Data, mode, &dataBits, encoding, e);
  if (e != BCExceptionNO)
    return false;
  int32_t numInputBytes = dataBits.sizeInBytes();
  if (!InitQRCode(numInputBytes, ecLevel, mode, qrCode))
    return false;
  CBC_QRCoderBitVector headerAndDataBits;
  AppendModeInfo(mode, &headerAndDataBits);
  int32_t numLetters = mode == CBC_QRCoderMode::sBYTE ? dataBits.sizeInBytes()
                                                      : content.GetLength();
  if (!AppendLengthInfo(numLetters, qrCode->GetVersion(), mode,
                        &headerAndDataBits)) {
    return false;
  }
  headerAndDataBits.AppendBitVector(&dataBits);
  if (!TerminateBits(qrCode->GetNumDataBytes(), &headerAndDataBits))
    return false;
  CBC_QRCoderBitVector finalBits;
  if (!InterleaveWithECBytes(&headerAndDataBits, qrCode->GetNumTotalBytes(),
                             qrCode->GetNumDataBytes(),
                             qrCode->GetNumRSBlocks(), &finalBits)) {
    return false;
  }

  auto matrix = pdfium::MakeUnique<CBC_CommonByteMatrix>(
      qrCode->GetMatrixWidth(), qrCode->GetMatrixWidth());
  matrix->Init();
  int32_t maskPattern = ChooseMaskPattern(
      &finalBits, qrCode->GetECLevel(), qrCode->GetVersion(), matrix.get(), e);
  if (e != BCExceptionNO)
    return false;

  qrCode->SetMaskPattern(maskPattern);
  CBC_QRCoderMatrixUtil::BuildMatrix(&finalBits, qrCode->GetECLevel(),
                                     qrCode->GetVersion(),
                                     qrCode->GetMaskPattern(), matrix.get(), e);
  if (e != BCExceptionNO)
    return false;

  qrCode->SetMatrix(std::move(matrix));
  return qrCode->IsValid();
}
