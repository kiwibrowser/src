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

#ifndef __CORE_SAMPLER_HPP__
#define __CORE_SAMPLER_HPP__

#include "core/base.hpp"
#include "core/queue.hpp"

namespace clover {
   typedef struct _cl_sampler sampler;
}

struct _cl_sampler : public clover::ref_counter {
public:
   _cl_sampler(clover::context &ctx, bool norm_mode,
               cl_addressing_mode addr_mode, cl_filter_mode filter_mode);

   bool norm_mode();
   cl_addressing_mode addr_mode();
   cl_filter_mode filter_mode();

   clover::context &ctx;

   friend class _cl_kernel;

private:
   void *bind(clover::command_queue &q);
   void unbind(clover::command_queue &q, void *st);

   bool __norm_mode;
   cl_addressing_mode __addr_mode;
   cl_filter_mode __filter_mode;
};

#endif
