// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_WIDESTRING_H_
#define CORE_FXCRT_WIDESTRING_H_

#include <functional>
#include <iterator>
#include <utility>

#include "core/fxcrt/fx_memory.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/string_data_template.h"
#include "core/fxcrt/string_view_template.h"
#include "third_party/base/optional.h"
#include "third_party/base/span.h"

namespace fxcrt {

class ByteString;
class StringPool_WideString_Test;
class WideString_ConcatInPlace_Test;

// A mutable string with shared buffers using copy-on-write semantics that
// avoids the cost of std::string's iterator stability guarantees.
class WideString {
 public:
  using CharType = wchar_t;
  using const_iterator = const CharType*;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  static WideString Format(const wchar_t* lpszFormat, ...) WARN_UNUSED_RESULT;
  static WideString FormatV(const wchar_t* lpszFormat,
                            va_list argList) WARN_UNUSED_RESULT;

  WideString();
  WideString(const WideString& other);
  WideString(WideString&& other) noexcept;

  // Deliberately implicit to avoid calling on every string literal.
  // NOLINTNEXTLINE(runtime/explicit)
  WideString(wchar_t ch);
  // NOLINTNEXTLINE(runtime/explicit)
  WideString(const wchar_t* ptr);

  // No implicit conversions from byte strings.
  // NOLINTNEXTLINE(runtime/explicit)
  WideString(char) = delete;

  WideString(const wchar_t* ptr, size_t len);

  explicit WideString(const WideStringView& str);
  WideString(const WideStringView& str1, const WideStringView& str2);
  WideString(const std::initializer_list<WideStringView>& list);

  ~WideString();

  static WideString FromLocal(const ByteStringView& str) WARN_UNUSED_RESULT;
  static WideString FromCodePage(const ByteStringView& str,
                                 uint16_t codepage) WARN_UNUSED_RESULT;

  static WideString FromUTF8(const ByteStringView& str) WARN_UNUSED_RESULT;
  static WideString FromUTF16LE(const unsigned short* str,
                                size_t len) WARN_UNUSED_RESULT;

  static size_t WStringLength(const unsigned short* str) WARN_UNUSED_RESULT;

  // Explicit conversion to C-style wide string.
  // Note: Any subsequent modification of |this| will invalidate the result.
  const wchar_t* c_str() const { return m_pData ? m_pData->m_String : L""; }

  // Explicit conversion to WideStringView.
  // Note: Any subsequent modification of |this| will invalidate the result.
  WideStringView AsStringView() const {
    return WideStringView(c_str(), GetLength());
  }

  // Explicit conversion to span.
  // Note: Any subsequent modification of |this| will invalidate the result.
  pdfium::span<const wchar_t> AsSpan() const {
    return pdfium::make_span(m_pData ? m_pData->m_String : nullptr,
                             GetLength());
  }

  // Note: Any subsequent modification of |this| will invalidate iterators.
  const_iterator begin() const { return m_pData ? m_pData->m_String : nullptr; }
  const_iterator end() const {
    return m_pData ? m_pData->m_String + m_pData->m_nDataLength : nullptr;
  }

  // Note: Any subsequent modification of |this| will invalidate iterators.
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  void clear() { m_pData.Reset(); }

  size_t GetLength() const { return m_pData ? m_pData->m_nDataLength : 0; }
  size_t GetStringLength() const {
    return m_pData ? wcslen(m_pData->m_String) : 0;
  }
  bool IsEmpty() const { return !GetLength(); }
  bool IsValidIndex(size_t index) const { return index < GetLength(); }
  bool IsValidLength(size_t length) const { return length <= GetLength(); }

  const WideString& operator=(const wchar_t* str);
  const WideString& operator=(const WideString& stringSrc);
  const WideString& operator=(const WideStringView& stringSrc);

  const WideString& operator+=(const wchar_t* str);
  const WideString& operator+=(wchar_t ch);
  const WideString& operator+=(const WideString& str);
  const WideString& operator+=(const WideStringView& str);

  bool operator==(const wchar_t* ptr) const;
  bool operator==(const WideStringView& str) const;
  bool operator==(const WideString& other) const;

  bool operator!=(const wchar_t* ptr) const { return !(*this == ptr); }
  bool operator!=(const WideStringView& str) const { return !(*this == str); }
  bool operator!=(const WideString& other) const { return !(*this == other); }

  bool operator<(const wchar_t* ptr) const;
  bool operator<(const WideStringView& str) const;
  bool operator<(const WideString& other) const;

  CharType operator[](const size_t index) const {
    ASSERT(IsValidIndex(index));
    return m_pData ? m_pData->m_String[index] : 0;
  }

  CharType First() const { return GetLength() ? (*this)[0] : 0; }
  CharType Last() const { return GetLength() ? (*this)[GetLength() - 1] : 0; }

  void SetAt(size_t index, wchar_t c);

  int Compare(const wchar_t* str) const;
  int Compare(const WideString& str) const;
  int CompareNoCase(const wchar_t* str) const;

  WideString Mid(size_t first, size_t count) const;
  WideString Left(size_t count) const;
  WideString Right(size_t count) const;

  size_t Insert(size_t index, wchar_t ch);
  size_t InsertAtFront(wchar_t ch) { return Insert(0, ch); }
  size_t InsertAtBack(wchar_t ch) { return Insert(GetLength(), ch); }
  size_t Delete(size_t index, size_t count = 1);

  void MakeLower();
  void MakeUpper();

  void Trim();
  void Trim(wchar_t target);
  void Trim(const WideStringView& targets);

  void TrimLeft();
  void TrimLeft(wchar_t target);
  void TrimLeft(const WideStringView& targets);

  void TrimRight();
  void TrimRight(wchar_t target);
  void TrimRight(const WideStringView& targets);

  void Reserve(size_t len);

  // Note: any modification of the string (including ReleaseBuffer()) may
  // invalidate the span, which must not outlive its buffer.
  pdfium::span<wchar_t> GetBuffer(size_t len);
  void ReleaseBuffer(size_t len);

  int GetInteger() const;
  float GetFloat() const;

  Optional<size_t> Find(const WideStringView& pSub, size_t start = 0) const;
  Optional<size_t> Find(wchar_t ch, size_t start = 0) const;

  bool Contains(const WideStringView& lpszSub, size_t start = 0) const {
    return Find(lpszSub, start).has_value();
  }

  bool Contains(char ch, size_t start = 0) const {
    return Find(ch, start).has_value();
  }

  size_t Replace(const WideStringView& pOld, const WideStringView& pNew);
  size_t Remove(wchar_t ch);

  ByteString UTF8Encode() const;

  // This method will add \0\0 to the end of the string to represent the
  // wide string terminator. These values are in the string, not just the data,
  // so GetLength() will include them.
  ByteString UTF16LE_Encode() const;

 protected:
  using StringData = StringDataTemplate<wchar_t>;

  void ReallocBeforeWrite(size_t nLen);
  void AllocBeforeWrite(size_t nLen);
  void AllocCopy(WideString& dest, size_t nCopyLen, size_t nCopyIndex) const;
  void AssignCopy(const wchar_t* pSrcData, size_t nSrcLen);
  void Concat(const wchar_t* lpszSrcData, size_t nSrcLen);

  RetainPtr<StringData> m_pData;

  friend WideString_ConcatInPlace_Test;
  friend StringPool_WideString_Test;
};

inline WideString operator+(const WideStringView& str1,
                            const WideStringView& str2) {
  return WideString(str1, str2);
}
inline WideString operator+(const WideStringView& str1, const wchar_t* str2) {
  return WideString(str1, str2);
}
inline WideString operator+(const wchar_t* str1, const WideStringView& str2) {
  return WideString(str1, str2);
}
inline WideString operator+(const WideStringView& str1, wchar_t ch) {
  return WideString(str1, WideStringView(ch));
}
inline WideString operator+(wchar_t ch, const WideStringView& str2) {
  return WideString(ch, str2);
}
inline WideString operator+(const WideString& str1, const WideString& str2) {
  return WideString(str1.AsStringView(), str2.AsStringView());
}
inline WideString operator+(const WideString& str1, wchar_t ch) {
  return WideString(str1.AsStringView(), WideStringView(ch));
}
inline WideString operator+(wchar_t ch, const WideString& str2) {
  return WideString(ch, str2.AsStringView());
}
inline WideString operator+(const WideString& str1, const wchar_t* str2) {
  return WideString(str1.AsStringView(), str2);
}
inline WideString operator+(const wchar_t* str1, const WideString& str2) {
  return WideString(str1, str2.AsStringView());
}
inline WideString operator+(const WideString& str1,
                            const WideStringView& str2) {
  return WideString(str1.AsStringView(), str2);
}
inline WideString operator+(const WideStringView& str1,
                            const WideString& str2) {
  return WideString(str1, str2.AsStringView());
}
inline bool operator==(const wchar_t* lhs, const WideString& rhs) {
  return rhs == lhs;
}
inline bool operator==(const WideStringView& lhs, const WideString& rhs) {
  return rhs == lhs;
}
inline bool operator!=(const wchar_t* lhs, const WideString& rhs) {
  return rhs != lhs;
}
inline bool operator!=(const WideStringView& lhs, const WideString& rhs) {
  return rhs != lhs;
}
inline bool operator<(const wchar_t* lhs, const WideString& rhs) {
  return rhs.Compare(lhs) > 0;
}

std::wostream& operator<<(std::wostream& os, const WideString& str);
std::ostream& operator<<(std::ostream& os, const WideString& str);
std::wostream& operator<<(std::wostream& os, const WideStringView& str);
std::ostream& operator<<(std::ostream& os, const WideStringView& str);

}  // namespace fxcrt

using WideString = fxcrt::WideString;

uint32_t FX_HashCode_GetW(const WideStringView& str, bool bIgnoreCase);

namespace std {

template <>
struct hash<WideString> {
  std::size_t operator()(const WideString& str) const {
    return FX_HashCode_GetW(str.AsStringView(), false);
  }
};

}  // namespace std

extern template struct std::hash<WideString>;

#endif  // CORE_FXCRT_WIDESTRING_H_
