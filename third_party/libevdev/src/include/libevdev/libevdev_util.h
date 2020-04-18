/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef _LIBEVDEV_UTIL_H_
#define _LIBEVDEV_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

// Helper for bit operations
#define LONG_BITS (sizeof(long) * 8)
#define NLONGS(x) (((x) + LONG_BITS - 1) / LONG_BITS)

// Implementation of inline bit operations
static inline bool TestBit(int bit, unsigned long* array)
{
    return !!(array[bit / LONG_BITS] & (1L << (bit % LONG_BITS)));
}

static inline void AssignBit(unsigned long* array, int bit, int value)
{
    unsigned long mask = (1L << (bit % LONG_BITS));
    if (value)
        array[bit / LONG_BITS] |= mask;
    else
        array[bit / LONG_BITS] &= ~mask;
}

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
