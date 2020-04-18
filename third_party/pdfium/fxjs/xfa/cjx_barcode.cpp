// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_barcode.h"

#include "xfa/fxfa/parser/cxfa_barcode.h"

CJX_Barcode::CJX_Barcode(CXFA_Barcode* node) : CJX_Node(node) {}

CJX_Barcode::~CJX_Barcode() = default;

void CJX_Barcode::charEncoding(CFXJSE_Value* pValue,
                               bool bSetting,
                               XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::checksum(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::dataColumnCount(CFXJSE_Value* pValue,
                                  bool bSetting,
                                  XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::dataLength(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::dataPrep(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::dataRowCount(CFXJSE_Value* pValue,
                               bool bSetting,
                               XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::endChar(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::errorCorrectionLevel(CFXJSE_Value* pValue,
                                       bool bSetting,
                                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::moduleHeight(CFXJSE_Value* pValue,
                               bool bSetting,
                               XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::moduleWidth(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::printCheckDigit(CFXJSE_Value* pValue,
                                  bool bSetting,
                                  XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::rowColumnRatio(CFXJSE_Value* pValue,
                                 bool bSetting,
                                 XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::startChar(CFXJSE_Value* pValue,
                            bool bSetting,
                            XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::textLocation(CFXJSE_Value* pValue,
                               bool bSetting,
                               XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::truncate(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::type(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::upsMode(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::use(CFXJSE_Value* pValue,
                      bool bSetting,
                      XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::usehref(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Barcode::wideNarrowRatio(CFXJSE_Value* pValue,
                                  bool bSetting,
                                  XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
