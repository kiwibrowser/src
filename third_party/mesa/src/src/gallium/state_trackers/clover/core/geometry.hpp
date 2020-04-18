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

#ifndef __CORE_GEOMETRY_HPP__
#define __CORE_GEOMETRY_HPP__

#include <array>
#include <algorithm>

namespace clover {
   ///
   /// N-dimensional coordinate array.
   ///
   template<typename T, int N>
   class point {
   public:
      point() : a() {
      }

      point(std::initializer_list<T> v) {
         auto it = std::copy(v.begin(), v.end(), a.begin());
         std::fill(it, a.end(), 0);
      }

      point(const T *v) {
         std::copy(v, v + N, a.begin());
      }

      T &operator[](int i) {
         return a[i];
      }

      const T &operator[](int i) const {
         return a[i];
      }

      point operator+(const point &p) const {
         point q;
         std::transform(a.begin(), a.end(), p.a.begin(),
                        q.a.begin(), std::plus<T>());
         return q;
      }

      T operator()(const point &p) const {
         return std::inner_product(p.a.begin(), p.a.end(), a.begin(), 0);
      }

   protected:
      std::array<T, N> a;
   };
}

#endif
