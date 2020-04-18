/*
 * Implementation of C library functions that may be required by lowering
 * intrinsics that are part of the PNaCl stable bitcode ABI.
 *
 * This code is based on newlib/libc. The newlib license file can be found
 * in COPYING.NEWLIB in the same directory with this file.
 */

/*
 * PNaCl is ILP32.
 */
typedef unsigned int size_t;

/*
 * The mem{cpy,move,set} functions implemented here are optimized by copying
 * (or setting) a word at a time, and unrolling the loops 4x when the copied
 * block is large enough (UNROLLBLOCKSIZE).
 * When it's not large enough, or when the pointers are not aligned, a simple
 * byte-copy loop is used.
 */
static const size_t WORDSIZE = sizeof(long);
static const size_t UNROLLBLOCKSIZE = WORDSIZE * 4;

/* Return non-zero if ptr is aligned to WORDSIZE, zero otherwise */
static inline int ISALIGNED(char *ptr) {
  return ((long) ptr & (WORDSIZE - 1)) == 0;
}

/*
 * The memset() function fills the first n bytes of the memory area pointed to
 * by s_void with the constant byte c.
 */
void *memset(void *s_void, int c, size_t n) {
  char *s = (char *) s_void;

  /* Reach an aligned address */
  while (!ISALIGNED(s)) {
    if (n--)
      *s++ = (char) c;
    else
      return s_void;
  }

  if (n >= UNROLLBLOCKSIZE) {
    /* Use initbuf - a word full of c byte values, to do word-wise init */
    unsigned long d = c & 0xff;
    unsigned long initbuf = (d << 24) | (d << 16) | (d << 8) | d;

    /* Here we can assume a word-aligned pointer. Unroll the loop 4x. */
    unsigned long *aligned_s = (unsigned long *) s;
    while (n >= WORDSIZE * 4) {
      *aligned_s++ = initbuf;
      *aligned_s++ = initbuf;
      *aligned_s++ = initbuf;
      *aligned_s++ = initbuf;
      n -= 4 * WORDSIZE;
    }

    /* Collect the leftovers of unrolling. */
    while (n >= WORDSIZE) {
      *aligned_s++ = initbuf;
      n -= WORDSIZE;
    }

    /* The sub-word remainder will be handled by the byte-loop. */
    s = (char *) aligned_s;
  }

  while (n--)
    *s++ = (char) c;

  return s_void;
}

/*
 * The memcpy() function copies n bytes from memory area src_void to memory
 * area dst_void. The memory areas must not overlap.
 */
void *memcpy(void *dst_void, const void *src_void, size_t n) {
  char *dst = dst_void;
  char *src = (char *) src_void;

  if (n >= UNROLLBLOCKSIZE && ISALIGNED(src) && ISALIGNED(dst)) {
    long *aligned_dst = (long *) dst;
    long *aligned_src = (long *) src;

    /* Unroll the word-copy loop 4x. */
    while (n >= UNROLLBLOCKSIZE) {
      *aligned_dst++ = *aligned_src++;
      *aligned_dst++ = *aligned_src++;
      *aligned_dst++ = *aligned_src++;
      *aligned_dst++ = *aligned_src++;
      n -= UNROLLBLOCKSIZE;
    }

    /* Collect the leftovers of unrolling. */
    while (n >= WORDSIZE) {
      *aligned_dst++ = *aligned_src++;
      n -= WORDSIZE;
    }

    /* The sub-word remainder will be handled by the byte-loop. */
    dst = (char *) aligned_dst;
    src = (char *) aligned_src;
  }

  while (n--) {
    *dst++ = *src++;
  }

  return dst_void;
}

/*
 * The memmove() function copies n bytes from memory area src_void to memory
 * area dst_void. The memory areas may overlap.
 */
void *memmove(void *dst_void, const void *src_void, size_t n) {
  char *dst = dst_void;
  char *src = (char *) src_void;

  if (src < dst && dst < src + n) {
    /*
     * Overlap with dst after src: copy safely by starting from the end. This
     * way a byte is copied over before being overwritten by a later copy.
     */
    src += n;
    dst += n;
    while (n--) {
      *--dst = *--src;
    }
  } else {
    /* Safe to copy using memcpy */
    return memcpy(dst_void, src_void, n);
  }

  return dst_void;
}
