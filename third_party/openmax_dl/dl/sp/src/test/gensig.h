/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_ARM_FFT_TEST_GENSIG_H_
#define WEBRTC_ARM_FFT_TEST_GENSIG_H_

struct ComplexFloat {
    float Re;
    float Im;
};

/*
 * Returns the max allowed signal type, depending on whether the test
 * signal is real or not
 */
int MaxSignalType(int real_only);

/*
 * Generate a test signal and corresponding FFT.
 */
void GenerateTestSignalAndFFT(struct ComplexFloat* x,
                              struct ComplexFloat* fft,
                              int size,
                              int signal_type,
                              float signal_value,
                              int real_only);

#endif
