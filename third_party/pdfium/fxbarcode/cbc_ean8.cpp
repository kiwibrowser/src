// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
/*
 * Copyright 2011 ZXing authors
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

#include "fxbarcode/cbc_ean8.h"

#include <memory>

#include "fxbarcode/oned/BC_OnedEAN8Writer.h"
#include "third_party/base/ptr_util.h"

CBC_EAN8::CBC_EAN8() : CBC_OneCode(pdfium::MakeUnique<CBC_OnedEAN8Writer>()) {}

CBC_EAN8::~CBC_EAN8() {}

WideString CBC_EAN8::Preprocess(const WideStringView& contents) {
  auto* pWriter = GetOnedEAN8Writer();
  WideString encodeContents = pWriter->FilterContents(contents);
  int32_t length = encodeContents.GetLength();
  if (length <= 7) {
    for (int32_t i = 0; i < 7 - length; i++)
      encodeContents = wchar_t('0') + encodeContents;

    ByteString byteString = encodeContents.UTF8Encode();
    int32_t checksum = pWriter->CalcChecksum(byteString);
    encodeContents += wchar_t(checksum - 0 + '0');
  }
  if (length > 8)
    encodeContents = encodeContents.Left(8);

  return encodeContents;
}

bool CBC_EAN8::Encode(const WideStringView& contents) {
  if (contents.IsEmpty())
    return false;

  BCFORMAT format = BCFORMAT_EAN_8;
  int32_t outWidth = 0;
  int32_t outHeight = 0;
  WideString encodeContents = Preprocess(contents);
  ByteString byteString = encodeContents.UTF8Encode();
  m_renderContents = encodeContents;
  auto* pWriter = GetOnedEAN8Writer();
  std::unique_ptr<uint8_t, FxFreeDeleter> data(
      pWriter->Encode(byteString, format, outWidth, outHeight));
  if (!data)
    return false;
  return pWriter->RenderResult(encodeContents.AsStringView(), data.get(),
                               outWidth);
}

bool CBC_EAN8::RenderDevice(CFX_RenderDevice* device,
                            const CFX_Matrix* matrix) {
  return GetOnedEAN8Writer()->RenderDeviceResult(
      device, matrix, m_renderContents.AsStringView());
}

BC_TYPE CBC_EAN8::GetType() {
  return BC_EAN8;
}

CBC_OnedEAN8Writer* CBC_EAN8::GetOnedEAN8Writer() {
  return static_cast<CBC_OnedEAN8Writer*>(m_pBCWriter.get());
}
