/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_ARM_FLOAT_RFFT_THRESHOLDS_H_
#define WEBRTC_ARM_FLOAT_RFFT_THRESHOLDS_H_

#if defined(__arm__)
#ifdef BIG_FFT_TABLE
#define FLOAT_RFFT_FORWARD_THRESHOLD_NEON (136.07)
#define FLOAT_RFFT_INVERSE_THRESHOLD_NEON (140.76)
#define FLOAT_RFFT_FORWARD_THRESHOLD_ARMV7 (134.95)
#define FLOAT_RFFT_INVERSE_THRESHOLD_ARMV7 (140.69)
#else
#define FLOAT_RFFT_FORWARD_THRESHOLD_NEON (136.07)
#define FLOAT_RFFT_INVERSE_THRESHOLD_NEON (142.41)
#define FLOAT_RFFT_FORWARD_THRESHOLD_ARMV7 (134.95)
#define FLOAT_RFFT_INVERSE_THRESHOLD_ARMV7 (142.25)
#endif
#elif defined(__aarch64__)
#ifdef BIG_FFT_TABLE
#define FLOAT_RFFT_FORWARD_THRESHOLD_NEON (136.55)
#define FLOAT_RFFT_INVERSE_THRESHOLD_NEON (141.55)
#else
#define FLOAT_RFFT_FORWARD_THRESHOLD_NEON (136.55)
#define FLOAT_RFFT_INVERSE_THRESHOLD_NEON (142.74)
#endif
#elif defined(__mips__)
#ifdef BIG_FFT_TABLE
#define FLOAT_RFFT_FORWARD_THRESHOLD_MIPS (134.65)
#define FLOAT_RFFT_INVERSE_THRESHOLD_MIPS (140.52)
#else
#define FLOAT_RFFT_FORWARD_THRESHOLD_MIPS (137.33)
#define FLOAT_RFFT_INVERSE_THRESHOLD_MIPS (144.88)
#endif
#else
#ifdef BIG_FFT_TABLE
#define FLOAT_RFFT_FORWARD_THRESHOLD_X86 (135.97)
#define FLOAT_RFFT_INVERSE_THRESHOLD_X86 (140.76)
#else
#define FLOAT_RFFT_FORWARD_THRESHOLD_X86 (135.97)
#define FLOAT_RFFT_INVERSE_THRESHOLD_X86 (142.69)
#endif
#endif

#endif /* WEBRTC_ARM_FLOAT_RFFT_THRESHOLDS_H_ */
