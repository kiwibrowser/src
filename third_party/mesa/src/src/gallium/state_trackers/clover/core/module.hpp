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

#ifndef __CORE_MODULE_HPP__
#define __CORE_MODULE_HPP__

#include "core/compat.hpp"

namespace clover {
   struct module {
      class noent_error {
      public:
         virtual ~noent_error() {}
      };

      typedef uint32_t resource_id;
      typedef uint32_t size_t;

      struct section {
         enum type {
            text,
            data_constant,
            data_global,
            data_local,
            data_private
         };

         section(resource_id id, enum type type, size_t size,
                 const clover::compat::vector<char> &data) :
                 id(id), type(type), size(size), data(data) { }
         section() : id(0), type(text), size(0), data() { }

         resource_id id;
         type type;
         size_t size;
         clover::compat::vector<char> data;
      };

      struct argument {
         enum type {
            scalar,
            constant,
            global,
            local,
            image2d_rd,
            image2d_wr,
            image3d_rd,
            image3d_wr,
            sampler
         };

         argument(enum type type, size_t size) : type(type), size(size) { }
         argument() : type(scalar), size(0) { }

         type type;
         size_t size;
      };

      struct symbol {
         symbol(const clover::compat::vector<char> &name, resource_id section,
                size_t offset, const clover::compat::vector<argument> &args) :
                name(name), section(section), offset(offset), args(args) { }
         symbol() : name(), section(0), offset(0), args() { }

         clover::compat::vector<char> name;
         resource_id section;
         size_t offset;
         clover::compat::vector<argument> args;
      };

      void serialize(compat::ostream &os) const;
      static module deserialize(compat::istream &is);

      /// Look up a symbol by name.  Throws module::noent_error if not
      /// found.
      const symbol &sym(compat::string name) const;

      /// Look up a section by type.  Throws module::noent_error if not
      /// found.
      const section &sec(typename section::type type) const;

      clover::compat::vector<symbol> syms;
      clover::compat::vector<section> secs;
   };
}

#endif
