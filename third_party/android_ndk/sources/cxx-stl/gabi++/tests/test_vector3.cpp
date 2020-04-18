//===------------------------- test_vector3.cpp ---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "cxxabi.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <exception>

//#include <memory>

// use dtors instead of try/catch
namespace test1 {
    struct B {
         ~B() {
            printf("should not be run\n");
            exit(10);
            }
};

struct A {
 ~A()
#if __cplusplus >= 201103L
    noexcept(false)
#endif
 {
   B b;
   throw 0;
 }
};
}  // test1

void my_terminate() { exit(0); }

#ifdef __arm__
#define CTOR_RETURN_TYPE void*
#define CTOR_RETURN(x) return x
#else
#define CTOR_RETURN_TYPE void
#define CTOR_RETURN(x) return
#endif

template <class T>
CTOR_RETURN_TYPE destroy(void* v)
{
  T* t = static_cast<T*>(v);
  t->~T();
  CTOR_RETURN(v);
}

int main( int argc, char *argv [])
{
  std::set_terminate(my_terminate);
  {
  typedef test1::A Array[10];
  Array a[10]; // calls _cxa_vec_dtor
  __cxxabiv1::__cxa_vec_dtor(a, 10, sizeof(test1::A), destroy<test1::A>);
  assert(false);
  }
}
