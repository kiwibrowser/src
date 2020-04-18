/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_CHECKED_NUMERIC_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_CHECKED_NUMERIC_H_

/* See base/numerics/safe_math.h for usage.
 */
#include "base/numerics/safe_math.h"

namespace WTF {
using base::CheckedNumeric;
using base::IsValidForType;
using base::ValueOrDieForType;
using base::ValueOrDefaultForType;
using base::MakeCheckedNum;
using base::CheckMax;
using base::CheckMin;
using base::CheckAdd;
using base::CheckSub;
using base::CheckMul;
using base::CheckDiv;
using base::CheckMod;
using base::CheckLsh;
using base::CheckRsh;
using base::CheckAnd;
using base::CheckOr;
using base::CheckXor;
}  // namespace WTF

using WTF::CheckedNumeric;

#endif
