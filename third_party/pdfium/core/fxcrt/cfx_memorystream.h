// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_CFX_MEMORYSTREAM_H_
#define CORE_FXCRT_CFX_MEMORYSTREAM_H_

#include <vector>

#include "core/fxcrt/fx_stream.h"
#include "core/fxcrt/retain_ptr.h"

class CFX_MemoryStream : public IFX_SeekableStream {
 public:
  enum Type { kConsecutive = 1 << 0, kTakeOver = 1 << 1 };

  template <typename T, typename... Args>
  friend RetainPtr<T> pdfium::MakeRetain(Args&&... args);

  // IFX_SeekableStream
  FX_FILESIZE GetSize() override;
  FX_FILESIZE GetPosition() override;
  bool IsEOF() override;
  bool ReadBlock(void* buffer, FX_FILESIZE offset, size_t size) override;
  size_t ReadBlock(void* buffer, size_t size) override;
  bool WriteBlock(const void* buffer, FX_FILESIZE offset, size_t size) override;
  bool Flush() override;

  // Sets the cursor position to |pos| if possible
  bool Seek(size_t pos);

  bool IsConsecutive() const { return !!(m_dwFlags & Type::kConsecutive); }

  uint8_t* GetBuffer() const {
    return !m_Blocks.empty() ? m_Blocks.front() : nullptr;
  }

  void EstimateSize(size_t nInitSize, size_t nGrowSize);
  void AttachBuffer(uint8_t* pBuffer, size_t nSize);
  void DetachBuffer();

 private:
  explicit CFX_MemoryStream(bool bConsecutive);
  CFX_MemoryStream(uint8_t* pBuffer, size_t nSize, bool bTakeOver);
  ~CFX_MemoryStream() override;

  bool ExpandBlocks(size_t size);

  std::vector<uint8_t*> m_Blocks;
  size_t m_nTotalSize;
  size_t m_nCurSize;
  size_t m_nCurPos;
  size_t m_nGrowSize;
  uint32_t m_dwFlags;
};

#endif  // CORE_FXCRT_CFX_MEMORYSTREAM_H_
