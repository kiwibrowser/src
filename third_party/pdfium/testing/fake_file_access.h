// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_FAKE_FILE_ACCESS_H_
#define TESTING_FAKE_FILE_ACCESS_H_

#include <memory>

#include "core/fxcrt/unowned_ptr.h"
#include "public/fpdf_dataavail.h"
#include "public/fpdfview.h"
#include "testing/range_set.h"

class FakeFileAccess {
 public:
  explicit FakeFileAccess(FPDF_FILEACCESS* file_access);
  ~FakeFileAccess();

  FPDF_FILEACCESS* GetFileAccess() const { return file_access_wrapper_.get(); }
  FX_FILEAVAIL* GetFileAvail() const { return file_avail_.get(); }
  FX_DOWNLOADHINTS* GetDownloadHints() const { return download_hints_.get(); }

  FPDF_BOOL IsDataAvail(size_t offset, size_t size) const;
  void AddSegment(size_t offset, size_t size);

  unsigned long GetFileSize();

  int GetBlock(unsigned long position, unsigned char* pBuf, unsigned long size);
  void SetRequestedDataAvailable();
  void SetWholeFileAvailable();

 private:
  fxcrt::UnownedPtr<FPDF_FILEACCESS> file_access_;
  std::unique_ptr<FPDF_FILEACCESS> file_access_wrapper_;
  std::unique_ptr<FX_FILEAVAIL> file_avail_;
  std::unique_ptr<FX_DOWNLOADHINTS> download_hints_;
  RangeSet available_data_;
  RangeSet requested_data_;
};

#endif  // TESTING_FAKE_FILE_ACCESS_H_
