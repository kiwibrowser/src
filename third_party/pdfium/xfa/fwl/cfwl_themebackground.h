// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FWL_CFWL_THEMEBACKGROUND_H_
#define XFA_FWL_CFWL_THEMEBACKGROUND_H_

#include "xfa/fwl/cfwl_themepart.h"

class CXFA_Graphics;
class CXFA_GEPath;

class CFWL_ThemeBackground : public CFWL_ThemePart {
 public:
  CFWL_ThemeBackground();
  ~CFWL_ThemeBackground();

  CXFA_Graphics* m_pGraphics;
  CXFA_GEPath* m_pPath;
};

inline CFWL_ThemeBackground::CFWL_ThemeBackground()
    : m_pGraphics(nullptr), m_pPath(nullptr) {}

inline CFWL_ThemeBackground::~CFWL_ThemeBackground() {}

#endif  // XFA_FWL_CFWL_THEMEBACKGROUND_H_
