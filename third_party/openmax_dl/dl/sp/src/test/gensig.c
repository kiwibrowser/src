/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "dl/sp/src/test/gensig.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define MAX_REAL_SIGNAL_TYPE 3
#define MAX_SIGNAL_TYPE 4

int MaxSignalType(int real_only) {
  return real_only ? MAX_REAL_SIGNAL_TYPE : MAX_SIGNAL_TYPE;
}

/*
 * Generate a test signal and compute the theoretical FFT.
 *
 * The test signal is specified by |signal_type|, and the test signal
 * is saved in |x| with the corresponding FFT in |fft|.  The size of
 * the test signal is |size|.  |signalValue| is desired the amplitude
 * of the test signal.
 *
 * If |real_only| is true, then the test signal is assumed to be real
 * instead of complex, which is the default.  This is only applicable
 * for a |signal_type| of 0 or 3; the other signals are already real-valued.
 */
void GenerateTestSignalAndFFT(struct ComplexFloat* x,
                              struct ComplexFloat* fft,
                              int size,
                              int signal_type,
                              float signal_value,
                              int real_only) {
  int k;
    
  switch (signal_type) {
    case 0:
      /*
       * Signal is a constant signal_value + i*signal_value (or just
       * signal_value if real_only is true.)
       */
      for (k = 0; k < size; ++k) {
        x[k].Re = signal_value;
        x[k].Im = real_only ? 0 : signal_value;
      }

      fft[0].Re = signal_value * size;
      fft[0].Im = real_only ? 0 : signal_value * size;

      for (k = 1; k < size; ++k) {
        fft[k].Re = fft[k].Im = 0;
      }
      break;
    case 1:
      /*
       * A real-valued ramp
       */
      {
        double factor = signal_value / (float) size;
        double omega = 2 * M_PI / size;

        for (k = 0; k < size; ++k) {
          x[k].Re = ((k + 1)*factor);
          x[k].Im = 0;
        }

        fft[0].Re = factor * size * (size + 1) / 2;
        fft[0].Im = 0;
        for (k = 1; k < size; ++k) {
          double phase;
          phase = omega * k;
          fft[k].Re = factor * -size / 2;
          fft[k].Im = factor * size / 2 * (sin(phase) / (1 - cos(phase)));
        }

        /*
         * Remove any roundoff for k = N/2 since sin(2*pi/N*N/2) = 0.
         */
        fft[size / 2].Im = 0;
      }
      break;
    case 2:
      /*
       * Pure real-valued sine wave, one cycle.
       */
      {
        double omega = 2 * M_PI / size;

        for (k = 0; k < size; ++k) {
          x[k].Re = signal_value * sin(omega * k);
          x[k].Im = 0;
        }

        /*
         * Remove any roundoff for k = N/2 since sin(2*pi/N*N/2) = 0.
         */
        x[size / 2 ].Re = 0;

        for (k = 0; k < size; ++k) {
          fft[k].Re = 0;
          fft[k].Im = 0;
        }

        /*
         * When size == 2, x[k] is identically zero, so the FFT is also zero.
         */
        if (size != 2) {
          fft[1].Im = -signal_value * (size / 2);
          fft[size - 1].Im = signal_value * (size / 2);
        }
      }
      break;
    case 3:
      /*
       * The source signal is x[k] = 0 except x[1] = x[size-1] =
       * -i*signal_value.  The transform is one period of a pure real
       * (negative) sine wave.  Only defined when real_only is false.
       */
      if (!real_only) {
        double omega = 2 * M_PI / size;
        for (k = 0; k < size; ++k) {
          x[k].Re = 0;
          x[k].Im = 0;
        }
        x[1].Im = -signal_value;
        x[size-1].Im = signal_value;

        if (size == 2) {
          fft[0].Re = 0;
          fft[0].Im = signal_value;
          fft[1].Re = 0;
          fft[1].Im = -signal_value;
        } else {
          for (k = 0; k < size; ++k) {
            fft[k].Re = -2 * signal_value * sin(omega * k);
            fft[k].Im = 0;
          }

          /*
           * Remove any roundoff for k = N/2 since sin(2*pi/N*N/2) = 0.
           */
          fft[size / 2].Re = 0;
        }
        break;
      }
      /* Fall through if real_only */
    case MAX_SIGNAL_TYPE:
    default:
      fprintf(stderr, "invalid signal type: %d\n", signal_type);
      exit(1);
  }
}
