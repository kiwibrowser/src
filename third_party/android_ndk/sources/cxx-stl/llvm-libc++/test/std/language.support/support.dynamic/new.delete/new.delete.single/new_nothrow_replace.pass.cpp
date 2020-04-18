//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// test operator new nothrow by replacing only operator new

// UNSUPPORTED: sanitizer-new-delete

#include <new>
#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <limits>

#include "count_new.hpp"
#include "test_macros.h"

bool A_constructed = false;

struct A
{
    A() {A_constructed = true;}
    ~A() {A_constructed = false;}
};

A* volatile ap;

int main()
{
    globalMemCounter.reset();
    assert(globalMemCounter.checkOutstandingNewEq(0));
    ap = new (std::nothrow) A;
    assert(ap);
    assert(A_constructed);
    assert(globalMemCounter.checkOutstandingNewNotEq(0));
    delete ap;
    assert(!A_constructed);
    assert(globalMemCounter.checkOutstandingNewEq(0));
}
