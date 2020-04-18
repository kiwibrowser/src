// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/page/cpdf_contentmark.h"

#include <utility>

#include "core/fpdfapi/parser/cpdf_dictionary.h"

CPDF_ContentMark::CPDF_ContentMark() {}

CPDF_ContentMark::CPDF_ContentMark(const CPDF_ContentMark& that)
    : m_Ref(that.m_Ref) {}

CPDF_ContentMark::~CPDF_ContentMark() {}

size_t CPDF_ContentMark::CountItems() const {
  return m_Ref.GetObject()->CountItems();
}

const CPDF_ContentMarkItem& CPDF_ContentMark::GetItem(size_t i) const {
  ASSERT(i < CountItems());
  return m_Ref.GetObject()->GetItem(i);
}

int CPDF_ContentMark::GetMarkedContentID() const {
  const MarkData* pData = m_Ref.GetObject();
  return pData ? pData->GetMarkedContentID() : -1;
}

void CPDF_ContentMark::AddMark(const ByteString& name,
                               const CPDF_Dictionary* pDict,
                               bool bDirect) {
  m_Ref.GetPrivateCopy()->AddMark(name, pDict, bDirect);
}

void CPDF_ContentMark::DeleteLastMark() {
  m_Ref.GetPrivateCopy()->DeleteLastMark();
  if (CountItems() == 0)
    m_Ref.SetNull();
}

CPDF_ContentMark::MarkData::MarkData() {}

CPDF_ContentMark::MarkData::MarkData(const MarkData& src)
    : m_Marks(src.m_Marks) {}

CPDF_ContentMark::MarkData::~MarkData() {}

size_t CPDF_ContentMark::MarkData::CountItems() const {
  return m_Marks.size();
}

const CPDF_ContentMarkItem& CPDF_ContentMark::MarkData::GetItem(
    size_t index) const {
  return m_Marks[index];
}

int CPDF_ContentMark::MarkData::GetMarkedContentID() const {
  for (const auto& mark : m_Marks) {
    const CPDF_Dictionary* pDict = mark.GetParam();
    if (pDict && pDict->KeyExist("MCID"))
      return pDict->GetIntegerFor("MCID");
  }
  return -1;
}

void CPDF_ContentMark::MarkData::AddMark(const ByteString& name,
                                         const CPDF_Dictionary* pDict,
                                         bool bDirect) {
  CPDF_ContentMarkItem item;
  item.SetName(name);
  if (pDict) {
    if (bDirect)
      item.SetDirectDict(ToDictionary(pDict->Clone()));
    else
      item.SetPropertiesDict(pDict);
  }
  m_Marks.push_back(std::move(item));
}

void CPDF_ContentMark::MarkData::DeleteLastMark() {
  if (!m_Marks.empty())
    m_Marks.pop_back();
}
