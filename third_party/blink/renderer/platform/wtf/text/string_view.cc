// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/wtf/text/string_view.h"

#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/string_impl.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace WTF {

StringView::StringView(const UChar* chars)
    : StringView(chars, chars ? LengthOfNullTerminatedString(chars) : 0) {}

#if DCHECK_IS_ON()
StringView::~StringView() {
  DCHECK(impl_);
  DCHECK(!impl_->HasOneRef() || impl_->IsStatic())
      << "StringView does not own the StringImpl, it "
         "must not have the last ref.";
}
#endif

String StringView::ToString() const {
  if (IsNull())
    return String();
  if (IsEmpty())
    return g_empty_string;
  if (StringImpl* impl = SharedImpl())
    return impl;
  if (Is8Bit())
    return String(Characters8(), length_);
  return StringImpl::Create8BitIfPossible(Characters16(), length_);
}

AtomicString StringView::ToAtomicString() const {
  if (IsNull())
    return g_null_atom;
  if (IsEmpty())
    return g_empty_atom;
  if (StringImpl* impl = SharedImpl())
    return AtomicString(impl);
  if (Is8Bit())
    return AtomicString(Characters8(), length_);
  return AtomicString(Characters16(), length_);
}

bool EqualStringView(const StringView& a, const StringView& b) {
  if (a.IsNull() || b.IsNull())
    return a.IsNull() == b.IsNull();
  if (a.length() != b.length())
    return false;
  if (a.Is8Bit()) {
    if (b.Is8Bit())
      return Equal(a.Characters8(), b.Characters8(), a.length());
    return Equal(a.Characters8(), b.Characters16(), a.length());
  }
  if (b.Is8Bit())
    return Equal(a.Characters16(), b.Characters8(), a.length());
  return Equal(a.Characters16(), b.Characters16(), a.length());
}

bool DeprecatedEqualIgnoringCaseAndNullity(const StringView& a,
                                           const StringView& b) {
  if (a.length() != b.length())
    return false;
  if (a.Is8Bit()) {
    if (b.Is8Bit()) {
      return DeprecatedEqualIgnoringCase(a.Characters8(), b.Characters8(),
                                         a.length());
    }
    return DeprecatedEqualIgnoringCase(a.Characters8(), b.Characters16(),
                                       a.length());
  }
  if (b.Is8Bit()) {
    return DeprecatedEqualIgnoringCase(a.Characters16(), b.Characters8(),
                                       a.length());
  }
  return DeprecatedEqualIgnoringCase(a.Characters16(), b.Characters16(),
                                     a.length());
}

bool DeprecatedEqualIgnoringCase(const StringView& a, const StringView& b) {
  if (a.IsNull() || b.IsNull())
    return a.IsNull() == b.IsNull();
  return DeprecatedEqualIgnoringCaseAndNullity(a, b);
}

bool EqualIgnoringASCIICase(const StringView& a, const StringView& b) {
  if (a.IsNull() || b.IsNull())
    return a.IsNull() == b.IsNull();
  if (a.length() != b.length())
    return false;
  if (a.Is8Bit()) {
    if (b.Is8Bit())
      return EqualIgnoringASCIICase(a.Characters8(), b.Characters8(),
                                    a.length());
    return EqualIgnoringASCIICase(a.Characters8(), b.Characters16(),
                                  a.length());
  }
  if (b.Is8Bit())
    return EqualIgnoringASCIICase(a.Characters16(), b.Characters8(),
                                  a.length());
  return EqualIgnoringASCIICase(a.Characters16(), b.Characters16(), a.length());
}

}  // namespace WTF
