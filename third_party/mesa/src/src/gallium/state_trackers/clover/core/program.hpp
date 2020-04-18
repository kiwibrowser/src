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

#ifndef __CORE_PROGRAM_HPP__
#define __CORE_PROGRAM_HPP__

#include <map>

#include "core/base.hpp"
#include "core/context.hpp"
#include "core/module.hpp"

namespace clover {
   typedef struct _cl_program program;
}

struct _cl_program : public clover::ref_counter {
public:
   _cl_program(clover::context &ctx,
               const std::string &source);
   _cl_program(clover::context &ctx,
               const std::vector<clover::device *> &devs,
               const std::vector<clover::module> &binaries);

   void build(const std::vector<clover::device *> &devs);

   const std::string &source() const;
   const std::map<clover::device *, clover::module> &binaries() const;

   cl_build_status build_status(clover::device *dev) const;
   std::string build_opts(clover::device *dev) const;
   std::string build_log(clover::device *dev) const;

   clover::context &ctx;

private:
   std::map<clover::device *, clover::module> __binaries;
   std::map<clover::device *, std::string> __logs;
   std::string __source;
};

#endif
