// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/page/cpdf_psfunc.h"

#include "core/fpdfapi/parser/cpdf_stream_acc.h"
#include "third_party/base/ptr_util.h"

CPDF_PSFunc::CPDF_PSFunc() : CPDF_Function(Type::kType4PostScript) {}

CPDF_PSFunc::~CPDF_PSFunc() {}

bool CPDF_PSFunc::v_Init(const CPDF_Object* pObj,
                         std::set<const CPDF_Object*>* pVisited) {
  auto pAcc = pdfium::MakeRetain<CPDF_StreamAcc>(pObj->AsStream());
  pAcc->LoadAllDataFiltered();
  return m_PS.Parse(pAcc->GetSpan());
}

bool CPDF_PSFunc::v_Call(const float* inputs, float* results) const {
  CPDF_PSEngine& PS = const_cast<CPDF_PSEngine&>(m_PS);
  PS.Reset();
  for (uint32_t i = 0; i < m_nInputs; i++)
    PS.Push(inputs[i]);
  PS.Execute();
  if (PS.GetStackSize() < m_nOutputs)
    return false;
  for (uint32_t i = 0; i < m_nOutputs; i++)
    results[m_nOutputs - i - 1] = PS.Pop();
  return true;
}
