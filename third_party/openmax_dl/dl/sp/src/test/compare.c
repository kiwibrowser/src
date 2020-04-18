/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "dl/sp/src/test/compare.h"

#include <math.h>
#include <stdio.h>

extern int verbose;

// Convert to dB
static double Db(double x) {
    return (x <= 0) ? -1000.0 : 10 * log(x) / log(10);
}

static double CalculateSNR(double signal_power, double noise_power) {
    return (noise_power == 0) ? 1000 : Db(signal_power / noise_power);
}

/*
 * Each of the following functions computes the SNR of the actual
 * signal, returning the SNR of the real and imaginary parts
 * separately, and also the overall (complex SNR).
 *
 * For tests that compare real signals (CompareReal32, CompareReal16,
 * CompareFloat), the real SNR is also copied to the overall (complex)
 * SNR result.
 */

void CompareComplex32(struct SnrResult* snr, OMX_SC32* actual,
                      OMX_SC32* expected, int size) {
  double real_signal_power = 0;
  double imag_signal_power = 0;
  double complex_signal_power = 0;
  double real_noise_power = 0;
  double imag_noise_power = 0;
  double complex_noise_power = 0;
  int k;

  for (k = 0; k < size; ++k) {
    double x2;
    double y2;

    if (verbose > 255) {
      printf("%4d: (%10d, %10d) (%10d, %10d)\n", k,
             actual[k].Re, actual[k].Im,
             expected[k].Re, expected[k].Im);
    }

    x2 = pow((double) expected[k].Re, 2);
    y2 = pow((double) expected[k].Im, 2);
    real_signal_power += x2;
    imag_signal_power += y2;
    complex_signal_power += x2 + y2;

    x2 = pow((double) actual[k].Re - expected[k].Re, 2);
    y2 = pow((double) actual[k].Im - expected[k].Im, 2);

    real_noise_power += x2;
    imag_noise_power += y2;
    complex_noise_power += x2 + y2;
  }

  snr->real_snr_ = CalculateSNR(real_signal_power, real_noise_power);
  snr->imag_snr_ = CalculateSNR(imag_signal_power, imag_noise_power);
  snr->complex_snr_ = CalculateSNR(complex_signal_power, complex_noise_power);
}

void CompareComplex16(struct SnrResult* snr, OMX_SC16* actual,
                      OMX_SC16* expected, int size) {
    double realSignalPower = 0;
    double imagSignalPower = 0;
    double complexSignalPower = 0;
    double realNoisePower = 0;
    double imagNoisePower = 0;
    double complexNoisePower = 0;
    int k;
    for (k = 0; k < size; ++k) {
        double x2;
        double y2;

        if (verbose > 255) {
            printf("%4d: (%10d, %10d) (%10d, %10d)\n", k,
                   actual[k].Re, actual[k].Im,
                   expected[k].Re, expected[k].Im);
        }

        x2 = pow((double) expected[k].Re, 2);
        y2 = pow((double) expected[k].Im, 2);
        realSignalPower += x2;
        imagSignalPower += y2;
        complexSignalPower += x2 + y2;

        x2 = pow((double) actual[k].Re - expected[k].Re, 2);
        y2 = pow((double) actual[k].Im - expected[k].Im, 2);

        realNoisePower += x2;
        imagNoisePower += y2;
        complexNoisePower += x2 + y2;
    }

    snr->real_snr_ = CalculateSNR(realSignalPower, realNoisePower);
    snr->imag_snr_ = CalculateSNR(imagSignalPower, imagNoisePower);
    snr->complex_snr_ = CalculateSNR(complexSignalPower, complexNoisePower);
}

void CompareReal32(struct SnrResult* snr, OMX_S32* actual,
                   OMX_S32* expected, int size) {
  double real_signal_power = 0;
  double real_noise_power = 0;

  int k;
  for (k = 0; k < size; ++k) {
    double x2;

    x2 = pow((double) expected[k], 2);

    real_signal_power += x2;

    x2 = pow((double) actual[k] - expected[k], 2);

    real_noise_power += x2;
  }

  snr->real_snr_ = CalculateSNR(real_signal_power, real_noise_power);
  snr->imag_snr_ = -10000;
  snr->complex_snr_ = snr->real_snr_;
}

void CompareReal16(struct SnrResult* snr, OMX_S16* actual,
                   OMX_S16* expected, int size) {
  double real_signal_power = 0;
  double real_noise_power = 0;

  int k;
  for (k = 0; k < size; ++k) {
    double x2;

    x2 = pow((double) expected[k], 2);

    real_signal_power += x2;

    x2 = pow((double) actual[k] - expected[k], 2);

    real_noise_power += x2;
  }

  snr->real_snr_ = CalculateSNR(real_signal_power, real_noise_power);
  snr->imag_snr_ = -10000;
  snr->complex_snr_ = snr->real_snr_;
}

void CompareComplexFloat(struct SnrResult* snr, OMX_FC32* actual,
                         OMX_FC32* expected, int size) {
  double real_signal_power = 0;
  double imag_signal_power = 0;
  double complex_signal_power = 0;
  double real_noise_power = 0;
  double imag_noise_power = 0;
  double complex_noise_power = 0;
  int k;

  for (k = 0; k < size; ++k) {
    double x2;
    double y2;

    if (verbose > 255) {
      printf("%4d: (%10g, %10g) (%10g, %10g)\n", k,
             actual[k].Re, actual[k].Im,
             expected[k].Re, expected[k].Im);
    }

    x2 = pow((double) expected[k].Re, 2);
    y2 = pow((double) expected[k].Im, 2);
    real_signal_power += x2;
    imag_signal_power += y2;
    complex_signal_power += x2 + y2;

    x2 = pow((double) actual[k].Re - expected[k].Re, 2);
    y2 = pow((double) actual[k].Im - expected[k].Im, 2);

    real_noise_power += x2;
    imag_noise_power += y2;
    complex_noise_power += x2 + y2;
  }

  snr->real_snr_ = CalculateSNR(real_signal_power, real_noise_power);
  snr->imag_snr_ = CalculateSNR(imag_signal_power, imag_noise_power);
  snr->complex_snr_ = CalculateSNR(complex_signal_power, complex_noise_power);
}

void CompareFloat(struct SnrResult* snr, OMX_F32* actual,
                  OMX_F32* expected, int size) {
  double signal_power = 0;
  double noise_power = 0;

  int k;
  for (k = 0; k < size; ++k) {
    double x2;

    x2 = pow((double) expected[k], 2);

    signal_power += x2;

    x2 = pow((double) actual[k] - expected[k], 2);

    noise_power += x2;
  }

  snr->real_snr_ = CalculateSNR(signal_power, noise_power);
  snr->imag_snr_ = -10000;
  snr->complex_snr_ = snr->real_snr_;
}
