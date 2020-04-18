/* Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "build/build_config.h"

#if defined(OS_ANDROID) && defined(WTF_USE_WEBAUDIO_OPENMAX_DL_FFT)

#include "third_party/blink/renderer/platform/audio/fft_frame.h"

#include <dl/sp/api/armSP.h>
#include <dl/sp/api/omxSP.h>
#include "third_party/blink/renderer/platform/audio/audio_array.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"

namespace blink {

const unsigned kMaxFFTPow2Size = 15;

// Normal constructor: allocates for a given fftSize.
FFTFrame::FFTFrame(unsigned fft_size)
    : fft_size_(fft_size),
      log2fft_size_(static_cast<unsigned>(log2(fft_size))),
      real_data_(fft_size / 2),
      imag_data_(fft_size / 2),
      forward_context_(nullptr),
      inverse_context_(nullptr),
      complex_data_(fft_size) {
  // We only allow power of two.
  DCHECK_EQ(1UL << log2fft_size_, fft_size_);

  forward_context_ = ContextForSize(log2fft_size_);
  inverse_context_ = ContextForSize(log2fft_size_);
}

// Creates a blank/empty frame (interpolate() must later be called).
FFTFrame::FFTFrame()
    : fft_size_(0),
      log2fft_size_(0),
      forward_context_(nullptr),
      inverse_context_(nullptr) {}

// Copy constructor.
FFTFrame::FFTFrame(const FFTFrame& frame)
    : fft_size_(frame.fft_size_),
      log2fft_size_(frame.log2fft_size_),
      real_data_(frame.fft_size_ / 2),
      imag_data_(frame.fft_size_ / 2),
      forward_context_(nullptr),
      inverse_context_(nullptr),
      complex_data_(frame.fft_size_) {
  forward_context_ = ContextForSize(log2fft_size_);
  inverse_context_ = ContextForSize(log2fft_size_);

  // Copy/setup frame data.
  unsigned nbytes = sizeof(float) * (fft_size_ / 2);
  memcpy(RealData(), frame.RealData(), nbytes);
  memcpy(ImagData(), frame.ImagData(), nbytes);
}

void FFTFrame::Initialize() {}

void FFTFrame::Cleanup() {}

FFTFrame::~FFTFrame() {
  if (forward_context_)
    free(forward_context_);
  if (inverse_context_)
    free(inverse_context_);
}

void FFTFrame::DoFFT(const float* data) {
  DCHECK(forward_context_);

  if (forward_context_) {
    AudioFloatArray complex_fft(fft_size_ + 2);

    omxSP_FFTFwd_RToCCS_F32(data, complex_fft.Data(), forward_context_);

    unsigned len = fft_size_ / 2;

    // Split FFT data into real and imaginary arrays.
    const float* c = complex_fft.Data();
    float* real = real_data_.Data();
    float* imag = imag_data_.Data();
    for (unsigned k = 1; k < len; ++k) {
      int index = 2 * k;
      real[k] = c[index];
      imag[k] = c[index + 1];
    }
    real[0] = c[0];
    imag[0] = c[fft_size_];
  }
}

void FFTFrame::DoInverseFFT(float* data) {
  DCHECK(inverse_context_);

  if (inverse_context_) {
    AudioFloatArray fft_data_array(fft_size_ + 2);

    unsigned len = fft_size_ / 2;

    // Pack the real and imaginary data into the complex array format
    float* fft_data = fft_data_array.Data();
    const float* real = real_data_.Data();
    const float* imag = imag_data_.Data();
    for (unsigned k = 1; k < len; ++k) {
      int index = 2 * k;
      fft_data[index] = real[k];
      fft_data[index + 1] = imag[k];
    }
    fft_data[0] = real[0];
    fft_data[1] = 0;
    fft_data[fft_size_] = imag[0];
    fft_data[fft_size_ + 1] = 0;

    omxSP_FFTInv_CCSToR_F32(fft_data, data, inverse_context_);
  }
}

OMXFFTSpec_R_F32* FFTFrame::ContextForSize(unsigned log2fft_size) {
  DCHECK(log2fft_size);
  DCHECK_LE(log2fft_size, kMaxFFTPow2Size);
  int buf_size;
  OMXResult status = omxSP_FFTGetBufSize_R_F32(log2fft_size, &buf_size);

  if (status == OMX_Sts_NoErr) {
    OMXFFTSpec_R_F32* context =
        static_cast<OMXFFTSpec_R_F32*>(malloc(buf_size));
    omxSP_FFTInit_R_F32(context, log2fft_size);
    return context;
  }

  return nullptr;
}

}  // namespace blink

#endif  // #if defined(OS_ANDROID) && !defined(WTF_USE_WEBAUDIO_OPENMAX_DL_FFT)
