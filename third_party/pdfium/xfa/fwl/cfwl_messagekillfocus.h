// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FWL_CFWL_MESSAGEKILLFOCUS_H_
#define XFA_FWL_CFWL_MESSAGEKILLFOCUS_H_

#include <memory>

#include "xfa/fwl/cfwl_message.h"

class CFWL_MessageKillFocus : public CFWL_Message {
 public:
  explicit CFWL_MessageKillFocus(CFWL_Widget* pSrcTarget);
  CFWL_MessageKillFocus(CFWL_Widget* pSrcTarget, CFWL_Widget* pDstTarget);
  ~CFWL_MessageKillFocus() override;

  // CFWL_Message
  std::unique_ptr<CFWL_Message> Clone() override;

  CFWL_Widget* m_pSetFocus;
};

#endif  // XFA_FWL_CFWL_MESSAGEKILLFOCUS_H_
