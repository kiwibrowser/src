/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_CPU_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_CPU_H_

#if defined(arm) || defined(__arm__) || defined(ARM) || defined(_ARM_)

#if !defined(__ARMEB__) && !defined(__ARM_EABI__) && !defined(__EABI__) && \
    !defined(__VFP_FP__) && !defined(_WIN32_WCE) && !defined(ANDROID)
#error Chromium does not support middle endian architecture
#endif

// WTF_CPU_ARM_NEON is 0 or 1, and should not use defined(WTF_CPU_ARM_NEON).
#if defined(__ARM_NEON__) && !defined(WTF_CPU_ARM_NEON)
#define WTF_CPU_ARM_NEON 1
#endif

#endif /* ARM */

#if !defined(WTF_CPU_ARM_NEON)
#define WTF_CPU_ARM_NEON 0
#endif

// HAVE_MIPS_MSA_INTRINSICS is 0 or 1, and we should not use
// defined(HAVE_MIPS_MSA_INTRINSICS).
#if defined(__mips_msa) && defined(__mips_isa_rev) && (__mips_isa_rev >= 5)
#define HAVE_MIPS_MSA_INTRINSICS 1
#else
#define HAVE_MIPS_MSA_INTRINSICS 0
#endif

#endif /* WTF_CPU_h */
