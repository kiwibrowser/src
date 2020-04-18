/* Functions to compute SHA1 message digest of files or memory blocks.
   according to the definition of SHA1 in FIPS 180-1 from April 1997.
   Copyright (C) 2008-2011 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2008.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "sha1.h"
#include "system.h"

#define SWAP(n) BE32 (n)

/* This array contains the bytes used to pad the buffer to the next
   64-byte boundary.  */
static const unsigned char fillbuf[64] = { 0x80, 0 /* , 0, 0, ...  */ };


/* Initialize structure containing state of computation.  */
void
sha1_init_ctx (ctx)
     struct sha1_ctx *ctx;
{
  ctx->A = 0x67452301;
  ctx->B = 0xefcdab89;
  ctx->C = 0x98badcfe;
  ctx->D = 0x10325476;
  ctx->E = 0xc3d2e1f0;

  ctx->total[0] = ctx->total[1] = 0;
  ctx->buflen = 0;
}

/* Put result from CTX in first 20 bytes following RESBUF.  The result
   must be in little endian byte order.

   IMPORTANT: On some systems it is required that RESBUF is correctly
   aligned for a 32 bits value.  */
void *
sha1_read_ctx (ctx, resbuf)
     const struct sha1_ctx *ctx;
     void *resbuf;
{
  ((sha1_uint32 *) resbuf)[0] = SWAP (ctx->A);
  ((sha1_uint32 *) resbuf)[1] = SWAP (ctx->B);
  ((sha1_uint32 *) resbuf)[2] = SWAP (ctx->C);
  ((sha1_uint32 *) resbuf)[3] = SWAP (ctx->D);
  ((sha1_uint32 *) resbuf)[4] = SWAP (ctx->E);

  return resbuf;
}

static void
be64_copy (char *dest, uint64_t x)
{
  for (size_t i = 8; i-- > 0; x >>= 8)
    dest[i] = (uint8_t) x;
}

/* Process the remaining bytes in the internal buffer and the usual
   prolog according to the standard and write the result to RESBUF.

   IMPORTANT: On some systems it is required that RESBUF is correctly
   aligned for a 32 bits value.  */
void *
sha1_finish_ctx (ctx, resbuf)
     struct sha1_ctx *ctx;
     void *resbuf;
{
  /* Take yet unprocessed bytes into account.  */
  sha1_uint32 bytes = ctx->buflen;
  size_t pad;

  /* Now count remaining bytes.  */
  ctx->total[0] += bytes;
  if (ctx->total[0] < bytes)
    ++ctx->total[1];

  pad = bytes >= 56 ? 64 + 56 - bytes : 56 - bytes;
  memcpy (&ctx->buffer[bytes], fillbuf, pad);

  /* Put the 64-bit file length in *bits* at the end of the buffer.  */
  const uint64_t bit_length = ((ctx->total[0] << 3)
			       + ((uint64_t) ((ctx->total[1] << 3) |
					      (ctx->total[0] >> 29)) << 32));
  be64_copy (&ctx->buffer[bytes + pad], bit_length);

  /* Process last bytes.  */
  sha1_process_block (ctx->buffer, bytes + pad + 8, ctx);

  return sha1_read_ctx (ctx, resbuf);
}


void
sha1_process_bytes (buffer, len, ctx)
     const void *buffer;
     size_t len;
     struct sha1_ctx *ctx;
{
  /* When we already have some bits in our internal buffer concatenate
     both inputs first.  */
  if (ctx->buflen != 0)
    {
      size_t left_over = ctx->buflen;
      size_t add = 128 - left_over > len ? len : 128 - left_over;

      memcpy (&ctx->buffer[left_over], buffer, add);
      ctx->buflen += add;

      if (ctx->buflen > 64)
	{
	  sha1_process_block (ctx->buffer, ctx->buflen & ~63, ctx);

	  ctx->buflen &= 63;
	  /* The regions in the following copy operation cannot overlap.  */
	  memcpy (ctx->buffer, &ctx->buffer[(left_over + add) & ~63],
		  ctx->buflen);
	}

      buffer = (const char *) buffer + add;
      len -= add;
    }

  /* Process available complete blocks.  */
  if (len >= 64)
    {
#if !_STRING_ARCH_unaligned
/* To check alignment gcc has an appropriate operator.  Other
   compilers don't.  */
# if __GNUC__ >= 2
#  define UNALIGNED_P(p) (((sha1_uintptr) p) % __alignof__ (sha1_uint32) != 0)
# else
#  define UNALIGNED_P(p) (((sha1_uintptr) p) % sizeof (sha1_uint32) != 0)
# endif
      if (UNALIGNED_P (buffer))
	while (len > 64)
	  {
	    sha1_process_block (memcpy (ctx->buffer, buffer, 64), 64, ctx);
	    buffer = (const char *) buffer + 64;
	    len -= 64;
	  }
      else
#endif
	{
	  sha1_process_block (buffer, len & ~63, ctx);
	  buffer = (const char *) buffer + (len & ~63);
	  len &= 63;
	}
    }

  /* Move remaining bytes in internal buffer.  */
  if (len > 0)
    {
      size_t left_over = ctx->buflen;

      memcpy (&ctx->buffer[left_over], buffer, len);
      left_over += len;
      if (left_over >= 64)
	{
	  sha1_process_block (ctx->buffer, 64, ctx);
	  left_over -= 64;
	  memcpy (ctx->buffer, &ctx->buffer[64], left_over);
	}
      ctx->buflen = left_over;
    }
}


/* These are the four functions used in the four steps of the SHA1 algorithm
   and defined in the FIPS 180-1.  */
/* #define FF(b, c, d) ((b & c) | (~b & d)) */
#define FF(b, c, d) (d ^ (b & (c ^ d)))
#define FG(b, c, d) (b ^ c ^ d)
/* define FH(b, c, d) ((b & c) | (b & d) | (c & d)) */
#define FH(b, c, d) (((b | c) & d) | (b & c))

/* It is unfortunate that C does not provide an operator for cyclic
   rotation.  Hope the C compiler is smart enough.  */
#define CYCLIC(w, s) (((w) << s) | ((w) >> (32 - s)))

/* Magic constants.  */
#define K0 0x5a827999
#define K1 0x6ed9eba1
#define K2 0x8f1bbcdc
#define K3 0xca62c1d6


/* Process LEN bytes of BUFFER, accumulating context into CTX.
   It is assumed that LEN % 64 == 0.  */

void
sha1_process_block (buffer, len, ctx)
     const void *buffer;
     size_t len;
     struct sha1_ctx *ctx;
{
  sha1_uint32 computed_words[16];
#define W(i) computed_words[(i) % 16]
  const sha1_uint32 *words = buffer;
  size_t nwords = len / sizeof (sha1_uint32);
  const sha1_uint32 *endp = words + nwords;
  sha1_uint32 A = ctx->A;
  sha1_uint32 B = ctx->B;
  sha1_uint32 C = ctx->C;
  sha1_uint32 D = ctx->D;
  sha1_uint32 E = ctx->E;

  /* First increment the byte count.  FIPS 180-1 specifies the possible
     length of the file up to 2^64 bits.  Here we only compute the
     number of bytes.  Do a double word increment.  */
  ctx->total[0] += len;
  if (ctx->total[0] < len)
    ++ctx->total[1];

  /* Process all bytes in the buffer with 64 bytes in each round of
     the loop.  */
  while (words < endp)
    {
      sha1_uint32 A_save = A;
      sha1_uint32 B_save = B;
      sha1_uint32 C_save = C;
      sha1_uint32 D_save = D;
      sha1_uint32 E_save = E;

      /* First round: using the given function, the context and a constant
	 the next context is computed.  Because the algorithms processing
	 unit is a 32-bit word and it is determined to work on words in
	 little endian byte order we perhaps have to change the byte order
	 before the computation.  */

#define OP(i, a, b, c, d, e)						\
      do								\
        {								\
	  W (i) = SWAP (*words);					\
	  e = CYCLIC (a, 5) + FF (b, c, d) + e + W (i) + K0;		\
	  ++words;							\
	  b = CYCLIC (b, 30);						\
        }								\
      while (0)

      /* Steps 0 to 15.  */
      OP (0, A, B, C, D, E);
      OP (1, E, A, B, C, D);
      OP (2, D, E, A, B, C);
      OP (3, C, D, E, A, B);
      OP (4, B, C, D, E, A);
      OP (5, A, B, C, D, E);
      OP (6, E, A, B, C, D);
      OP (7, D, E, A, B, C);
      OP (8, C, D, E, A, B);
      OP (9, B, C, D, E, A);
      OP (10, A, B, C, D, E);
      OP (11, E, A, B, C, D);
      OP (12, D, E, A, B, C);
      OP (13, C, D, E, A, B);
      OP (14, B, C, D, E, A);
      OP (15, A, B, C, D, E);

      /* For the remaining 64 steps we have a more complicated
	 computation of the input data-derived values.  Redefine the
	 macro to take an additional second argument specifying the
	 function to use and a new last parameter for the magic
	 constant.  */
#undef OP
#define OP(i, f, a, b, c, d, e, K) \
      do								\
        {								\
	  W (i) = CYCLIC (W (i - 3) ^ W (i - 8) ^ W (i - 14) ^ W (i - 16), 1);\
	  e = CYCLIC (a, 5) + f (b, c, d) + e + W (i) + K;		\
	  b = CYCLIC (b, 30);						\
        }								\
      while (0)

      /* Steps 16 to 19.  */
      OP (16, FF, E, A, B, C, D, K0);
      OP (17, FF, D, E, A, B, C, K0);
      OP (18, FF, C, D, E, A, B, K0);
      OP (19, FF, B, C, D, E, A, K0);

      /* Steps 20 to 39.  */
      OP (20, FG, A, B, C, D, E, K1);
      OP (21, FG, E, A, B, C, D, K1);
      OP (22, FG, D, E, A, B, C, K1);
      OP (23, FG, C, D, E, A, B, K1);
      OP (24, FG, B, C, D, E, A, K1);
      OP (25, FG, A, B, C, D, E, K1);
      OP (26, FG, E, A, B, C, D, K1);
      OP (27, FG, D, E, A, B, C, K1);
      OP (28, FG, C, D, E, A, B, K1);
      OP (29, FG, B, C, D, E, A, K1);
      OP (30, FG, A, B, C, D, E, K1);
      OP (31, FG, E, A, B, C, D, K1);
      OP (32, FG, D, E, A, B, C, K1);
      OP (33, FG, C, D, E, A, B, K1);
      OP (34, FG, B, C, D, E, A, K1);
      OP (35, FG, A, B, C, D, E, K1);
      OP (36, FG, E, A, B, C, D, K1);
      OP (37, FG, D, E, A, B, C, K1);
      OP (38, FG, C, D, E, A, B, K1);
      OP (39, FG, B, C, D, E, A, K1);

      /* Steps 40 to 59.  */
      OP (40, FH, A, B, C, D, E, K2);
      OP (41, FH, E, A, B, C, D, K2);
      OP (42, FH, D, E, A, B, C, K2);
      OP (43, FH, C, D, E, A, B, K2);
      OP (44, FH, B, C, D, E, A, K2);
      OP (45, FH, A, B, C, D, E, K2);
      OP (46, FH, E, A, B, C, D, K2);
      OP (47, FH, D, E, A, B, C, K2);
      OP (48, FH, C, D, E, A, B, K2);
      OP (49, FH, B, C, D, E, A, K2);
      OP (50, FH, A, B, C, D, E, K2);
      OP (51, FH, E, A, B, C, D, K2);
      OP (52, FH, D, E, A, B, C, K2);
      OP (53, FH, C, D, E, A, B, K2);
      OP (54, FH, B, C, D, E, A, K2);
      OP (55, FH, A, B, C, D, E, K2);
      OP (56, FH, E, A, B, C, D, K2);
      OP (57, FH, D, E, A, B, C, K2);
      OP (58, FH, C, D, E, A, B, K2);
      OP (59, FH, B, C, D, E, A, K2);

      /* Steps 60 to 79.  */
      OP (60, FG, A, B, C, D, E, K3);
      OP (61, FG, E, A, B, C, D, K3);
      OP (62, FG, D, E, A, B, C, K3);
      OP (63, FG, C, D, E, A, B, K3);
      OP (64, FG, B, C, D, E, A, K3);
      OP (65, FG, A, B, C, D, E, K3);
      OP (66, FG, E, A, B, C, D, K3);
      OP (67, FG, D, E, A, B, C, K3);
      OP (68, FG, C, D, E, A, B, K3);
      OP (69, FG, B, C, D, E, A, K3);
      OP (70, FG, A, B, C, D, E, K3);
      OP (71, FG, E, A, B, C, D, K3);
      OP (72, FG, D, E, A, B, C, K3);
      OP (73, FG, C, D, E, A, B, K3);
      OP (74, FG, B, C, D, E, A, K3);
      OP (75, FG, A, B, C, D, E, K3);
      OP (76, FG, E, A, B, C, D, K3);
      OP (77, FG, D, E, A, B, C, K3);
      OP (78, FG, C, D, E, A, B, K3);
      OP (79, FG, B, C, D, E, A, K3);

      /* Add the starting values of the context.  */
      A += A_save;
      B += B_save;
      C += C_save;
      D += D_save;
      E += E_save;
    }

  /* Put checksum in context given as argument.  */
  ctx->A = A;
  ctx->B = B;
  ctx->C = C;
  ctx->D = D;
  ctx->E = E;
}
