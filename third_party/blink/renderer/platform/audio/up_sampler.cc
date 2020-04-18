/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/audio/up_sampler.h"

#include <memory>

#include "third_party/blink/renderer/platform/wtf/math_extras.h"

namespace blink {

namespace {

// Computes ideal band-limited filter coefficients to sample in between each
// source sample-frame.  This filter will be used to compute the odd
// sample-frames of the output.
std::unique_ptr<AudioFloatArray> MakeKernel(size_t size) {
  std::unique_ptr<AudioFloatArray> kernel =
      std::make_unique<AudioFloatArray>(size);

  // Blackman window parameters.
  double alpha = 0.16;
  double a0 = 0.5 * (1.0 - alpha);
  double a1 = 0.5;
  double a2 = 0.5 * alpha;

  int n = kernel->size();
  int half_size = n / 2;
  double subsample_offset = -0.5;

  for (int i = 0; i < n; ++i) {
    // Compute the sinc() with offset.
    double s = piDouble * (i - half_size - subsample_offset);
    double sinc = !s ? 1.0 : sin(s) / s;

    // Compute Blackman window, matching the offset of the sinc().
    double x = (i - subsample_offset) / n;
    double window =
        a0 - a1 * cos(twoPiDouble * x) + a2 * cos(twoPiDouble * 2.0 * x);

    // Window the sinc() function.
    (*kernel)[i] = sinc * window;
  }

  return kernel;
}

}  // namespace

UpSampler::UpSampler(size_t input_block_size)
    : input_block_size_(input_block_size),
      convolver_(input_block_size, MakeKernel(kDefaultKernelSize)),
      temp_buffer_(input_block_size),
      input_buffer_(input_block_size * 2) {}

void UpSampler::Process(const float* source_p,
                        float* dest_p,
                        size_t source_frames_to_process) {
  bool is_input_block_size_good = source_frames_to_process == input_block_size_;
  DCHECK(is_input_block_size_good);
  if (!is_input_block_size_good)
    return;

  bool is_temp_buffer_good = source_frames_to_process == temp_buffer_.size();
  DCHECK(is_temp_buffer_good);
  if (!is_temp_buffer_good)
    return;

  bool is_kernel_good =
      convolver_.ConvolutionKernelSize() == kDefaultKernelSize;
  DCHECK(is_kernel_good);
  if (!is_kernel_good)
    return;

  size_t half_size = convolver_.ConvolutionKernelSize() / 2;

  // Copy source samples to 2nd half of input buffer.
  bool is_input_buffer_good =
      input_buffer_.size() == source_frames_to_process * 2 &&
      half_size <= source_frames_to_process;
  DCHECK(is_input_buffer_good);
  if (!is_input_buffer_good)
    return;

  float* input_p = input_buffer_.Data() + source_frames_to_process;
  memcpy(input_p, source_p, sizeof(float) * source_frames_to_process);

  // Copy even sample-frames 0,2,4,6... (delayed by the linear phase delay)
  // directly into destP.
  for (unsigned i = 0; i < source_frames_to_process; ++i)
    dest_p[i * 2] = *((input_p - half_size) + i);

  // Compute odd sample-frames 1,3,5,7...
  float* odd_samples_p = temp_buffer_.Data();
  convolver_.Process(source_p, odd_samples_p, source_frames_to_process);

  for (unsigned i = 0; i < source_frames_to_process; ++i)
    dest_p[i * 2 + 1] = odd_samples_p[i];

  // Copy 2nd half of input buffer to 1st half.
  memcpy(input_buffer_.Data(), input_p,
         sizeof(float) * source_frames_to_process);
}

void UpSampler::Reset() {
  convolver_.Reset();
  input_buffer_.Zero();
}

size_t UpSampler::LatencyFrames() const {
  // Divide by two since this is a linear phase kernel and the delay is at the
  // center of the kernel.
  return convolver_.ConvolutionKernelSize() / 2;
}

}  // namespace blink
