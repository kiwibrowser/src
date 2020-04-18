// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
// Original code is licensed as follows:
/*
 * Copyright 2012 ZXing authors
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

#include "fxbarcode/pdf417/BC_PDF417Writer.h"

#include <algorithm>

#include "fxbarcode/BC_TwoDimWriter.h"
#include "fxbarcode/common/BC_CommonBitArray.h"
#include "fxbarcode/common/BC_CommonBitMatrix.h"
#include "fxbarcode/pdf417/BC_PDF417.h"
#include "fxbarcode/pdf417/BC_PDF417BarcodeMatrix.h"
#include "fxbarcode/pdf417/BC_PDF417Compaction.h"

CBC_PDF417Writer::CBC_PDF417Writer() {
  m_bFixedSize = false;
}
CBC_PDF417Writer::~CBC_PDF417Writer() {
  m_bTruncated = true;
}
bool CBC_PDF417Writer::SetErrorCorrectionLevel(int32_t level) {
  if (level < 0 || level > 8) {
    return false;
  }
  m_iCorrectLevel = level;
  return true;
}
void CBC_PDF417Writer::SetTruncated(bool truncated) {
  m_bTruncated = truncated;
}

uint8_t* CBC_PDF417Writer::Encode(const WideString& contents,
                                  int32_t& outWidth,
                                  int32_t& outHeight) {
  CBC_PDF417 encoder;
  int32_t col = (m_Width / m_ModuleWidth - 69) / 17;
  int32_t row = m_Height / (m_ModuleWidth * 20);
  if (row >= 3 && row <= 90 && col >= 1 && col <= 30)
    encoder.setDimensions(col, col, row, row);
  else if (col >= 1 && col <= 30)
    encoder.setDimensions(col, col, 90, 3);
  else if (row >= 3 && row <= 90)
    encoder.setDimensions(30, 1, row, row);
  if (!encoder.generateBarcodeLogic(contents, m_iCorrectLevel))
    return nullptr;

  int32_t lineThickness = 2;
  int32_t aspectRatio = 4;
  CBC_BarcodeMatrix* barcodeMatrix = encoder.getBarcodeMatrix();
  std::vector<uint8_t> originalScale = barcodeMatrix->getScaledMatrix(
      lineThickness, aspectRatio * lineThickness);
  int32_t width = outWidth;
  int32_t height = outHeight;
  outWidth = barcodeMatrix->getWidth();
  outHeight = barcodeMatrix->getHeight();
  bool rotated = false;
  if ((height > width) ^ (outWidth < outHeight)) {
    rotateArray(originalScale, outHeight, outWidth);
    rotated = true;
    int32_t temp = outHeight;
    outHeight = outWidth;
    outWidth = temp;
  }
  int32_t scaleX = width / outWidth;
  int32_t scaleY = height / outHeight;
  int32_t scale = std::min(scaleX, scaleY);
  if (scale > 1) {
    originalScale = barcodeMatrix->getScaledMatrix(
        scale * lineThickness, scale * aspectRatio * lineThickness);
    if (rotated) {
      rotateArray(originalScale, outHeight, outWidth);
      int32_t temp = outHeight;
      outHeight = outWidth;
      outWidth = temp;
    }
  }
  uint8_t* result = FX_Alloc2D(uint8_t, outHeight, outWidth);
  memcpy(result, originalScale.data(), outHeight * outWidth);
  return result;
}

void CBC_PDF417Writer::rotateArray(std::vector<uint8_t>& bitarray,
                                   int32_t height,
                                   int32_t width) {
  std::vector<uint8_t> temp = bitarray;
  for (int32_t ii = 0; ii < height; ii++) {
    int32_t inverseii = height - ii - 1;
    for (int32_t jj = 0; jj < width; jj++) {
      bitarray[jj * height + inverseii] = temp[ii * width + jj];
    }
  }
}
