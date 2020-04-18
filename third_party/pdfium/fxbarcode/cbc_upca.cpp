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

#include "fxbarcode/cbc_upca.h"

#include <memory>

#include "fxbarcode/oned/BC_OnedUPCAWriter.h"
#include "third_party/base/ptr_util.h"

CBC_UPCA::CBC_UPCA() : CBC_OneCode(pdfium::MakeUnique<CBC_OnedUPCAWriter>()) {}

CBC_UPCA::~CBC_UPCA() {}

WideString CBC_UPCA::Preprocess(const WideStringView& contents) {
  CBC_OnedUPCAWriter* pWriter = GetOnedUPCAWriter();
  WideString encodeContents = pWriter->FilterContents(contents);
  int32_t length = encodeContents.GetLength();
  if (length <= 11) {
    for (int32_t i = 0; i < 11 - length; i++)
      encodeContents = wchar_t('0') + encodeContents;

    ByteString byteString = encodeContents.UTF8Encode();
    int32_t checksum = pWriter->CalcChecksum(byteString);
    byteString += checksum - 0 + '0';
    encodeContents = byteString.UTF8Decode();
  }
  if (length > 12)
    encodeContents = encodeContents.Left(12);

  return encodeContents;
}

bool CBC_UPCA::Encode(const WideStringView& contents) {
  if (contents.IsEmpty())
    return false;

  BCFORMAT format = BCFORMAT_UPC_A;
  int32_t outWidth = 0;
  int32_t outHeight = 0;
  WideString encodeContents = Preprocess(contents);
  ByteString byteString = encodeContents.UTF8Encode();
  m_renderContents = encodeContents;

  CBC_OnedUPCAWriter* pWriter = GetOnedUPCAWriter();
  pWriter->Init();
  std::unique_ptr<uint8_t, FxFreeDeleter> data(
      pWriter->Encode(byteString, format, outWidth, outHeight));
  if (!data)
    return false;
  return pWriter->RenderResult(encodeContents.AsStringView(), data.get(),
                               outWidth);
}

bool CBC_UPCA::RenderDevice(CFX_RenderDevice* device,
                            const CFX_Matrix* matrix) {
  return GetOnedUPCAWriter()->RenderDeviceResult(
      device, matrix, m_renderContents.AsStringView());
}

BC_TYPE CBC_UPCA::GetType() {
  return BC_UPCA;
}

CBC_OnedUPCAWriter* CBC_UPCA::GetOnedUPCAWriter() {
  return static_cast<CBC_OnedUPCAWriter*>(m_pBCWriter.get());
}
