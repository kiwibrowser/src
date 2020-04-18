// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FWL_CFWL_MESSAGE_H_
#define XFA_FWL_CFWL_MESSAGE_H_

#include <memory>

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"

class CFWL_Widget;

class CFWL_Message {
 public:
  enum class Type { Key, KillFocus, Mouse, MouseWheel, SetFocus };

  explicit CFWL_Message(Type type);
  CFWL_Message(Type type, CFWL_Widget* pSrcTarget);
  CFWL_Message(Type type, CFWL_Widget* pSrcTarget, CFWL_Widget* pDstTarget);
  virtual ~CFWL_Message();

  virtual std::unique_ptr<CFWL_Message> Clone();
  Type GetType() const { return m_type; }

  CFWL_Widget* m_pSrcTarget;
  CFWL_Widget* m_pDstTarget;

 private:
  Type m_type;
};

#endif  // XFA_FWL_CFWL_MESSAGE_H_
