//
// Copyright 2012 Francisco Jerez
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
// OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "core/sampler.hpp"
#include "pipe/p_state.h"

using namespace clover;

_cl_sampler::_cl_sampler(clover::context &ctx, bool norm_mode,
                         cl_addressing_mode addr_mode,
                         cl_filter_mode filter_mode) :
   ctx(ctx), __norm_mode(norm_mode),
   __addr_mode(addr_mode), __filter_mode(filter_mode) {
}

bool
_cl_sampler::norm_mode() {
   return __norm_mode;
}

cl_addressing_mode
_cl_sampler::addr_mode() {
   return __addr_mode;
}

cl_filter_mode
_cl_sampler::filter_mode() {
   return __filter_mode;
}

void *
_cl_sampler::bind(clover::command_queue &q) {
   struct pipe_sampler_state info {};

   info.normalized_coords = norm_mode();

   info.wrap_s = info.wrap_t = info.wrap_r =
      (addr_mode() == CL_ADDRESS_CLAMP_TO_EDGE ? PIPE_TEX_WRAP_CLAMP_TO_EDGE :
       addr_mode() == CL_ADDRESS_CLAMP ? PIPE_TEX_WRAP_CLAMP_TO_BORDER :
       addr_mode() == CL_ADDRESS_REPEAT ? PIPE_TEX_WRAP_REPEAT :
       addr_mode() == CL_ADDRESS_MIRRORED_REPEAT ? PIPE_TEX_WRAP_MIRROR_REPEAT :
       PIPE_TEX_WRAP_CLAMP_TO_EDGE);

   info.min_img_filter = info.mag_img_filter =
      (filter_mode() == CL_FILTER_LINEAR ? PIPE_TEX_FILTER_LINEAR :
       PIPE_TEX_FILTER_NEAREST);

   return q.pipe->create_sampler_state(q.pipe, &info);
}

void
_cl_sampler::unbind(clover::command_queue &q, void *st) {
   q.pipe->delete_sampler_state(q.pipe, st);
}
