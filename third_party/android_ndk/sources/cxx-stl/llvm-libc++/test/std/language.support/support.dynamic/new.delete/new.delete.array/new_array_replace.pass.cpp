//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// test operator new[] replacement by replacing only operator new

// UNSUPPORTED: sanitizer-new-delete


#include <new>
#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <limits>

#include "count_new.hpp"
#include "test_macros.h"

int A_constructed = 0;

struct A
{
    A() {++A_constructed;}
    ~A() {--A_constructed;}
};

A* volatile ap;

int main()
{
    globalMemCounter.reset();
    assert(globalMemCounter.checkOutstandingNewEq(0));
    ap = new A[3];
    assert(ap);
    assert(A_constructed == 3);
    assert(globalMemCounter.checkOutstandingNewEq(1));
    delete [] ap;
    assert(A_constructed == 0);
    assert(globalMemCounter.checkOutstandingNewEq(0));
}
