/* Unaligned memory access functionality.
   Copyright (C) 2000-2013 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2001.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#ifndef _MEMORY_ACCESS_H
#define _MEMORY_ACCESS_H 1

#include <byteswap.h>
#include <limits.h>
#include <stdint.h>


/* Number decoding macros.  See 7.6 Variable Length Data.  */

#define len_leb128(var) ((8 * sizeof (var) + 6) / 7)

#define get_uleb128_step(var, addr, nth)				      \
  do {									      \
    unsigned char __b = *(addr)++;					      \
    (var) |= (typeof (var)) (__b & 0x7f) << ((nth) * 7);		      \
    if (likely ((__b & 0x80) == 0))					      \
      return (var);							      \
  } while (0)

static inline uint64_t
__libdw_get_uleb128 (const unsigned char **addrp)
{
  uint64_t acc = 0;
  /* Unroll the first step to help the compiler optimize
     for the common single-byte case.  */
  get_uleb128_step (acc, *addrp, 0);
  for (unsigned int i = 1; i < len_leb128 (acc); ++i)
    get_uleb128_step (acc, *addrp, i);
  /* Other implementations set VALUE to UINT_MAX in this
     case.  So we better do this as well.  */
  return UINT64_MAX;
}

#define get_uleb128(var, addr) ((var) = __libdw_get_uleb128 (&(addr)))

/* The signed case is similar, but we sign-extend the result.  */

#define get_sleb128_step(var, addr, nth)				      \
  do {									      \
    unsigned char __b = *(addr)++;					      \
    if (likely ((__b & 0x80) == 0))					      \
      {									      \
	struct { signed int i:7; } __s = { .i = __b };			      \
	(var) |= (typeof (var)) __s.i << ((nth) * 7);			      \
	return (var);							      \
      }									      \
    (var) |= (typeof (var)) (__b & 0x7f) << ((nth) * 7);		      \
  } while (0)

static inline int64_t
__libdw_get_sleb128 (const unsigned char **addrp)
{
  int64_t acc = 0;
  /* Unrolling 0 like uleb128 didn't prove to benefit optimization.  */
  for (unsigned int i = 0; i < len_leb128 (acc); ++i)
    get_sleb128_step (acc, *addrp, i);
  /* Other implementations set VALUE to INT_MAX in this
     case.  So we better do this as well.  */
  return INT64_MAX;
}

#define get_sleb128(var, addr) ((var) = __libdw_get_sleb128 (&(addr)))


/* We use simple memory access functions in case the hardware allows it.
   The caller has to make sure we don't have alias problems.  */
#if ALLOW_UNALIGNED

# define read_2ubyte_unaligned(Dbg, Addr) \
  (unlikely ((Dbg)->other_byte_order)					      \
   ? bswap_16 (*((const uint16_t *) (Addr)))				      \
   : *((const uint16_t *) (Addr)))
# define read_2sbyte_unaligned(Dbg, Addr) \
  (unlikely ((Dbg)->other_byte_order)					      \
   ? (int16_t) bswap_16 (*((const int16_t *) (Addr)))			      \
   : *((const int16_t *) (Addr)))

# define read_4ubyte_unaligned_noncvt(Addr) \
   *((const uint32_t *) (Addr))
# define read_4ubyte_unaligned(Dbg, Addr) \
  (unlikely ((Dbg)->other_byte_order)					      \
   ? bswap_32 (*((const uint32_t *) (Addr)))				      \
   : *((const uint32_t *) (Addr)))
# define read_4sbyte_unaligned(Dbg, Addr) \
  (unlikely ((Dbg)->other_byte_order)					      \
   ? (int32_t) bswap_32 (*((const int32_t *) (Addr)))			      \
   : *((const int32_t *) (Addr)))

# define read_8ubyte_unaligned_noncvt(Addr) \
   *((const uint64_t *) (Addr))
# define read_8ubyte_unaligned(Dbg, Addr) \
  (unlikely ((Dbg)->other_byte_order)					      \
   ? bswap_64 (*((const uint64_t *) (Addr)))				      \
   : *((const uint64_t *) (Addr)))
# define read_8sbyte_unaligned(Dbg, Addr) \
  (unlikely ((Dbg)->other_byte_order)					      \
   ? (int64_t) bswap_64 (*((const int64_t *) (Addr)))			      \
   : *((const int64_t *) (Addr)))

#else

union unaligned
  {
    void *p;
    uint16_t u2;
    uint32_t u4;
    uint64_t u8;
    int16_t s2;
    int32_t s4;
    int64_t s8;
  } __attribute__ ((packed));

# define read_2ubyte_unaligned(Dbg, Addr) \
  read_2ubyte_unaligned_1 ((Dbg)->other_byte_order, (Addr))
# define read_2sbyte_unaligned(Dbg, Addr) \
  read_2sbyte_unaligned_1 ((Dbg)->other_byte_order, (Addr))
# define read_4ubyte_unaligned(Dbg, Addr) \
  read_4ubyte_unaligned_1 ((Dbg)->other_byte_order, (Addr))
# define read_4sbyte_unaligned(Dbg, Addr) \
  read_4sbyte_unaligned_1 ((Dbg)->other_byte_order, (Addr))
# define read_8ubyte_unaligned(Dbg, Addr) \
  read_8ubyte_unaligned_1 ((Dbg)->other_byte_order, (Addr))
# define read_8sbyte_unaligned(Dbg, Addr) \
  read_8sbyte_unaligned_1 ((Dbg)->other_byte_order, (Addr))

static inline uint16_t
read_2ubyte_unaligned_1 (bool other_byte_order, const void *p)
{
  const union unaligned *up = p;
  if (unlikely (other_byte_order))
    return bswap_16 (up->u2);
  return up->u2;
}
static inline int16_t
read_2sbyte_unaligned_1 (bool other_byte_order, const void *p)
{
  const union unaligned *up = p;
  if (unlikely (other_byte_order))
    return (int16_t) bswap_16 (up->u2);
  return up->s2;
}

static inline uint32_t
read_4ubyte_unaligned_noncvt (const void *p)
{
  const union unaligned *up = p;
  return up->u4;
}
static inline uint32_t
read_4ubyte_unaligned_1 (bool other_byte_order, const void *p)
{
  const union unaligned *up = p;
  if (unlikely (other_byte_order))
    return bswap_32 (up->u4);
  return up->u4;
}
static inline int32_t
read_4sbyte_unaligned_1 (bool other_byte_order, const void *p)
{
  const union unaligned *up = p;
  if (unlikely (other_byte_order))
    return (int32_t) bswap_32 (up->u4);
  return up->s4;
}

static inline uint64_t
read_8ubyte_unaligned_noncvt (const void *p)
{
  const union unaligned *up = p;
  return up->u8;
}
static inline uint64_t
read_8ubyte_unaligned_1 (bool other_byte_order, const void *p)
{
  const union unaligned *up = p;
  if (unlikely (other_byte_order))
    return bswap_64 (up->u8);
  return up->u8;
}
static inline int64_t
read_8sbyte_unaligned_1 (bool other_byte_order, const void *p)
{
  const union unaligned *up = p;
  if (unlikely (other_byte_order))
    return (int64_t) bswap_64 (up->u8);
  return up->s8;
}

#endif	/* allow unaligned */


#define read_ubyte_unaligned(Nbytes, Dbg, Addr) \
  ((Nbytes) == 2 ? read_2ubyte_unaligned (Dbg, Addr)			      \
   : (Nbytes) == 4 ? read_4ubyte_unaligned (Dbg, Addr)			      \
   : read_8ubyte_unaligned (Dbg, Addr))

#define read_sbyte_unaligned(Nbytes, Dbg, Addr) \
  ((Nbytes) == 2 ? read_2sbyte_unaligned (Dbg, Addr)			      \
   : (Nbytes) == 4 ? read_4sbyte_unaligned (Dbg, Addr)			      \
   : read_8sbyte_unaligned (Dbg, Addr))


#define read_2ubyte_unaligned_inc(Dbg, Addr) \
  ({ uint16_t t_ = read_2ubyte_unaligned (Dbg, Addr);			      \
     Addr = (__typeof (Addr)) (((uintptr_t) (Addr)) + 2);		      \
     t_; })
#define read_2sbyte_unaligned_inc(Dbg, Addr) \
  ({ int16_t t_ = read_2sbyte_unaligned (Dbg, Addr);			      \
     Addr = (__typeof (Addr)) (((uintptr_t) (Addr)) + 2);		      \
     t_; })

#define read_4ubyte_unaligned_inc(Dbg, Addr) \
  ({ uint32_t t_ = read_4ubyte_unaligned (Dbg, Addr);			      \
     Addr = (__typeof (Addr)) (((uintptr_t) (Addr)) + 4);		      \
     t_; })
#define read_4sbyte_unaligned_inc(Dbg, Addr) \
  ({ int32_t t_ = read_4sbyte_unaligned (Dbg, Addr);			      \
     Addr = (__typeof (Addr)) (((uintptr_t) (Addr)) + 4);		      \
     t_; })

#define read_8ubyte_unaligned_inc(Dbg, Addr) \
  ({ uint64_t t_ = read_8ubyte_unaligned (Dbg, Addr);			      \
     Addr = (__typeof (Addr)) (((uintptr_t) (Addr)) + 8);		      \
     t_; })
#define read_8sbyte_unaligned_inc(Dbg, Addr) \
  ({ int64_t t_ = read_8sbyte_unaligned (Dbg, Addr);			      \
     Addr = (__typeof (Addr)) (((uintptr_t) (Addr)) + 8);		      \
     t_; })


#define read_ubyte_unaligned_inc(Nbytes, Dbg, Addr) \
  ((Nbytes) == 2 ? read_2ubyte_unaligned_inc (Dbg, Addr)		      \
   : (Nbytes) == 4 ? read_4ubyte_unaligned_inc (Dbg, Addr)		      \
   : read_8ubyte_unaligned_inc (Dbg, Addr))

#define read_sbyte_unaligned_inc(Nbytes, Dbg, Addr) \
  ((Nbytes) == 2 ? read_2sbyte_unaligned_inc (Dbg, Addr)		      \
   : (Nbytes) == 4 ? read_4sbyte_unaligned_inc (Dbg, Addr)		      \
   : read_8sbyte_unaligned_inc (Dbg, Addr))

#endif	/* memory-access.h */
