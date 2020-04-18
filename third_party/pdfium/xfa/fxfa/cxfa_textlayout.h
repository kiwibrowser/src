// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_CXFA_TEXTLAYOUT_H_
#define XFA_FXFA_CXFA_TEXTLAYOUT_H_

#include <memory>
#include <vector>

#include "core/fxcrt/css/cfx_css.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_string.h"
#include "xfa/fgas/layout/cfx_rtfbreak.h"
#include "xfa/fxfa/cxfa_textparser.h"

class CFDE_RenderDevice;
class CFX_CSSComputedStyle;
class CFX_RenderDevice;
class CFX_RTFBreak;
class CFX_XMLNode;
class CXFA_LinkUserData;
class CXFA_LoaderContext;
class CXFA_Node;
class CXFA_PieceLine;
class CXFA_TextPiece;
class CXFA_TextProvider;
class CXFA_TextTabstopsContext;

class CXFA_TextLayout {
 public:
  explicit CXFA_TextLayout(CXFA_FFDoc* doc, CXFA_TextProvider* pTextProvider);
  ~CXFA_TextLayout();

  float GetLayoutHeight();
  float StartLayout(float fWidth);
  float DoLayout(int32_t iBlockIndex,
                 float fCalcHeight,
                 float fContentAreaHeight,
                 float fTextHeight);
  float Layout(const CFX_SizeF& size);

  CFX_SizeF CalcSize(const CFX_SizeF& minSize, const CFX_SizeF& maxSize);
  void ItemBlocks(const CFX_RectF& rtText, int32_t iBlockIndex);
  bool DrawString(CFX_RenderDevice* pFxDevice,
                  const CFX_Matrix& tmDoc2Device,
                  const CFX_RectF& rtClip,
                  int32_t iBlock);
  bool IsLoaded() const { return !m_pieceLines.empty(); }
  void Unload();
  const std::vector<std::unique_ptr<CXFA_PieceLine>>* GetPieceLines() const {
    return &m_pieceLines;
  }

  bool m_bHasBlock;
  std::vector<int32_t> m_Blocks;

 private:
  void GetTextDataNode();
  CFX_XMLNode* GetXMLContainerNode();
  std::unique_ptr<CFX_RTFBreak> CreateBreak(bool bDefault);
  void InitBreak(float fLineWidth);
  void InitBreak(CFX_CSSComputedStyle* pStyle,
                 CFX_CSSDisplay eDisplay,
                 float fLineWidth,
                 CFX_XMLNode* pXMLNode,
                 CFX_CSSComputedStyle* pParentStyle);
  bool Loader(float textWidth, float* pLinePos, bool bSavePieces);
  void LoadText(CXFA_Node* pNode,
                float textWidth,
                float* pLinePos,
                bool bSavePieces);
  bool LoadRichText(CFX_XMLNode* pXMLNode,
                    float textWidth,
                    float* pLinePos,
                    const RetainPtr<CFX_CSSComputedStyle>& pParentStyle,
                    bool bSavePieces,
                    RetainPtr<CXFA_LinkUserData> pLinkData,
                    bool bEndBreak = true,
                    bool bIsOl = false,
                    int32_t iLiCount = 0);
  bool AppendChar(const WideString& wsText,
                  float* pLinePos,
                  float fSpaceAbove,
                  bool bSavePieces);
  void AppendTextLine(CFX_BreakType dwStatus,
                      float* pLinePos,
                      bool bSavePieces,
                      bool bEndBreak = false);
  void EndBreak(CFX_BreakType dwStatus, float* pLinePos, bool bDefault);
  bool IsEnd(bool bSavePieces);
  void ProcessText(WideString& wsText);
  void UpdateAlign(float fHeight, float fBottom);
  void RenderString(CFX_RenderDevice* pDevice,
                    CXFA_PieceLine* pPieceLine,
                    int32_t iPiece,
                    FXTEXT_CHARPOS* pCharPos,
                    const CFX_Matrix& tmDoc2Device);
  void RenderPath(CFX_RenderDevice* pDevice,
                  CXFA_PieceLine* pPieceLine,
                  int32_t iPiece,
                  FXTEXT_CHARPOS* pCharPos,
                  const CFX_Matrix& tmDoc2Device);
  int32_t GetDisplayPos(const CXFA_TextPiece* pPiece,
                        FXTEXT_CHARPOS* pCharPos,
                        bool bCharCode = false);
  bool ToRun(const CXFA_TextPiece* pPiece, FX_RTFTEXTOBJ* tr);
  void DoTabstops(CFX_CSSComputedStyle* pStyle, CXFA_PieceLine* pPieceLine);
  bool Layout(int32_t iBlock);
  int32_t CountBlocks() const;

  CXFA_FFDoc* m_pDoc;
  CXFA_TextProvider* m_pTextProvider;
  CXFA_Node* m_pTextDataNode;
  bool m_bRichText;
  std::unique_ptr<CFX_RTFBreak> m_pBreak;
  std::unique_ptr<CXFA_LoaderContext> m_pLoader;
  int32_t m_iLines;
  float m_fMaxWidth;
  CXFA_TextParser m_textParser;
  std::vector<std::unique_ptr<CXFA_PieceLine>> m_pieceLines;
  std::unique_ptr<CXFA_TextTabstopsContext> m_pTabstopContext;
  bool m_bBlockContinue;
};

#endif  // XFA_FXFA_CXFA_TEXTLAYOUT_H_
