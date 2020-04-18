/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_AUDIO_FFT_FRAME_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_AUDIO_FFT_FRAME_H_

#include <memory>

#include "build/build_config.h"
#include "third_party/blink/renderer/platform/audio/audio_array.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"

#if defined(OS_MACOSX)
#include <Accelerate/Accelerate.h>
#elif defined(WTF_USE_WEBAUDIO_OPENMAX_DL_FFT)
#include <dl/sp/api/omxSP.h>
#elif defined(WTF_USE_WEBAUDIO_FFMPEG)
struct RDFTContext;
#endif

namespace blink {

// Defines the interface for an "FFT frame", an object which is able to perform
// a forward and reverse FFT, internally storing the resultant frequency-domain
// data.

class PLATFORM_EXPORT FFTFrame {
  USING_FAST_MALLOC(FFTFrame);

 public:
  // The constructors, destructor, and methods up to the CROSS-PLATFORM section
  // have platform-dependent implementations.

  FFTFrame(unsigned fft_size);
  // creates a blank/empty frame for later use with createInterpolatedFrame()
  FFTFrame();
  FFTFrame(const FFTFrame& frame);
  ~FFTFrame();

  static void Initialize();
  static void Cleanup();
  void DoFFT(const float* data);
  void DoInverseFFT(float* data);

  float* RealData() { return real_data_.Data(); }
  const float* RealData() const { return real_data_.Data(); }
  float* ImagData() { return imag_data_.Data(); }
  const float* ImagData() const { return imag_data_.Data(); }

  unsigned FftSize() const { return fft_size_; }
  unsigned Log2FFTSize() const { return log2fft_size_; }

  // CROSS-PLATFORM
  // The remaining public methods have cross-platform implementations:

  // Interpolates from frame1 -> frame2 as x goes from 0.0 -> 1.0
  static std::unique_ptr<FFTFrame> CreateInterpolatedFrame(
      const FFTFrame& frame1,
      const FFTFrame& frame2,
      double x);
  // zero-padding with dataSize <= fftSize
  void DoPaddedFFT(const float* data, size_t data_size);
  double ExtractAverageGroupDelay();
  void AddConstantGroupDelay(double sample_frame_delay);
  // multiplies ourself with frame : effectively operator*=()
  void Multiply(const FFTFrame&);

 private:
  void InterpolateFrequencyComponents(const FFTFrame& frame1,
                                      const FFTFrame& frame2,
                                      double x);

  unsigned fft_size_;
  unsigned log2fft_size_;
  AudioFloatArray real_data_;
  AudioFloatArray imag_data_;

#if defined(OS_MACOSX)
  DSPSplitComplex& DspSplitComplex() { return frame_; }
  DSPSplitComplex DspSplitComplex() const { return frame_; }
  static FFTSetup FftSetupForSize(unsigned fft_size);
  static FFTSetup* fft_setups_;
  FFTSetup fft_setup_;
  DSPSplitComplex frame_;
#elif defined(WTF_USE_WEBAUDIO_FFMPEG)
  static RDFTContext* ContextForSize(unsigned fft_size, int trans);
  RDFTContext* forward_context_;
  RDFTContext* inverse_context_;
  float* GetUpToDateComplexData();
  AudioFloatArray complex_data_;
#elif defined(WTF_USE_WEBAUDIO_OPENMAX_DL_FFT)
  static OMXFFTSpec_R_F32* ContextForSize(unsigned log2fft_size);
  OMXFFTSpec_R_F32* forward_context_;
  OMXFFTSpec_R_F32* inverse_context_;
  AudioFloatArray complex_data_;
#endif
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_AUDIO_FFT_FRAME_H_
