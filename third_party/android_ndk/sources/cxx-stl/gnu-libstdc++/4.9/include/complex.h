// -*- C++ -*- compatibility header.

// Copyright (C) 2007-2014 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file complex.h
 *  This is a Standard C++ Library header.
 */

#ifndef _GLIBCXX_COMPLEX_H
#define _GLIBCXX_COMPLEX_H 1

#if __cplusplus >= 201103L
# include <ccomplex>
#else // C++98 and C++03

// The C++ <complex> header is incompatible with the C99 <complex.h> header,
// they cannot be included into a single translation unit portably. Notably,
// C++11's <ccomplex> does not include C99's <complex.h> and in C++11's
// <complex.h> is defined to provide only what C++11's <ccomplex> does in a
// different namespace.
#ifdef _GLIBCXX_COMPLEX
# error "Cannot include both <complex> and C99 <complex.h>"
#endif

// Delegate to a system complex.h if we don't provide it as part of the C++
// implementation.
#include_next <complex.h>

// Provide a define indicating that a C99-style <complex.h> has been included.
#define _GLIBCXX_C99_COMPLEX_H

#endif // C++98 and C++03

#endif
