// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_PARSER_CXFA_NODEHELPER_H_
#define XFA_FXFA_PARSER_CXFA_NODEHELPER_H_

#include <vector>

#include "xfa/fxfa/parser/xfa_resolvenode_rs.h"

class CFXJSE_Engine;

enum XFA_LOGIC_TYPE {
  XFA_LOGIC_NoTransparent,
  XFA_LOGIC_Transparent,
};

class CXFA_NodeHelper {
 public:
  CXFA_NodeHelper();
  ~CXFA_NodeHelper();

  CXFA_Node* ResolveNodes_GetOneChild(CXFA_Node* parent,
                                      const wchar_t* pwsName,
                                      bool bIsClassName = false);
  CXFA_Node* ResolveNodes_GetParent(
      CXFA_Node* pNode,
      XFA_LOGIC_TYPE eLogicType = XFA_LOGIC_NoTransparent);

  int32_t NodeAcc_TraverseSiblings(CXFA_Node* parent,
                                   uint32_t dNameHash,
                                   std::vector<CXFA_Node*>* pSiblings,
                                   XFA_LOGIC_TYPE eLogicType,
                                   bool bIsClassName = false,
                                   bool bIsFindProperty = true);
  int32_t NodeAcc_TraverseAnySiblings(CXFA_Node* parent,
                                      uint32_t dNameHash,
                                      std::vector<CXFA_Node*>* pSiblings,
                                      bool bIsClassName = false);
  int32_t CountSiblings(CXFA_Node* pNode,
                        XFA_LOGIC_TYPE eLogicType,
                        std::vector<CXFA_Node*>* pSiblings,
                        bool bIsClassName = false);
  int32_t GetIndex(CXFA_Node* pNode,
                   XFA_LOGIC_TYPE eLogicType = XFA_LOGIC_NoTransparent,
                   bool bIsProperty = false,
                   bool bIsClassIndex = false);
  WideString GetNameExpression(
      CXFA_Node* refNode,
      bool bIsAllPath,
      XFA_LOGIC_TYPE eLogicType = XFA_LOGIC_NoTransparent);
  bool NodeIsTransparent(CXFA_Node* refNode);
  bool ResolveNodes_CreateNode(WideString wsName,
                               WideString wsCondition,
                               bool bLastNode,
                               CFXJSE_Engine* pScriptContext);
  bool CreateNode_ForCondition(WideString& wsCondition);
  void SetCreateNodeType(CXFA_Node* refNode);
  bool NodeIsProperty(CXFA_Node* refNode);

  XFA_Element m_eLastCreateType;
  CXFA_Node* m_pCreateParent;
  int32_t m_iCreateCount;
  XFA_ResolveNode_RSType m_iCreateFlag;
  int32_t m_iCurAllStart;
  CXFA_Node* m_pAllStartParent;
};

#endif  // XFA_FXFA_PARSER_CXFA_NODEHELPER_H_
