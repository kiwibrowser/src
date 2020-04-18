// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
// Original code is licensed as follows:
/*
 * Copyright 2006 Jeremias Maerki.
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

#include "fxbarcode/datamatrix/BC_ErrorCorrection.h"

#include <vector>

#include "fxbarcode/datamatrix/BC_Encoder.h"
#include "fxbarcode/datamatrix/BC_SymbolInfo.h"
#include "fxbarcode/utils.h"

namespace {

const uint8_t FACTOR_SETS[] = {5,  7,  10, 11, 12, 14, 18, 20,
                               24, 28, 36, 42, 48, 56, 62, 68};

const uint8_t FACTORS_0[5] = {228, 48, 15, 111, 62};
const uint8_t FACTORS_1[7] = {23, 68, 144, 134, 240, 92, 254};
const uint8_t FACTORS_2[10] = {28, 24, 185, 166, 223, 248, 116, 255, 110, 61};
const uint8_t FACTORS_3[11] = {175, 138, 205, 12, 194, 168,
                               39,  245, 60,  97, 120};

const uint8_t FACTORS_4[12] = {41,  153, 158, 91,  61,  42,
                               142, 213, 97,  178, 100, 242};

const uint8_t FACTORS_5[14] = {156, 97,  192, 252, 95,  9,  157,
                               119, 138, 45,  18,  186, 83, 185};

const uint8_t FACTORS_6[18] = {83,  195, 100, 39, 188, 75,  66, 61, 241,
                               213, 109, 129, 94, 254, 225, 48, 90, 188};

const uint8_t FACTORS_7[20] = {15,  195, 244, 9,  233, 71, 168, 2,   188, 160,
                               153, 145, 253, 79, 108, 82, 27,  174, 186, 172};

const uint8_t FACTORS_8[24] = {52,  190, 88,  205, 109, 39, 176, 21,
                               155, 197, 251, 223, 155, 21, 5,   172,
                               254, 124, 12,  181, 184, 96, 50,  193};

const uint8_t FACTORS_9[28] = {211, 231, 43,  97,  71,  96,  103, 174, 37,  151,
                               170, 53,  75,  34,  249, 121, 17,  138, 110, 213,
                               141, 136, 120, 151, 233, 168, 93,  255};

const uint8_t FACTORS_10[36] = {245, 127, 242, 218, 130, 250, 162, 181, 102,
                                120, 84,  179, 220, 251, 80,  182, 229, 18,
                                2,   4,   68,  33,  101, 137, 95,  119, 115,
                                44,  175, 184, 59,  25,  225, 98,  81,  112};

const uint8_t FACTORS_11[42] = {
    77,  193, 137, 31,  19,  38,  22,  153, 247, 105, 122, 2,   245, 133,
    242, 8,   175, 95,  100, 9,   167, 105, 214, 111, 57,  121, 21,  1,
    253, 57,  54,  101, 248, 202, 69,  50,  150, 177, 226, 5,   9,   5};

const uint8_t FACTORS_12[48] = {
    245, 132, 172, 223, 96,  32,  117, 22,  238, 133, 238, 231,
    205, 188, 237, 87,  191, 106, 16,  147, 118, 23,  37,  90,
    170, 205, 131, 88,  120, 100, 66,  138, 186, 240, 82,  44,
    176, 87,  187, 147, 160, 175, 69,  213, 92,  253, 225, 19};

const uint8_t FACTORS_13[56] = {
    175, 9,   223, 238, 12,  17,  220, 208, 100, 29,  175, 170, 230, 192,
    215, 235, 150, 159, 36,  223, 38,  200, 132, 54,  228, 146, 218, 234,
    117, 203, 29,  232, 144, 238, 22,  150, 201, 117, 62,  207, 164, 13,
    137, 245, 127, 67,  247, 28,  155, 43,  203, 107, 233, 53,  143, 46};

const uint8_t FACTORS_14[62] = {
    242, 93,  169, 50,  144, 210, 39,  118, 202, 188, 201, 189, 143,
    108, 196, 37,  185, 112, 134, 230, 245, 63,  197, 190, 250, 106,
    185, 221, 175, 64,  114, 71,  161, 44,  147, 6,   27,  218, 51,
    63,  87,  10,  40,  130, 188, 17,  163, 31,  176, 170, 4,   107,
    232, 7,   94,  166, 224, 124, 86,  47,  11,  204};

const uint8_t FACTORS_15[68] = {
    220, 228, 173, 89,  251, 149, 159, 56,  89,  33,  147, 244, 154, 36,
    73,  127, 213, 136, 248, 180, 234, 197, 158, 177, 68,  122, 93,  213,
    15,  160, 227, 236, 66,  139, 153, 185, 202, 167, 179, 25,  220, 232,
    96,  210, 231, 136, 223, 239, 181, 241, 59,  52,  172, 25,  49,  232,
    211, 189, 64,  54,  108, 153, 132, 63,  96,  103, 82,  186};

const uint8_t* const FACTORS[16] = {
    FACTORS_0,  FACTORS_1,  FACTORS_2,  FACTORS_3, FACTORS_4,  FACTORS_5,
    FACTORS_6,  FACTORS_7,  FACTORS_8,  FACTORS_9, FACTORS_10, FACTORS_11,
    FACTORS_12, FACTORS_13, FACTORS_14, FACTORS_15};

}  // namespace

int32_t CBC_ErrorCorrection::LOG[256] = {0};
int32_t CBC_ErrorCorrection::ALOG[256] = {0};

void CBC_ErrorCorrection::Initialize() {
  int32_t p = 1;
  for (int32_t i = 0; i < 255; i++) {
    ALOG[i] = p;
    LOG[p] = i;
    p <<= 1;
    if (p >= 256) {
      p ^= MODULO_VALUE;
    }
  }
}
void CBC_ErrorCorrection::Finalize() {}
CBC_ErrorCorrection::CBC_ErrorCorrection() {}
CBC_ErrorCorrection::~CBC_ErrorCorrection() {}
WideString CBC_ErrorCorrection::encodeECC200(WideString codewords,
                                             CBC_SymbolInfo* symbolInfo,
                                             int32_t& e) {
  if (pdfium::base::checked_cast<int32_t>(codewords.GetLength()) !=
      symbolInfo->dataCapacity()) {
    e = BCExceptionIllegalArgument;
    return WideString();
  }
  WideString sb;
  sb += codewords;
  int32_t blockCount = symbolInfo->getInterleavedBlockCount();
  if (blockCount == 1) {
    WideString ecc = createECCBlock(codewords, symbolInfo->errorCodewords(), e);
    if (e != BCExceptionNO)
      return WideString();
    sb += ecc;
  } else {
    std::vector<int32_t> dataSizes(blockCount);
    std::vector<int32_t> errorSizes(blockCount);
    std::vector<int32_t> startPos(blockCount);
    for (int32_t i = 0; i < blockCount; i++) {
      dataSizes[i] = symbolInfo->getDataLengthForInterleavedBlock(i + 1);
      errorSizes[i] = symbolInfo->getErrorLengthForInterleavedBlock(i + 1);
      startPos[i] = 0;
      if (i > 0) {
        startPos[i] = startPos[i - 1] + dataSizes[i];
      }
    }
    for (int32_t block = 0; block < blockCount; block++) {
      WideString temp;
      for (int32_t d = block; d < symbolInfo->dataCapacity(); d += blockCount) {
        temp += (wchar_t)codewords[d];
      }
      WideString ecc = createECCBlock(temp, errorSizes[block], e);
      if (e != BCExceptionNO)
        return WideString();
      int32_t pos = 0;
      for (int32_t l = block; l < errorSizes[block] * blockCount;
           l += blockCount) {
        sb.SetAt(symbolInfo->dataCapacity() + l, ecc[pos++]);
      }
    }
  }
  return sb;
}
WideString CBC_ErrorCorrection::createECCBlock(WideString codewords,
                                               int32_t numECWords,
                                               int32_t& e) {
  return createECCBlock(codewords, 0, codewords.GetLength(), numECWords, e);
}
WideString CBC_ErrorCorrection::createECCBlock(WideString codewords,
                                               int32_t start,
                                               int32_t len,
                                               int32_t numECWords,
                                               int32_t& e) {
  static const size_t kFactorTableNum = sizeof(FACTOR_SETS) / sizeof(int32_t);
  size_t table = 0;
  while (table < kFactorTableNum && FACTOR_SETS[table] != numECWords)
    table++;

  if (table >= kFactorTableNum) {
    e = BCExceptionIllegalArgument;
    return WideString();
  }
  uint16_t* ecc = FX_Alloc(uint16_t, numECWords);
  memset(ecc, 0, numECWords * sizeof(uint16_t));
  for (int32_t l = start; l < start + len; l++) {
    uint16_t m = ecc[numECWords - 1] ^ codewords[l];
    for (int32_t k = numECWords - 1; k > 0; k--) {
      if (m != 0 && FACTORS[table][k] != 0) {
        ecc[k] = (uint16_t)(ecc[k - 1] ^
                            ALOG[(LOG[m] + LOG[FACTORS[table][k]]) % 255]);
      } else {
        ecc[k] = ecc[k - 1];
      }
    }
    if (m != 0 && FACTORS[table][0] != 0) {
      ecc[0] = (uint16_t)ALOG[(LOG[m] + LOG[FACTORS[table][0]]) % 255];
    } else {
      ecc[0] = 0;
    }
  }
  WideString strecc;
  for (int32_t j = 0; j < numECWords; j++) {
    strecc += (wchar_t)ecc[numECWords - j - 1];
  }
  FX_Free(ecc);
  return strecc;
}
