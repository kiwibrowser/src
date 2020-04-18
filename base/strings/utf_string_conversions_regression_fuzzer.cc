// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/strings/old_utf_string_conversions.h"
#include "base/strings/utf_string_conversions.h"

namespace {

void UTF8ToCheck(const uint8_t* data, size_t size) {
  const auto* src = reinterpret_cast<const char*>(data);
  const size_t src_len = size;

  // UTF16
  {
    base::string16 new_out;
    bool new_res = base::UTF8ToUTF16(src, src_len, &new_out);

    base::string16 old_out;
    bool old_res = base_old::UTF8ToUTF16(src, src_len, &old_out);

    CHECK(new_res == old_res);
    CHECK(new_out == old_out);
  }

  // Wide
  {
    std::wstring new_out;
    bool new_res = base::UTF8ToWide(src, src_len, &new_out);

    std::wstring old_out;
    bool old_res = base_old::UTF8ToWide(src, src_len, &old_out);

    CHECK(new_res == old_res);
    CHECK(new_out == old_out);
  }
}

void UTF16ToCheck(const uint8_t* data, size_t size) {
  const auto* src = reinterpret_cast<const base::char16*>(data);
  const size_t src_len = size / 2;

  // UTF8
  {
    std::string new_out;
    bool new_res = base::UTF16ToUTF8(src, src_len, &new_out);

    std::string old_out;
    bool old_res = base_old::UTF16ToUTF8(src, src_len, &old_out);

    CHECK(new_res == old_res);
    CHECK(new_out == old_out);
  }

  // Wide
  {
    std::wstring new_out;
    bool new_res = base::UTF16ToWide(src, src_len, &new_out);

    std::wstring old_out;
    bool old_res = base_old::UTF16ToWide(src, src_len, &old_out);

    CHECK(new_res == old_res);
    CHECK(new_out == old_out);
  }
}

void WideToCheck(const uint8_t* data, size_t size) {
  const auto* src = reinterpret_cast<const wchar_t*>(data);
  const size_t src_len = size / 4;  // It's OK even if Wide is 16bit.

  // UTF8
  {
    std::string new_out;
    bool new_res = base::WideToUTF8(src, src_len, &new_out);

    std::string old_out;
    bool old_res = base_old::WideToUTF8(src, src_len, &old_out);

    CHECK(new_res == old_res);
    CHECK(new_out == old_out);
  }

  // UTF16
  {
    base::string16 new_out;
    bool new_res = base::WideToUTF16(src, src_len, &new_out);

    base::string16 old_out;
    bool old_res = base_old::WideToUTF16(src, src_len, &old_out);

    CHECK(new_res == old_res);
    CHECK(new_out == old_out);
  }
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  UTF8ToCheck(data, size);
  UTF16ToCheck(data, size);
  WideToCheck(data, size);
  return 0;
}
