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

#include <inttypes.h>
#include <stdlib.h>
#include <wchar.h>

#include "UniquePtr.h"

namespace {
constexpr size_t MBS_FAILURE = static_cast<size_t>(-1);
}

template <typename T>
static T wcstox(T (*func)(const char*, char**, int), const wchar_t* wcs,
                wchar_t** wcs_end, int base) {
  mbstate_t mbstate;
  memset(&mbstate, 0, sizeof(mbstate));

  if (wcs_end != nullptr) {
    *wcs_end = const_cast<wchar_t*>(wcs);
  }

  const size_t max_mb_len = wcslen(wcs) * 4 + 1;
  UniquePtr<char[]> mbs(new char[max_mb_len]);
  const wchar_t* s = wcs;
  if (wcsrtombs(mbs.get(), &s, max_mb_len, &mbstate) == MBS_FAILURE) {
    return static_cast<T>(0);
  }

  char* mbs_end;
  T value = func(mbs.get(), &mbs_end, base);
  if (wcs_end == nullptr) {
    // If the user passed nullptr for the end pointer, we don't need to compute
    // it and can return early.
    return value;
  }

  // strto* can set ERANGE or EINVAL. Preserve the value of errno in case any of
  // the things we're about to do to comput the end pointer don't clobber it.
  int preserved_errno = errno;

  // wcs_end needs to point to the character after the one converted. We don't
  // know how many wide characters were converted, but we can figure that out by
  // converting the multibyte string between mbs and mbs_end back to a wide
  // character string.
  size_t converted_len = mbs_end - mbs.get();
  UniquePtr<char[]> converted_mbs(new char[converted_len + 1]);
  strncpy(converted_mbs.get(), mbs.get(), converted_len);
  converted_mbs[converted_len] = '\0';

  const char* mbsp = converted_mbs.get();
  size_t converted_wlen = mbsrtowcs(nullptr, &mbsp, 0, &mbstate);
  if (converted_wlen == MBS_FAILURE) {
    // This should be impossible.
    abort();
  }

  *wcs_end = const_cast<wchar_t*>(wcs) + converted_wlen;
  errno = preserved_errno;
  return value;
}

static float strtof_wrapper(const char* s, char** p, int) {
  return strtof(s, p);
}

float wcstof(const wchar_t* s, wchar_t** p) {
  return wcstox(strtof_wrapper, s, p, 0);
}

static double strtod_wrapper(const char* s, char** p, int) {
  return strtod(s, p);
}

double wcstod(const wchar_t *restrict s, wchar_t **restrict p) {
  return wcstox(strtod_wrapper, s, p, 0);
}

long wcstol(const wchar_t *restrict s, wchar_t **restrict p, int base) {
  return wcstox(strtol, s, p, base);
}

unsigned long wcstoul(const wchar_t *restrict s, wchar_t **restrict p, int base) {
  return wcstox(strtoul, s, p, base);
}

long long wcstoll(const wchar_t *restrict s, wchar_t **restrict p, int base) {
  return wcstox(strtoll, s, p, base);
}

unsigned long long wcstoull(const wchar_t *restrict s, wchar_t **restrict p, int base) {
  return wcstox(strtoull, s, p, base);
}

intmax_t wcstoimax(const wchar_t *restrict s, wchar_t **restrict p, int base) {
  return wcstox(strtoimax, s, p, base);
}

uintmax_t wcstoumax(const wchar_t *restrict s, wchar_t **restrict p, int base) {
  return wcstox(strtoumax, s, p, base);
}
