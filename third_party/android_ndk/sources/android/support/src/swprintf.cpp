/*
 * Copyright (C) 2017 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <wchar.h>

#include "UniquePtr.h"

namespace {
const size_t MBS_FAILURE = static_cast<size_t>(-1);
}

int swprintf(wchar_t* wcs, size_t maxlen, const wchar_t* format, ...) {
  va_list ap;
  va_start(ap, format);
  int result = vswprintf(wcs, maxlen, format, ap);
  va_end(ap);
  return result;
}

int vswprintf(wchar_t* wcs, size_t maxlen, const wchar_t* fmt, va_list ap) {
  mbstate_t mbstate;
  memset(&mbstate, 0, sizeof(mbstate));

  // At most, each wide character (UTF-32) can be expanded to four narrow
  // characters (UTF-8).
  const size_t max_mb_len = maxlen * 4;
  const size_t mb_fmt_len = wcslen(fmt) * 4 + 1;
  UniquePtr<char[]> mbfmt(new char[mb_fmt_len]);
  if (wcsrtombs(mbfmt.get(), &fmt, mb_fmt_len, &mbstate) == MBS_FAILURE) {
    return -1;
  }

  UniquePtr<char[]> mbs(new char[max_mb_len]);
  int nprinted = vsnprintf(mbs.get(), max_mb_len, mbfmt.get(), ap);
  if (nprinted == -1) {
    return -1;
  }

  const char* mbsp = mbs.get();
  if (mbsrtowcs(wcs, &mbsp, maxlen, &mbstate) == MBS_FAILURE) {
    return -1;
  }

  // Can't use return value from vsnprintf because that number is in narrow
  // characters, not wide characters.
  int result = wcslen(wcs);

  // swprintf differs from snprintf in that it returns -1 if the output was
  // truncated.
  //
  // Truncation can occur in two places:
  // 1) vsnprintf truncated, in which case the return value is greater than the
  //    length we passed.
  // 2) Since the char buffer we pass to vsnprintf might be oversized, that
  //    might not truncate while mbsrtowcs will. In this case, mbsp will point
  //    to the next unconverted character instead of nullptr.
  if (nprinted >= max_mb_len || mbsp != nullptr) {
    return -1;
  }

  return result;
}
