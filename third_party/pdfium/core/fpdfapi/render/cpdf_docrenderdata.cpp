// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/render/cpdf_docrenderdata.h"

#include <memory>

#include "core/fpdfapi/font/cpdf_type3font.h"
#include "core/fpdfapi/page/cpdf_function.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/render/cpdf_dibsource.h"
#include "core/fpdfapi/render/cpdf_transferfunc.h"
#include "core/fpdfapi/render/cpdf_type3cache.h"

namespace {

const int kMaxOutputs = 16;

}  // namespace

CPDF_DocRenderData::CPDF_DocRenderData(CPDF_Document* pPDFDoc)
    : m_pPDFDoc(pPDFDoc) {}

CPDF_DocRenderData::~CPDF_DocRenderData() {
  Clear(true);
}

void CPDF_DocRenderData::Clear(bool bRelease) {
  for (auto it = m_Type3FaceMap.begin(); it != m_Type3FaceMap.end();) {
    auto curr_it = it++;
    if (bRelease || curr_it->second->HasOneRef()) {
      m_Type3FaceMap.erase(curr_it);
    }
  }

  for (auto it = m_TransferFuncMap.begin(); it != m_TransferFuncMap.end();) {
    auto curr_it = it++;
    if (bRelease || curr_it->second->HasOneRef())
      m_TransferFuncMap.erase(curr_it);
  }
}

RetainPtr<CPDF_Type3Cache> CPDF_DocRenderData::GetCachedType3(
    CPDF_Type3Font* pFont) {
  auto it = m_Type3FaceMap.find(pFont);
  if (it != m_Type3FaceMap.end())
    return it->second;

  auto pCache = pdfium::MakeRetain<CPDF_Type3Cache>(pFont);
  m_Type3FaceMap[pFont] = pCache;
  return pCache;
}

void CPDF_DocRenderData::MaybePurgeCachedType3(CPDF_Type3Font* pFont) {
  auto it = m_Type3FaceMap.find(pFont);
  if (it != m_Type3FaceMap.end() && it->second->HasOneRef())
    m_Type3FaceMap.erase(it);
}

RetainPtr<CPDF_TransferFunc> CPDF_DocRenderData::GetTransferFunc(
    const CPDF_Object* pObj) {
  if (!pObj)
    return nullptr;

  auto it = m_TransferFuncMap.find(pObj);
  if (it != m_TransferFuncMap.end())
    return it->second;

  std::unique_ptr<CPDF_Function> pFuncs[3];
  bool bUniTransfer = true;
  bool bIdentity = true;
  if (const CPDF_Array* pArray = pObj->AsArray()) {
    bUniTransfer = false;
    if (pArray->GetCount() < 3)
      return nullptr;

    for (uint32_t i = 0; i < 3; ++i) {
      pFuncs[2 - i] = CPDF_Function::Load(pArray->GetDirectObjectAt(i));
      if (!pFuncs[2 - i])
        return nullptr;
    }
  } else {
    pFuncs[0] = CPDF_Function::Load(pObj);
    if (!pFuncs[0])
      return nullptr;
  }
  auto pTransfer = pdfium::MakeRetain<CPDF_TransferFunc>(m_pPDFDoc.Get());
  m_TransferFuncMap[pObj] = pTransfer;

  float input;
  int noutput;
  float output[kMaxOutputs];
  memset(output, 0, sizeof(output));
  for (int v = 0; v < 256; ++v) {
    input = (float)v / 255.0f;
    if (bUniTransfer) {
      if (pFuncs[0] && pFuncs[0]->CountOutputs() <= kMaxOutputs)
        pFuncs[0]->Call(&input, 1, output, &noutput);
      int o = FXSYS_round(output[0] * 255);
      if (o != v)
        bIdentity = false;
      for (int i = 0; i < 3; ++i)
        pTransfer->GetSamples()[i * 256 + v] = o;
      continue;
    }
    for (int i = 0; i < 3; ++i) {
      if (!pFuncs[i] || pFuncs[i]->CountOutputs() > kMaxOutputs) {
        pTransfer->GetSamples()[i * 256 + v] = v;
        continue;
      }
      pFuncs[i]->Call(&input, 1, output, &noutput);
      int o = FXSYS_round(output[0] * 255);
      if (o != v)
        bIdentity = false;
      pTransfer->GetSamples()[i * 256 + v] = o;
    }
  }

  pTransfer->SetIdentity(bIdentity);
  return pTransfer;
}

void CPDF_DocRenderData::MaybePurgeTransferFunc(const CPDF_Object* pObj) {
  auto it = m_TransferFuncMap.find(pObj);
  if (it != m_TransferFuncMap.end() && it->second->HasOneRef())
    m_TransferFuncMap.erase(it);
}
