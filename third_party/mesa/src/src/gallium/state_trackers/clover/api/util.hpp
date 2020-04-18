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

#ifndef __CL_UTIL_HPP__
#define __CL_UTIL_HPP__

#include <cstdint>
#include <cstring>
#include <algorithm>
#include <map>

#include "core/base.hpp"
#include "pipe/p_compiler.h"

namespace clover {
   ///
   /// Return a matrix (a container of containers) in \a buf with
   /// argument and bounds checking.  Intended to be used by
   /// implementations of \a clGetXXXInfo().
   ///
   template<typename T, typename V>
   cl_int
   matrix_property(void *buf, size_t size, size_t *size_ret, const V& v) {
      if (buf && size < sizeof(T *) * v.size())
         return CL_INVALID_VALUE;

      if (size_ret)
         *size_ret = sizeof(T *) * v.size();

      if (buf)
         for_each([](typename V::value_type src, T *dst) {
               if (dst)
                  std::copy(src.begin(), src.end(), dst);
            },
            v.begin(), v.end(), (T **)buf);

      return CL_SUCCESS;
   }

   ///
   /// Return a vector in \a buf with argument and bounds checking.
   /// Intended to be used by implementations of \a clGetXXXInfo().
   ///
   template<typename T, typename V>
   cl_int
   vector_property(void *buf, size_t size, size_t *size_ret, const V& v) {
      if (buf && size < sizeof(T) * v.size())
         return CL_INVALID_VALUE;

      if (size_ret)
         *size_ret = sizeof(T) * v.size();
      if (buf)
         std::copy(v.begin(), v.end(), (T *)buf);

      return CL_SUCCESS;
   }

   ///
   /// Return a scalar in \a buf with argument and bounds checking.
   /// Intended to be used by implementations of \a clGetXXXInfo().
   ///
   template<typename T>
   cl_int
   scalar_property(void *buf, size_t size, size_t *size_ret, T v) {
      return vector_property<T>(buf, size, size_ret, std::vector<T>(1, v));
   }

   ///
   /// Return a string in \a buf with argument and bounds checking.
   /// Intended to be used by implementations of \a clGetXXXInfo().
   ///
   inline cl_int
   string_property(void *buf, size_t size, size_t *size_ret,
                   const std::string &v) {
      if (buf && size < v.size() + 1)
         return CL_INVALID_VALUE;

      if (size_ret)
         *size_ret = v.size() + 1;
      if (buf)
         std::strcpy((char *)buf, v.c_str());

      return CL_SUCCESS;
   }

   ///
   /// Convert a NULL-terminated property list into an std::map.
   ///
   template<typename T>
   std::map<T, T>
   property_map(const T *props) {
      std::map<T, T> m;

      while (props && *props) {
         T key = *props++;
         T value = *props++;

         if (m.count(key))
            throw clover::error(CL_INVALID_PROPERTY);

         m.insert({ key, value });
      }

      return m;
   }

   ///
   /// Convert an std::map into a NULL-terminated property list.
   ///
   template<typename T>
   std::vector<T>
   property_vector(const std::map<T, T> &m) {
      std::vector<T> v;

      for (auto &p : m) {
         v.push_back(p.first);
         v.push_back(p.second);
      }

      v.push_back(0);
      return v;
   }

   ///
   /// Return an error code in \a p if non-zero.
   ///
   inline void
   ret_error(cl_int *p, const clover::error &e) {
      if (p)
         *p = e.get();
   }

   ///
   /// Return a reference-counted object in \a p if non-zero.
   /// Otherwise release object ownership.
   ///
   template<typename T, typename S>
   void
   ret_object(T p, S v) {
      if (p)
         *p = v;
      else
         v->release();
   }
}

#endif
