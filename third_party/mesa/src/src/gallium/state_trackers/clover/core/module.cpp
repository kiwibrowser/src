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

#include <type_traits>
#include <algorithm>

#include "core/module.hpp"

using namespace clover;

namespace {
   template<typename T, typename = void>
   struct __serializer;

   /// Serialize the specified object.
   template<typename T>
   void
   __proc(compat::ostream &os, const T &x) {
      __serializer<T>::proc(os, x);
   }

   /// Deserialize the specified object.
   template<typename T>
   void
   __proc(compat::istream &is, T &x) {
      __serializer<T>::proc(is, x);
   }

   template<typename T>
   T
   __proc(compat::istream &is) {
      T x;
      __serializer<T>::proc(is, x);
      return x;
   }

   /// (De)serialize a scalar value.
   template<typename T>
   struct __serializer<T, typename std::enable_if<
                             std::is_scalar<T>::value>::type> {
      static void
      proc(compat::ostream &os, const T &x) {
         os.write(reinterpret_cast<const char *>(&x), sizeof(x));
      }

      static void
      proc(compat::istream &is, T &x) {
         is.read(reinterpret_cast<char *>(&x), sizeof(x));
      }
   };

   /// (De)serialize a vector.
   template<typename T>
   struct __serializer<compat::vector<T>> {
      static void
      proc(compat::ostream &os, const compat::vector<T> &v) {
         __proc<uint32_t>(os, v.size());

         for (size_t i = 0; i < v.size(); i++)
            __proc<T>(os, v[i]);
      }

      static void
      proc(compat::istream &is, compat::vector<T> &v) {
         v.reserve(__proc<uint32_t>(is));

         for (size_t i = 0; i < v.size(); i++)
            new(&v[i]) T(__proc<T>(is));
      }
   };

   /// (De)serialize a module::section.
   template<>
   struct __serializer<module::section> {
      template<typename S, typename QT>
      static void
      proc(S &s, QT &x) {
         __proc(s, x.type);
         __proc(s, x.size);
         __proc(s, x.data);
      }
   };

   /// (De)serialize a module::argument.
   template<>
   struct __serializer<module::argument> {
      template<typename S, typename QT>
      static void
      proc(S &s, QT &x) {
         __proc(s, x.type);
         __proc(s, x.size);
      }
   };

   /// (De)serialize a module::symbol.
   template<>
   struct __serializer<module::symbol> {
      template<typename S, typename QT>
      static void
      proc(S &s, QT &x) {
         __proc(s, x.section);
         __proc(s, x.offset);
         __proc(s, x.args);
      }
   };

   /// (De)serialize a module.
   template<>
   struct __serializer<module> {
      template<typename S, typename QT>
      static void
      proc(S &s, QT &x) {
         __proc(s, x.syms);
         __proc(s, x.secs);
      }
   };
};

namespace clover {
   void
   module::serialize(compat::ostream &os) const {
      __proc(os, *this);
   }

   module
   module::deserialize(compat::istream &is) {
      return __proc<module>(is);
   }

   const module::symbol &
   module::sym(compat::string name) const {
      auto it = std::find_if(syms.begin(), syms.end(), [&](const symbol &x) {
            return compat::string(x.name) == name;
         });

      if (it == syms.end())
         throw noent_error();

      return *it;
   }

   const module::section &
   module::sec(typename section::type type) const {
      auto it = std::find_if(secs.begin(), secs.end(), [&](const section &x) {
            return x.type == type;
         });

      if (it == secs.end())
         throw noent_error();

      return *it;
   }
}
