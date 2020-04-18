// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/parser/cpdf_stream.h"

#include <utility>

#include "constants/stream_dict_common.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_stream_acc.h"
#include "core/fpdfapi/parser/fpdf_parser_decode.h"
#include "core/fxcrt/fx_stream.h"
#include "third_party/base/numerics/safe_conversions.h"
#include "third_party/base/ptr_util.h"
#include "third_party/base/stl_util.h"

CPDF_Stream::CPDF_Stream() {}

CPDF_Stream::CPDF_Stream(std::unique_ptr<uint8_t, FxFreeDeleter> pData,
                         uint32_t size,
                         std::unique_ptr<CPDF_Dictionary> pDict)
    : m_pDict(std::move(pDict)) {
  SetData(std::move(pData), size);
}

CPDF_Stream::~CPDF_Stream() {
  m_ObjNum = kInvalidObjNum;
  if (m_pDict && m_pDict->GetObjNum() == kInvalidObjNum)
    m_pDict.release();  // lowercase release, release ownership.
}

CPDF_Object::Type CPDF_Stream::GetType() const {
  return STREAM;
}

CPDF_Dictionary* CPDF_Stream::GetDict() {
  return m_pDict.get();
}

const CPDF_Dictionary* CPDF_Stream::GetDict() const {
  return m_pDict.get();
}

bool CPDF_Stream::IsStream() const {
  return true;
}

CPDF_Stream* CPDF_Stream::AsStream() {
  return this;
}

const CPDF_Stream* CPDF_Stream::AsStream() const {
  return this;
}

void CPDF_Stream::InitStream(const uint8_t* pData,
                             uint32_t size,
                             std::unique_ptr<CPDF_Dictionary> pDict) {
  m_pDict = std::move(pDict);
  SetData(pData, size);
}

void CPDF_Stream::InitStreamFromFile(
    const RetainPtr<IFX_SeekableReadStream>& pFile,
    std::unique_ptr<CPDF_Dictionary> pDict) {
  m_pDict = std::move(pDict);
  m_bMemoryBased = false;
  m_pDataBuf.reset();
  m_pFile = pFile;
  m_dwSize = pdfium::base::checked_cast<uint32_t>(pFile->GetSize());
  if (m_pDict)
    m_pDict->SetNewFor<CPDF_Number>("Length", static_cast<int>(m_dwSize));
}

std::unique_ptr<CPDF_Object> CPDF_Stream::Clone() const {
  return CloneObjectNonCyclic(false);
}

std::unique_ptr<CPDF_Object> CPDF_Stream::CloneNonCyclic(
    bool bDirect,
    std::set<const CPDF_Object*>* pVisited) const {
  pVisited->insert(this);
  auto pAcc = pdfium::MakeRetain<CPDF_StreamAcc>(this);
  pAcc->LoadAllDataRaw();

  uint32_t streamSize = pAcc->GetSize();
  const CPDF_Dictionary* pDict = GetDict();
  std::unique_ptr<CPDF_Dictionary> pNewDict;
  if (pDict && !pdfium::ContainsKey(*pVisited, pDict)) {
    pNewDict =
        ToDictionary(static_cast<const CPDF_Object*>(pDict)->CloneNonCyclic(
            bDirect, pVisited));
  }
  return pdfium::MakeUnique<CPDF_Stream>(pAcc->DetachData(), streamSize,
                                         std::move(pNewDict));
}

void CPDF_Stream::SetDataAndRemoveFilter(const uint8_t* pData, uint32_t size) {
  SetData(pData, size);
  m_pDict->RemoveFor("Filter");
  m_pDict->RemoveFor(pdfium::stream::kDecodeParms);
}

void CPDF_Stream::SetDataAndRemoveFilter(std::ostringstream* stream) {
  if (stream->tellp() <= 0) {
    SetDataAndRemoveFilter(nullptr, 0);
    return;
  }

  SetDataAndRemoveFilter(
      reinterpret_cast<const uint8_t*>(stream->str().c_str()), stream->tellp());
}

void CPDF_Stream::SetData(const uint8_t* pData, uint32_t size) {
  std::unique_ptr<uint8_t, FxFreeDeleter> data_copy;
  if (pData) {
    data_copy.reset(FX_Alloc(uint8_t, size));
    memcpy(data_copy.get(), pData, size);
  }
  SetData(std::move(data_copy), size);
}

void CPDF_Stream::SetData(std::unique_ptr<uint8_t, FxFreeDeleter> pData,
                          uint32_t size) {
  m_bMemoryBased = true;
  m_pFile = nullptr;
  m_pDataBuf = std::move(pData);
  m_dwSize = size;
  if (!m_pDict)
    m_pDict = pdfium::MakeUnique<CPDF_Dictionary>();
  m_pDict->SetNewFor<CPDF_Number>("Length", static_cast<int>(size));
}

void CPDF_Stream::SetData(std::ostringstream* stream) {
  if (stream->tellp() <= 0) {
    SetData(nullptr, 0);
    return;
  }

  SetData(reinterpret_cast<const uint8_t*>(stream->str().c_str()),
          stream->tellp());
}

bool CPDF_Stream::ReadRawData(FX_FILESIZE offset,
                              uint8_t* buf,
                              uint32_t size) const {
  if (!m_bMemoryBased && m_pFile)
    return m_pFile->ReadBlock(buf, offset, size);

  if (m_pDataBuf)
    memcpy(buf, m_pDataBuf.get() + offset, size);

  return true;
}

bool CPDF_Stream::HasFilter() const {
  return m_pDict && m_pDict->KeyExist("Filter");
}

WideString CPDF_Stream::GetUnicodeText() const {
  auto pAcc = pdfium::MakeRetain<CPDF_StreamAcc>(this);
  pAcc->LoadAllDataFiltered();
  return PDF_DecodeText(pAcc->GetData(), pAcc->GetSize());
}

bool CPDF_Stream::WriteTo(IFX_ArchiveStream* archive) const {
  if (!GetDict()->WriteTo(archive) || !archive->WriteString("stream\r\n"))
    return false;

  auto pAcc = pdfium::MakeRetain<CPDF_StreamAcc>(this);
  pAcc->LoadAllDataRaw();
  return archive->WriteBlock(pAcc->GetData(), pAcc->GetSize()) &&
         archive->WriteString("\r\nendstream");
}
