// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/parser/cpdf_stream_acc.h"

#include "core/fpdfapi/parser/fpdf_parser_decode.h"

CPDF_StreamAcc::CPDF_StreamAcc(const CPDF_Stream* pStream)
    : m_pStream(pStream) {}

CPDF_StreamAcc::~CPDF_StreamAcc() {
  if (m_bNewBuf)
    FX_Free(m_pData);
  FX_Free(m_pSrcData);
}

void CPDF_StreamAcc::LoadAllData(bool bRawAccess,
                                 uint32_t estimated_size,
                                 bool bImageAcc) {
  if (!m_pStream)
    return;

  bool bProcessRawData = bRawAccess || !m_pStream->HasFilter();
  if (bProcessRawData && m_pStream->IsMemoryBased()) {
    m_dwSize = m_pStream->GetRawSize();
    m_pData = m_pStream->GetRawData();
    return;
  }
  uint32_t dwSrcSize = m_pStream->GetRawSize();
  if (dwSrcSize == 0)
    return;

  uint8_t* pSrcData;
  if (m_pStream->IsMemoryBased()) {
    pSrcData = m_pStream->GetRawData();
  } else {
    pSrcData = m_pSrcData = FX_Alloc(uint8_t, dwSrcSize);
    if (!m_pStream->ReadRawData(0, pSrcData, dwSrcSize))
      return;
  }
  if (bProcessRawData) {
    m_pData = pSrcData;
    m_dwSize = dwSrcSize;
  } else if (!PDF_DataDecode(pSrcData, dwSrcSize, m_pStream->GetDict(),
                             estimated_size, bImageAcc, &m_pData, &m_dwSize,
                             &m_ImageDecoder, &m_pImageParam)) {
    m_pData = pSrcData;
    m_dwSize = dwSrcSize;
  }
  if (pSrcData != m_pStream->GetRawData() && pSrcData != m_pData)
    FX_Free(pSrcData);
  m_pSrcData = nullptr;
  m_bNewBuf = m_pData != m_pStream->GetRawData();
}

void CPDF_StreamAcc::LoadAllDataFiltered() {
  LoadAllData(false, 0, false);
}

void CPDF_StreamAcc::LoadAllDataRaw() {
  LoadAllData(true, 0, false);
}

const CPDF_Dictionary* CPDF_StreamAcc::GetDict() const {
  return m_pStream ? m_pStream->GetDict() : nullptr;
}

uint8_t* CPDF_StreamAcc::GetData() const {
  if (m_bNewBuf)
    return m_pData;
  return m_pStream ? m_pStream->GetRawData() : nullptr;
}

uint32_t CPDF_StreamAcc::GetSize() const {
  if (m_bNewBuf)
    return m_dwSize;
  return m_pStream ? m_pStream->GetRawSize() : 0;
}

std::unique_ptr<uint8_t, FxFreeDeleter> CPDF_StreamAcc::DetachData() {
  if (m_bNewBuf) {
    std::unique_ptr<uint8_t, FxFreeDeleter> p(m_pData);
    m_pData = nullptr;
    m_dwSize = 0;
    return p;
  }
  std::unique_ptr<uint8_t, FxFreeDeleter> p(FX_Alloc(uint8_t, m_dwSize));
  memcpy(p.get(), m_pData, m_dwSize);
  return p;
}
