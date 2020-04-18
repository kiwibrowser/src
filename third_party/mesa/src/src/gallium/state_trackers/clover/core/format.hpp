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

#ifndef __CORE_FORMAT_HPP__
#define __CORE_FORMAT_HPP__

#include <set>

#include "core/base.hpp"
#include "pipe/p_defines.h"
#include "pipe/p_format.h"

namespace clover {
   pipe_texture_target translate_target(cl_mem_object_type type);
   pipe_format translate_format(const cl_image_format &format);

   ///
   /// Return all the image formats supported by a given context for
   /// the given memory object type.
   ///
   std::set<cl_image_format> supported_formats(cl_context ctx,
                                               cl_mem_object_type type);
}

static inline bool
operator<(const cl_image_format &a, const cl_image_format &b) {
   return (a.image_channel_order != b.image_channel_order ?
           a.image_channel_order < b.image_channel_order :
           a.image_channel_data_type < b.image_channel_data_type);
}

#endif
