// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcrt/cfx_memorystream.h"

#include <algorithm>

#include "core/fxcrt/fx_safe_types.h"

namespace {

const int32_t kBlockSize = 64 * 1024;

}  // namespace

CFX_MemoryStream::CFX_MemoryStream(bool bConsecutive)
    : m_nTotalSize(0),
      m_nCurSize(0),
      m_nCurPos(0),
      m_nGrowSize(kBlockSize),
      m_dwFlags(Type::kTakeOver | (bConsecutive ? Type::kConsecutive : 0)) {}

CFX_MemoryStream::CFX_MemoryStream(uint8_t* pBuffer,
                                   size_t nSize,
                                   bool bTakeOver)
    : m_nTotalSize(nSize),
      m_nCurSize(nSize),
      m_nCurPos(0),
      m_nGrowSize(kBlockSize),
      m_dwFlags(Type::kConsecutive | (bTakeOver ? Type::kTakeOver : 0)) {
  m_Blocks.push_back(pBuffer);
}

CFX_MemoryStream::~CFX_MemoryStream() {
  if (m_dwFlags & Type::kTakeOver) {
    for (uint8_t* pBlock : m_Blocks)
      FX_Free(pBlock);
  }
}

FX_FILESIZE CFX_MemoryStream::GetSize() {
  return static_cast<FX_FILESIZE>(m_nCurSize);
}

bool CFX_MemoryStream::IsEOF() {
  return m_nCurPos >= static_cast<size_t>(GetSize());
}

FX_FILESIZE CFX_MemoryStream::GetPosition() {
  return static_cast<FX_FILESIZE>(m_nCurPos);
}

bool CFX_MemoryStream::Flush() {
  return true;
}

bool CFX_MemoryStream::ReadBlock(void* buffer,
                                 FX_FILESIZE offset,
                                 size_t size) {
  if (!buffer || !size || offset < 0)
    return false;

  FX_SAFE_SIZE_T newPos = size;
  newPos += offset;
  if (!newPos.IsValid() || newPos.ValueOrDefault(0) == 0 ||
      newPos.ValueOrDie() > m_nCurSize) {
    return false;
  }

  m_nCurPos = newPos.ValueOrDie();
  if (m_dwFlags & Type::kConsecutive) {
    memcpy(buffer, m_Blocks[0] + static_cast<size_t>(offset), size);
    return true;
  }

  size_t nStartBlock = static_cast<size_t>(offset) / m_nGrowSize;
  offset -= static_cast<FX_FILESIZE>(nStartBlock * m_nGrowSize);
  while (size) {
    size_t nRead = std::min(size, m_nGrowSize - static_cast<size_t>(offset));
    memcpy(buffer, m_Blocks[nStartBlock] + offset, nRead);
    buffer = static_cast<uint8_t*>(buffer) + nRead;
    size -= nRead;
    ++nStartBlock;
    offset = 0;
  }
  return true;
}

size_t CFX_MemoryStream::ReadBlock(void* buffer, size_t size) {
  if (m_nCurPos >= m_nCurSize)
    return 0;

  size_t nRead = std::min(size, m_nCurSize - m_nCurPos);
  if (!ReadBlock(buffer, static_cast<int32_t>(m_nCurPos), nRead))
    return 0;

  return nRead;
}

bool CFX_MemoryStream::WriteBlock(const void* buffer,
                                  FX_FILESIZE offset,
                                  size_t size) {
  if (!buffer || !size)
    return false;

  if (m_dwFlags & Type::kConsecutive) {
    FX_SAFE_SIZE_T newPos = size;
    newPos += offset;
    if (!newPos.IsValid())
      return false;

    m_nCurPos = newPos.ValueOrDie();
    if (m_nCurPos > m_nTotalSize) {
      m_nTotalSize = (m_nCurPos + m_nGrowSize - 1) / m_nGrowSize * m_nGrowSize;
      if (m_Blocks.empty())
        m_Blocks.push_back(FX_Alloc(uint8_t, m_nTotalSize));
      else
        m_Blocks[0] = FX_Realloc(uint8_t, m_Blocks[0], m_nTotalSize);
    }

    memcpy(m_Blocks[0] + offset, buffer, size);
    m_nCurSize = std::max(m_nCurSize, m_nCurPos);

    return true;
  }

  FX_SAFE_SIZE_T newPos = size;
  newPos += offset;
  if (!newPos.IsValid())
    return false;
  if (!ExpandBlocks(newPos.ValueOrDie()))
    return false;

  m_nCurPos = newPos.ValueOrDie();
  size_t nStartBlock = static_cast<size_t>(offset) / m_nGrowSize;
  offset -= static_cast<FX_FILESIZE>(nStartBlock * m_nGrowSize);
  while (size) {
    size_t nWrite = std::min(size, m_nGrowSize - static_cast<size_t>(offset));
    memcpy(m_Blocks[nStartBlock] + offset, buffer, nWrite);
    buffer = static_cast<const uint8_t*>(buffer) + nWrite;
    size -= nWrite;
    ++nStartBlock;
    offset = 0;
  }
  return true;
}

bool CFX_MemoryStream::Seek(size_t pos) {
  if (pos > m_nCurSize)
    return false;

  m_nCurPos = pos;
  return true;
}

void CFX_MemoryStream::EstimateSize(size_t nInitSize, size_t nGrowSize) {
  if (m_dwFlags & Type::kConsecutive) {
    if (m_Blocks.empty()) {
      m_Blocks.push_back(
          FX_Alloc(uint8_t, std::max(nInitSize, static_cast<size_t>(4096))));
    }
    m_nGrowSize = std::max(nGrowSize, static_cast<size_t>(4096));
  } else if (m_Blocks.empty()) {
    m_nGrowSize = std::max(nGrowSize, static_cast<size_t>(4096));
  }
}

void CFX_MemoryStream::AttachBuffer(uint8_t* pBuffer, size_t nSize) {
  if (!(m_dwFlags & Type::kConsecutive))
    return;

  m_Blocks.clear();
  m_Blocks.push_back(pBuffer);
  m_nTotalSize = nSize;
  m_nCurSize = nSize;
  m_nCurPos = 0;
  m_dwFlags = Type::kConsecutive;
}

void CFX_MemoryStream::DetachBuffer() {
  if (!(m_dwFlags & Type::kConsecutive))
    return;

  m_Blocks.clear();
  m_nTotalSize = 0;
  m_nCurSize = 0;
  m_nCurPos = 0;
  m_dwFlags = Type::kTakeOver;
}

bool CFX_MemoryStream::ExpandBlocks(size_t size) {
  m_nCurSize = std::max(m_nCurSize, size);
  if (size <= m_nTotalSize)
    return true;

  size = (size - m_nTotalSize + m_nGrowSize - 1) / m_nGrowSize;
  size_t iCount = m_Blocks.size();
  m_Blocks.resize(iCount + size);
  while (size--) {
    m_Blocks[iCount++] = FX_Alloc(uint8_t, m_nGrowSize);
    m_nTotalSize += m_nGrowSize;
  }
  return true;
}
