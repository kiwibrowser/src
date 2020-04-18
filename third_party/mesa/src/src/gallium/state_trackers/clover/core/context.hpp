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

#ifndef __CORE_CONTEXT_HPP__
#define __CORE_CONTEXT_HPP__

#include "core/base.hpp"
#include "core/device.hpp"

namespace clover {
   typedef struct _cl_context context;
}

struct _cl_context : public clover::ref_counter {
public:
   _cl_context(const std::vector<cl_context_properties> &props,
               const std::vector<clover::device *> &devs);
   _cl_context(const _cl_context &ctx) = delete;

   bool has_device(clover::device *dev) const;

   const std::vector<cl_context_properties> &props() const {
      return __props;
   }

   const std::vector<clover::device *> devs;

private:
   std::vector<cl_context_properties> __props;
};

#endif
