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

#ifndef __CORE_BASE_HPP__
#define __CORE_BASE_HPP__

#include <stdexcept>
#include <atomic>
#include <cassert>
#include <tuple>
#include <vector>
#include <functional>

#include "CL/cl.h"

///
/// Main namespace of the CL state tracker.
///
namespace clover {
   ///
   /// Class that represents an error that can be converted to an
   /// OpenCL status code.
   ///
   class error : public std::runtime_error {
   public:
      error(cl_int code, std::string what = "") :
         std::runtime_error(what), code(code) {
      }

      cl_int get() const {
         return code;
      }

   protected:
      cl_int code;
   };

   ///
   /// Base class for objects that support reference counting.
   ///
   class ref_counter {
   public:
      ref_counter() : __ref_count(1) {}

      unsigned ref_count() {
         return __ref_count;
      }

      void retain() {
         __ref_count++;
      }

      bool release() {
         return (--__ref_count) == 0;
      }

   private:
      std::atomic<unsigned> __ref_count;
   };

   ///
   /// Intrusive smart pointer for objects that implement the
   /// clover::ref_counter interface.
   ///
   template<typename T>
   class ref_ptr {
   public:
      ref_ptr(T *q = NULL) : p(NULL) {
         reset(q);
      }

      ref_ptr(const ref_ptr<T> &ref) : p(NULL) {
         reset(ref.p);
      }

      ~ref_ptr() {
         reset(NULL);
      }

      void reset(T *q = NULL) {
         if (q)
            q->retain();
         if (p && p->release())
            delete p;
         p = q;
      }

      ref_ptr &operator=(const ref_ptr &ref) {
         reset(ref.p);
         return *this;
      }

      T *operator*() const {
         return p;
      }

      T *operator->() const {
         return p;
      }

      operator bool() const {
         return p;
      }

   private:
      T *p;
   };

   ///
   /// Transfer the caller's ownership of a reference-counted object
   /// to a clover::ref_ptr smart pointer.
   ///
   template<typename T>
   inline ref_ptr<T>
   transfer(T *p) {
      ref_ptr<T> ref { p };
      p->release();
      return ref;
   }

   template<typename T, typename S, int N>
   struct __iter_helper {
      template<typename F, typename Its, typename... Args>
      static T
      step(F op, S state, Its its, Args... args) {
         return __iter_helper<T, S, N - 1>::step(
            op, state, its, *(std::get<N>(its)++), args...);
      }
   };

   template<typename T, typename S>
   struct __iter_helper<T, S, 0> {
      template<typename F, typename Its, typename... Args>
      static T
      step(F op, S state, Its its, Args... args) {
         return op(state, *(std::get<0>(its)++), args...);
      }
   };

   struct __empty {};

   template<typename T>
   struct __iter_helper<T, __empty, 0> {
      template<typename F, typename Its, typename... Args>
      static T
      step(F op, __empty state, Its its, Args... args) {
         return op(*(std::get<0>(its)++), args...);
      }
   };

   template<typename F, typename... Its>
   struct __result_helper {
      typedef typename std::remove_const<
         typename std::result_of<
            F (typename std::iterator_traits<Its>::value_type...)
            >::type
         >::type type;
   };

   ///
   /// Iterate \a op on the result of zipping all the specified
   /// iterators together.
   ///
   /// Similar to std::for_each, but it accepts functions of an
   /// arbitrary number of arguments.
   ///
   template<typename F, typename It0, typename... Its>
   F
   for_each(F op, It0 it0, It0 end0, Its... its) {
      while (it0 != end0)
         __iter_helper<void, __empty, sizeof...(Its)>::step(
            op, {}, std::tie(it0, its...));

      return op;
   }

   ///
   /// Iterate \a op on the result of zipping all the specified
   /// iterators together, storing return values in a new container.
   ///
   /// Similar to std::transform, but it accepts functions of an
   /// arbitrary number of arguments and it doesn't have to be
   /// provided with an output iterator.
   ///
   template<typename F, typename It0, typename... Its,
            typename C = std::vector<
               typename __result_helper<F, It0, Its...>::type>>
   C
   map(F op, It0 it0, It0 end0, Its... its) {
      C c;

      while (it0 != end0)
         c.push_back(
            __iter_helper<typename C::value_type, __empty, sizeof...(Its)>
            ::step(op, {}, std::tie(it0, its...)));

      return c;
   }

   ///
   /// Reduce the result of zipping all the specified iterators
   /// together, using iterative application of \a op from left to
   /// right.
   ///
   /// Similar to std::accumulate, but it accepts functions of an
   /// arbitrary number of arguments.
   ///
   template<typename F, typename T, typename It0, typename... Its>
   T
   fold(F op, T a, It0 it0, It0 end0, Its... its) {
      while (it0 != end0)
         a = __iter_helper<T, T, sizeof...(Its)>::step(
            op, a, std::tie(it0, its...));

      return a;
   }

   ///
   /// Iterate \a op on the result of zipping the specified iterators
   /// together, checking if any of the evaluations returns \a true.
   ///
   /// Similar to std::any_of, but it accepts functions of an
   /// arbitrary number of arguments.
   ///
   template<typename F, typename It0, typename... Its>
   bool
   any_of(F op, It0 it0, It0 end0, Its... its) {
      while (it0 != end0)
         if (__iter_helper<bool, __empty, sizeof...(Its)>::step(
                op, {}, std::tie(it0, its...)))
            return true;

      return false;
   }

   template<typename T, typename S>
   T
   keys(const std::pair<T, S> &ent) {
      return ent.first;
   }

   template<typename T, typename S>
   std::function<bool (const std::pair<T, S> &)>
   key_equals(const T &x) {
      return [=](const std::pair<T, S> &ent) {
         return ent.first == x;
      };
   }

   template<typename T, typename S>
   S
   values(const std::pair<T, S> &ent) {
      return ent.second;
   }

   template<typename T>
   std::function<bool (const T &)>
   is_zero() {
      return [](const T &x) {
         return x == 0;
      };
   }
}

#endif
