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

#include <sstream>

#include "core/compiler.hpp"

#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_text.h"
#include "util/u_memory.h"

using namespace clover;

namespace {
   void
   read_header(const std::string &header, module &m) {
      std::istringstream ls(header);
      std::string line;

      while (getline(ls, line)) {
         std::istringstream ts(line);
         std::string name, tok;
         module::size_t offset;
         compat::vector<module::argument> args;

         if (!(ts >> name))
            continue;

         if (!(ts >> offset))
            throw build_error("invalid kernel start address");

         while (ts >> tok) {
            if (tok == "scalar")
               args.push_back({ module::argument::scalar, 4 });
            else if (tok == "global")
               args.push_back({ module::argument::global, 4 });
            else if (tok == "local")
               args.push_back({ module::argument::local, 4 });
            else if (tok == "constant")
               args.push_back({ module::argument::constant, 4 });
            else if (tok == "image2d_rd")
               args.push_back({ module::argument::image2d_rd, 4 });
            else if (tok == "image2d_wr")
               args.push_back({ module::argument::image2d_wr, 4 });
            else if (tok == "image3d_rd")
               args.push_back({ module::argument::image3d_rd, 4 });
            else if (tok == "image3d_wr")
               args.push_back({ module::argument::image3d_wr, 4 });
            else if (tok == "sampler")
               args.push_back({ module::argument::sampler, 0 });
            else
               throw build_error("invalid kernel argument");
         }

         m.syms.push_back({ name, 0, offset, args });
      }
   }

   void
   read_body(const char *source, module &m) {
      tgsi_token prog[1024];

      if (!tgsi_text_translate(source, prog, Elements(prog)))
         throw build_error("translate failed");

      unsigned sz = tgsi_num_tokens(prog) * sizeof(tgsi_token);
      m.secs.push_back({ 0, module::section::text, sz, { (char *)prog, sz } });
   }
}

module
clover::compile_program_tgsi(const compat::string &source) {
   const char *body = source.find("COMP\n");
   module m;

   read_header({ source.begin(), body }, m);
   read_body(body, m);

   return m;
}
