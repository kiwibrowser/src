// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_PDF417_BC_PDF417COMPACTION_H_
#define FXBARCODE_PDF417_BC_PDF417COMPACTION_H_

class CBC_Compaction;
enum Compaction { AUTO, TEXT, BYTES, NUMERIC };
class CBC_Compaction {
 public:
  CBC_Compaction();
  virtual ~CBC_Compaction();
};

#endif  // FXBARCODE_PDF417_BC_PDF417COMPACTION_H_
