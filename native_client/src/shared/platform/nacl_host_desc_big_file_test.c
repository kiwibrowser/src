/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/include/portability_io.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/platform_init.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"

/*
 * This program runs a bunch of tests.  Each test gets a large file --
 * greater than 4GB in size -- as input.  Currently, we construct
 * these input files identically content-wise, but with opened with
 * possibly different permissions.  The contents of the input files
 * are lines of text from the |quotes| array below, written in a
 * sparse fashion -- some lines at the beginning of the file, some at
 * the 2GB boundary, and some at the 4G boundary.
 *
 * The individual tests do different things.  The most basic one just
 * ensures that the data can be read back.  Others ensure that the
 * "holes" in the file (created by seeking) are properly filled with
 * ASCII NUL characters by reading the file region that contains the
 * boundary between the hole regions and the text regions.  Another
 * overwrites the file with data at large strides, and checks that the
 * data can be recovered.
 */

/*
 * Number of lines of quote text to be written at various file offsets.
 */
#define LINES_AT_ZERO   (8)
#define LINES_AT_2G     (8)
#define LINES_AT_4G     (~(size_t) 0)  /* infinity; to the end of array */

/*
 * An interesting quote for populating the (sparse) test data file.
 */
static char const *quotes[] = {
  "ALICE was beginning to get very tired of sitting by her sister on the\n",
  "bank, and of having nothing to do: once or twice she had peeped into\n",
  "the book her sister was reading, but it had no pictures or conversations\n",
  "in it, \"and what is the use of a book,\" thought Alice, \"without\n",
  "pictures or conversations?\" So she was considering, in her own mind\n",
  "(as well as she could, for the hot day made her feel very sleepy and\n",
  "stupid), whether the pleasure of making a daisy-chain would be worth the\n",
  "trouble of getting up and picking the daisies, when suddenly a White\n",
  "Rabbit with pink eyes ran close by her. There was nothing so very\n",
  "remarkable in that; nor did Alice think it so very much out of the way\n",
  "to hear the Rabbit say to itself \"Oh dear! Oh dear! I shall be too\n",
  "late!\" (when she thought it over afterwards, it occurred to her that\n",
  "she ought to have wondered at this, but at the time it all seemed quite\n",
  "natural); but, when the Rabbit actually took a watch out of its\n",
  "waistcoat-pocket, and looked at it, and then hurried on, Alice started\n",
  "to her feet, for it flashed across her mind that she had never before\n",
  "seen a rabbit with either a waistcoat-pocket, or a watch to take out of\n",
  "it, and burning with curiosity, she ran across the field after it, and\n",
  "was just in time to see it pop down a large rabbit-hole under the hedge.\n",
  "In another moment down went Alice after it, never once considering how\n",
  "in the world she was to get out again. The rabbit-hole went straight on\n",
  "like a tunnel for some way, and then dipped suddenly down, so suddenly\n",
  "that Alice had not a moment to think about stopping herself before she\n",
  "found herself falling down what seemed to be a very deep well. Either\n",
  "the well was very deep, or she fell very slowly, for she had plenty of\n",
  "time as she went down to look about her, and to wonder what was going to\n",
  "happen next. First, she tried to look down and make out what she was\n",
  "coming to, but it was too dark to see anything: then she looked at the\n",
  "sides of the well, and noticed that they were filled with cupboards and\n",
  "book-shelves: here and there she saw maps and pictures hung upon pegs.\n",
  "She took down a jar from one of the shelves as she passed: it was\n",
  "labeled \"ORANGE MARMALADE,\" but to her great disappointment it was\n",
  "empty: she did not like to drop the jar, for fear of killing somebody\n",
  "underneath, so managed to put it into one of the cupboards as she\n",
};

/*
 * Utilities for performing operations on ranges of lines, starting at
 * |start_ix| for |count| lines.  We use a visitor pattern, so that we
 * can pass a functor to write out ranges of lines, to compute the
 * number of bytes that a range of lines would occupy, etc.
 * OperateOnLineRange returns the sum of the returned values from
 * |fn|.  |count| can be SIZE_T_MAX (i.e., ~(size_t) 0) to mean to
 * process from |start_ix| to the end of the quote array.
 */
static size_t OperateOnLineRange(size_t (*fn)(void *params, size_t ix),
                                 void *params,
                                 size_t start_ix,
                                 size_t count) {
  size_t ix;
  size_t limit_ix;
  size_t rv = 0;

  /* check for integer overflows */
  if ((~(size_t) 0) - count < start_ix) {
    limit_ix = NACL_ARRAY_SIZE(quotes);
  } else if (start_ix + count > NACL_ARRAY_SIZE(quotes)) {
    limit_ix = NACL_ARRAY_SIZE(quotes);
  } else {
    limit_ix = start_ix + count;
  }
  for (ix = start_ix; ix < limit_ix; ++ix) {
    rv += (*fn)(params, ix);
  }
  return rv;
}

/* visitor for line |ix| */
static size_t WriteLineRangeOp(void *params, size_t ix) {
  struct NaClHostDesc *d = (struct NaClHostDesc *) params;
  size_t len = strlen(quotes[ix]);
  ssize_t result;
  result = NaClHostDescWrite(d, quotes[ix], len);
  if (result < 0) {
    fprintf(stderr,
            "WriteLineRange: writing line %"NACL_PRIuS" failed, errno %d\n",
            ix, -(int) result);
    fprintf(stderr, " line contents: %.*s\n", (int) len, quotes[ix]);
    exit(1);
  }
  if ((size_t) result != len) {
    fprintf(stderr,
            "WriteLineRange: short write:\n"
            "  Expected: %"NACL_PRIuS" bytes,\n"
            "    actual: %"NACL_PRIuS" bytes.\n",
            len, (size_t) result);
    exit(1);
  }
  return 0;
}

static void WriteLineRange(struct NaClHostDesc *d,
                           size_t start_ix,
                           size_t count) {
  (void) OperateOnLineRange(WriteLineRangeOp, (void *) d, start_ix, count);
}

/* bool */
static int CheckedRead(struct NaClHostDesc *d,
                       void *buffer,
                       size_t num_bytes) {
  ssize_t result;

  result = NaClHostDescRead(d, buffer, num_bytes);
  if (result < 0) {
    fprintf(stderr, "CheckedRead: reading failed, errno %d\n", -(int) result);
    return 0;
  }
  if ((size_t) result != num_bytes) {
    fprintf(stderr,
            "CheckedRead: got short read; expected %"NACL_PRIuS
            ", got %"NACL_PRIuS"\n",
            num_bytes, result);
    return 0;
  }
  return 1;
}

/* visitor for line |ix| */
static size_t ReadAndCheckLineRangeOp(void *params, size_t ix) {
  struct NaClHostDesc *d = (struct NaClHostDesc *) params;
  char buffer[4096];
  size_t len = strlen(quotes[ix]);

  CHECK(len < NACL_ARRAY_SIZE(buffer));

  memset(buffer, 0, sizeof buffer);
  if (!CheckedRead(d, buffer, len)) {
    fprintf(stderr,
            "ReadAndCheckLineRange: reading line %"NACL_PRIuS" failed\n",
            ix);
    return 1;
  }
  if (0 != memcmp(buffer, quotes[ix], len)) {
    fprintf(stderr,
            "ReadAndCheckLineRange: content differs\n"
            "Expected: %.*s\n"
            "     Got: %.*s\n",
            (int) len, quotes[ix],
            (int) len, buffer);
    return 1;
  }
  return 0;
}

/*
 * Read at the current file position for |d|, expecting to see lines
 * [start_ix, start_ix + count).  Verify against source data.
 */
static size_t ReadAndCheckLineRange(struct NaClHostDesc *d,
                                    size_t start_ix,
                                    size_t count) {
  return OperateOnLineRange(ReadAndCheckLineRangeOp, (void *) d,
                            start_ix, count);
}

/* visitor for computing number of bytes needed to hold line |ix| */
static size_t BytesNeededForLineRangeOp(void *params, size_t ix) {
  UNREFERENCED_PARAMETER(params);
  return strlen(quotes[ix]);
}

static size_t BytesNeededForLineRange(size_t start_ix, size_t count) {
  return OperateOnLineRange(BytesNeededForLineRangeOp, (void *) NULL,
                            start_ix, count);
}

/*
 * Test utilities used by several tests.  In particular, see struct
 * PReadWriteInterface below.
 */
static nacl_off64_t CheckedSeek(struct NaClHostDesc *d,
                                nacl_off64_t offset,
                                int whence) {
  nacl_off64_t result;

  printf("CheckedSeek offset %"NACL_PRIx64"\n", offset);
  result = NaClHostDescSeek(d, offset, whence);
  if (result < 0) {
    fprintf(stderr, "CheckedSeek to offset 0x%"NACL_PRIx64
            ", whence %d failed, error %d\n",
            offset, whence, -(int) result);
    exit(1);
  }

  return result;
}

static ssize_t SimulatedPRead(struct NaClHostDesc *d,
                              void *buffer,
                              size_t num_bytes,
                              nacl_off64_t offset) {
  ssize_t rv;
  nacl_off64_t orig_offset;

  orig_offset = CheckedSeek(d, 0, SEEK_CUR);
  CheckedSeek(d, offset, 0);
  rv = NaClHostDescRead(d, buffer, num_bytes);
  CheckedSeek(d, orig_offset, 0);

  return rv;
}

static ssize_t SimulatedPWrite(struct NaClHostDesc *d,
                               void const *buffer,
                               size_t num_bytes,
                               nacl_off64_t offset) {
  ssize_t rv;
  nacl_off64_t orig_offset;

#if NACL_LINUX || NACL_OSX
  int is_append = d->flags & NACL_ABI_O_APPEND;
  int orig_flags;

  if (is_append) {
    orig_flags = fcntl(d->d, F_GETFL, 0);
    if (-1 == orig_flags) {
      fprintf(stderr, "pwrite POSIX O_APPEND hack (get) failed\n");
      exit(1);
    }
    if (-1 == fcntl(d->d, F_SETFL, orig_flags & ~O_APPEND)) {
      fprintf(stderr, "pwrite POSIX O_APPEND hack failed\n");
      exit(1);
    }
  }
#endif

  orig_offset = CheckedSeek(d, 0, SEEK_CUR);
  CheckedSeek(d, offset, 0);
  rv = NaClHostDescWrite(d, buffer, num_bytes);
  CheckedSeek(d, orig_offset, 0);

#if NACL_LINUX || NACL_OSX
  if (is_append) {
    if (-1 == fcntl(d->d, F_SETFL, orig_flags)) {
      fprintf(stderr, "pwrite POSIX O_APPEND hack (restore) failed\n");
      exit(1);
    }
  }
#endif

  return rv;
}

static int CheckedPReadFn(ssize_t (*fn)(struct NaClHostDesc *d,
                                        void *buffer,
                                        size_t num_bytes,
                                        nacl_off64_t offset),
                           struct NaClHostDesc *d,
                           void *buffer,
                           size_t num_bytes,
                           nacl_off64_t offset) {
  ssize_t result;
  result = (*fn)(d, buffer, num_bytes, offset);
  if (result < 0) {
    fprintf(stderr, "CheckedPReadFn: reading failed, errno %d\n",
            -(int) result);
    return 0;
  }
  if ((size_t) result != num_bytes) {
    fprintf(stderr,
            "CheckedReadFn: got short read; expected %"NACL_PRIuS
            ", got %"NACL_PRIuS"\n",
            num_bytes, result);
    return 0;
  }
  return 1;
}

static int CheckedPWriteFn(ssize_t (*fn)(struct NaClHostDesc *d,
                                         void const *buffer,
                                         size_t num_bytes,
                                         nacl_off64_t offset),
                           struct NaClHostDesc *d,
                           void const *buffer,
                           size_t num_bytes,
                           nacl_off64_t offset) {
  ssize_t result;
  result = (*fn)(d, buffer, num_bytes, offset);
  if (result < 0) {
    fprintf(stderr, "CheckedPWriteFn: reading failed, errno %d\n",
            -(int) result);
    return 0;
  }
  if ((size_t) result != num_bytes) {
    fprintf(stderr,
            "CheckedPWriteFn: got short read; expected %"NACL_PRIuS
            ", got %"NACL_PRIuS"\n",
            num_bytes, result);
    return 0;
  }
  return 1;
}

static size_t BasicReadWriteTest(struct NaClHostDesc *d, void *test_specifics) {
  size_t error_count = 0;

  UNREFERENCED_PARAMETER(test_specifics);

  CheckedSeek(d, ((nacl_off64_t) 0), 0);
  error_count += ReadAndCheckLineRange(d, 0, LINES_AT_ZERO);
  CheckedSeek(d, ((nacl_off64_t) 2) << 30, 0);
  error_count += ReadAndCheckLineRange(d, LINES_AT_ZERO, LINES_AT_2G);
  CheckedSeek(d, ((nacl_off64_t) 4) << 30, 0);
  error_count += ReadAndCheckLineRange(d, LINES_AT_ZERO + LINES_AT_2G,
                                       LINES_AT_4G);
  return error_count;
}

/* bool */
static int CheckNull(char const *buffer, size_t nbytes, nacl_off64_t offset) {
  size_t ix;

  for (ix = 0; ix < nbytes; ++ix) {
    if (buffer[ix] != '\0') {
      fprintf(stderr,
              "CheckHole: byte 0x%"NACL_PRIx64" not NUL\n",
              (uint64_t) (offset + ix));
      return 0;
    }
  }
  return 1;
}

/*
 * We parameterize a pread/pwrite test that can use a simulated
 * pread/pwrite (simply seeking and reading or writing, since this
 * test is single threaded) or the real pread/pwrite implementation to
 * read/modify large files.  Since we used seek to create the file and
 * have tested various properties, we trust that seek works, and thus
 * we can check the pread/pwrite test itself for consistency.
 */
struct PReadWriteInterface {
  ssize_t (*PRead)(struct NaClHostDesc *d,
                   void *buffer,
                   size_t num_bytes,
                   nacl_off64_t offset);
  ssize_t (*PWrite)(struct NaClHostDesc *d,
                    void const *buffer,
                    size_t num_bytes,
                    nacl_off64_t offset);
};

/*
 * BasicPReadTest: pread at offsets that span various hole-data boundaries
 * and verify that ASCII NUL bytes are in the hole region and the
 * expected data where they should be in the data region.  Essentially
 * the same as CheckHoles.
 */
size_t BasicPReadTest(struct NaClHostDesc *d, void *test_specifics) {
  struct PReadWriteInterface *interface =
      (struct PReadWriteInterface *) test_specifics;
  nacl_off64_t offset_last_line;
  char buffer[4096];
  size_t len;
  static size_t const kNullBytesInHole = 17;
  size_t available_bytes;

  UNREFERENCED_PARAMETER(test_specifics);

  offset_last_line = BytesNeededForLineRange(0, LINES_AT_ZERO - 1);
  memset(buffer, 0, sizeof buffer);
  if (!CheckedPReadFn(interface->PRead, d, buffer, sizeof buffer,
                      offset_last_line)) {
    fprintf(stderr,
            "BasicPReadTest: pread of first data-then-hole boundary failed\n");
    return 1;
  }
  len = strlen(quotes[LINES_AT_ZERO - 1]);
  if (0 != memcmp(buffer, quotes[LINES_AT_ZERO - 1], len)) {
    fprintf(stderr,
            "BasicPReadTest: last line data error\n"
            "Expected: %.*s\n"
            "     Got: %.*s\n",
            (int) len, quotes[LINES_AT_ZERO - 1],
            (int) len, buffer);
    return 1;
  }
  if (!CheckNull(buffer + len, NACL_ARRAY_SIZE(buffer) - len,
                 offset_last_line + len)) {
    return 1;
  }

  memset(buffer, 0, sizeof buffer);
  if (!CheckedPReadFn(interface->PRead, d, buffer, sizeof buffer,
                      (((nacl_off64_t) 2) << 30) - kNullBytesInHole)) {
    fprintf(stderr,
            "BasicPReadTest: read of first hole-then-data boundary failed\n");
    return 1;
  }
  if (!CheckNull(buffer, kNullBytesInHole,
                 (((nacl_off64_t) 2) << 30) - kNullBytesInHole)) {
    fprintf(stderr,
            "BasicPReadTest: expected NUL at first hole-then-data"
            " boundary absent\n");
    return 1;
  }
  len = strlen(quotes[LINES_AT_ZERO]);
  if (0 != memcmp(buffer + kNullBytesInHole, quotes[LINES_AT_ZERO], len)) {
    fprintf(stderr,
            "BasicPReadTest: quote at 2nd data region (after 1st hole)"
            " wrong.\n"
            "Expected: %.*s\n"
            "     got: %.*s\n",
            (int) len, quotes[LINES_AT_ZERO],
            (int) len, buffer + kNullBytesInHole);
    return 1;
  }

  /* check boundaries of 2nd hole */
  offset_last_line = (((nacl_off64_t) 2) << 30) +
      BytesNeededForLineRange(LINES_AT_ZERO,
                              LINES_AT_2G - 1);
  memset(buffer, 0, sizeof buffer);
  if (!CheckedPReadFn(interface->PRead, d, buffer, sizeof buffer,
                      offset_last_line)) {
    fprintf(stderr,
            "BasicPReadTest: read of 2nd data-then-hole boundary failed\n");
    return 1;
  }
  len = strlen(quotes[LINES_AT_ZERO + LINES_AT_2G - 1]);
  if (0 != memcmp(buffer, quotes[LINES_AT_ZERO + LINES_AT_2G - 1], len)) {
    fprintf(stderr,
            "BasicPReadTest: last line of 2nd data region error\n"
            "Expected: %.*s\n"
            "     got: %.*s\n",
            (int) len, quotes[LINES_AT_ZERO + LINES_AT_2G - 1],
            (int) len, buffer);
    return 1;
  }

  if (!CheckNull(buffer + len, NACL_ARRAY_SIZE(buffer) - len,
                 offset_last_line + len)) {
    return 1;
  }
  available_bytes = BytesNeededForLineRange(LINES_AT_ZERO + LINES_AT_2G,
                                            LINES_AT_4G);
  if (available_bytes > sizeof buffer) {
    available_bytes = sizeof buffer;
  }
  memset(buffer, 0, sizeof buffer);
  if (!CheckedPReadFn(interface->PRead, d, buffer, available_bytes,
                      (((nacl_off64_t) 4) << 30) - kNullBytesInHole)) {
    fprintf(stderr,
            "BasicPReadTest: read of 2nd hole-then-data boundary failed\n");
    return 1;
  }
  if (!CheckNull(buffer, kNullBytesInHole,
                 (((nacl_off64_t) 4) << 30) - kNullBytesInHole)) {
    fprintf(stderr,
            "BasicPReadTest: expected NUL at 2nd hole-then-data"
            " boundary absent\n");
    return 1;
  }
  len = strlen(quotes[LINES_AT_ZERO + LINES_AT_2G]);
  if (0 != memcmp(buffer + kNullBytesInHole,
                  quotes[LINES_AT_ZERO + LINES_AT_2G],
                  len)) {
    fprintf(stderr,
            "BasicPReadTest: quote at 3rd data region (after 2nd hole)"
            " wrong.\n"
            "Expected: %.*s\n"
            "     got: %.*s\n",
            (int) len, quotes[LINES_AT_ZERO + LINES_AT_2G],
            (int) len, buffer + kNullBytesInHole);
    return 1;
  }

  return 0;
}

/*
 * BasicPWriteRead uses Pwrite to overwrite some of the bytes of the
 * sparse input file, possibly extending it, and re-reads it to check
 * that the data was written properly.  The stride is chosen to evenly
 * split the target file size.
 */
size_t BasicPWriteReadTest(struct NaClHostDesc *d, void *test_specifics) {
  struct PReadWriteInterface *interface =
      (struct PReadWriteInterface *) test_specifics;
  nacl_off64_t offset;
  nacl_off64_t const target_file_size = ((nacl_off64_t) 1) << 33;
  nacl_off64_t stride;
  size_t ix;
  size_t len;
  char buffer[4096];
  size_t err_count = 0;

  stride = target_file_size / NACL_ARRAY_SIZE(quotes);
  for (ix = 0, offset = 0;
       ix < NACL_ARRAY_SIZE(quotes);
       ++ix, offset += stride) {
    len = strlen(quotes[ix]);
    CheckedPWriteFn(interface->PWrite, d, quotes[ix], len, offset);
  }
  for (ix = 0, offset = 0;
       ix < NACL_ARRAY_SIZE(quotes);
       ++ix, offset += stride) {
    len = strlen(quotes[ix]);
    CheckedPReadFn(interface->PRead, d, buffer, len, offset);
    if (0 != memcmp(quotes[ix], buffer, len)) {
      fprintf(stderr,
              "BasicPWriteReadTest: quote line %"NACL_PRIuS" wrong\n"
              " Expected: %.*s\n"
              "      Got: %.*s\n",
              ix, (int) len, quotes[ix], (int) len, buffer);
      ++err_count;
    }
  }
  return err_count;
}

static struct PReadWriteInterface const g_SimulatedPReadWriteImpl = {
  SimulatedPRead, SimulatedPWrite,
};

static struct PReadWriteInterface const g_RealPReadSimulatedPWriteImpl = {
  NaClHostDescPRead, SimulatedPWrite,
};

static struct PReadWriteInterface const g_SimulatedPReadRealPWriteImpl = {
  SimulatedPRead, NaClHostDescPWrite,
};

static struct PReadWriteInterface const g_RealPReadWriteImpl = {
  NaClHostDescPRead, NaClHostDescPWrite,
};

/*
 * Tests are parameterized, so test behavior can be data-driven.  The
 * same test functions can be run multiple times with files that are
 * opened with different flags, with the test function's expectations
 * controlled by the |test_specifics| generic pointer.
 */
struct TestParams {
  /* The name of the test, for use to label test output */
  char const *test_name;

  /*
   * The test function is implemented by |test_func|.  It receives an
   * initialized test input file |d|, with test-specific
   * parameterization via |test_specifics|.
   */
  size_t (*test_func)(struct NaClHostDesc *d, void *test_specifics);
  int open_flags;

  /* The |test_specifics| to use with |test_func| for this test run */
  void *test_specifics;
};

static struct TestParams const tests[] = {
  {
    "Basic test: data: data at 0, 2<<30, and 4<<30 -- read & check",
    BasicReadWriteTest,
    NACL_ABI_O_RDONLY,
    (void *) NULL,
  }, {
    "Using simulated pread/pwrite: read boundaries and check",
    BasicPReadTest,
    NACL_ABI_O_RDONLY,
    (void *) &g_SimulatedPReadWriteImpl,
  }, {
    "Using simulated pread/pwrite: write too",
    BasicPWriteReadTest,
    NACL_ABI_O_RDWR,
    (void *) &g_SimulatedPReadWriteImpl,
  }, {
    "Using real pread, simulated pwrite: write too",
    BasicPWriteReadTest,
    NACL_ABI_O_RDWR,
    (void *) &g_RealPReadSimulatedPWriteImpl,
  }, {
    "Using simulated pread, real pwrite: write too",
    BasicPWriteReadTest,
    NACL_ABI_O_RDWR,
    (void *) &g_SimulatedPReadRealPWriteImpl,
  }, {
    "Using real pread/pwrite: write too",
    BasicPWriteReadTest,
    NACL_ABI_O_RDWR,
    (void *) &g_RealPReadWriteImpl,
  },
#if !NACL_WINDOWS
  /*
   * Windows doesn't have a way to drop O_APPEND, which the simulated pwrite
   * implementation relies on, so skip these tests.
   */
  {
    "Using simulated pread/pwrite: write too (O_APPEND)",
    BasicPWriteReadTest,
    NACL_ABI_O_RDWR | NACL_ABI_O_APPEND,
    (void *) &g_SimulatedPReadWriteImpl,
  }, {
    "Using real pread, simulated pwrite: write too (O_APPEND)",
    BasicPWriteReadTest,
    NACL_ABI_O_RDWR | NACL_ABI_O_APPEND,
    (void *) &g_RealPReadSimulatedPWriteImpl,
  },
#endif
  {
    "Using simulated pread, real pwrite: write too (O_APPEND)",
    BasicPWriteReadTest,
    NACL_ABI_O_RDWR | NACL_ABI_O_APPEND,
    (void *) &g_SimulatedPReadRealPWriteImpl,
  }, {
    "Using real pread/pwrite: write too (O_APPEND)",
    BasicPWriteReadTest,
    NACL_ABI_O_RDWR | NACL_ABI_O_APPEND,
    (void *) &g_RealPReadWriteImpl,
  },
};


/*
 * Functions for initializing the test input file(s).
 */
static void WriteDataSparsely(struct NaClHostDesc *d) {
  CHECK(LINES_AT_ZERO + LINES_AT_2G < NACL_ARRAY_SIZE(quotes));
  WriteLineRange(d, 0, LINES_AT_ZERO);
  CheckedSeek(d, ((nacl_off64_t) 2) << 30, 0);
  WriteLineRange(d, LINES_AT_ZERO, LINES_AT_2G);
  CheckedSeek(d, ((nacl_off64_t) 4) << 30, 0);
  WriteLineRange(d, LINES_AT_ZERO + LINES_AT_2G, LINES_AT_4G);
}

static void CreateTestFile(struct NaClHostDesc *d_out,
                           char const *pathname,
                           struct TestParams const *param) {
  struct NaClHostDesc temp;
  int err;

  int flags = NACL_ABI_O_CREAT | NACL_ABI_O_TRUNC | NACL_ABI_O_WRONLY;
  err = NaClHostDescOpen(&temp, pathname,
                         flags, 0640);
  if (0 != err) {
    fprintf(stderr, "CreateTestFile: NaClHostDescOpen failed"
            " for initial data\n");
    fprintf(stderr, "   pathname %s\n", pathname);
    fprintf(stderr, " open_flags 0x%x\n", flags);
    exit(1);
  }

  WriteDataSparsely(&temp);

  err = NaClHostDescClose(&temp);
  if (0 != err) {
    fprintf(stderr, "CreateTestFile: NaClHostDescClose failed\n");
    exit(1);
  }
  err = NaClHostDescOpen(d_out, pathname,
                         param->open_flags, 0640);
  if (0 != err) {
    fprintf(stderr, "CreateTestFile: NaClHostDescOpen failed"
            " for test access\n");
    fprintf(stderr, "   pathname %s\n", pathname);
    fprintf(stderr, " open_flags 0x%x\n", param->open_flags);
    exit(1);
  }
}

/*
 * It is the responsibility of the invoking environment to ensure that
 * files created in the test directory passed as command-line argument
 * are deleted.  While this test tries to clean up the generated input
 * files, on CHECK failures or other aborts, a temporary file may be
 * left behind.
 *
 * See the build.scons file.
 */
int main(int ac, char **av) {
  /*
   * On try-/build-bots, this should be in the scons-out directory
   * since that will be on a disk that is provisioned with enough disk
   * space.
   */
  char const *test_dir_name = "/tmp/nacl_bit_file_test";
  struct NaClHostDesc hd;
  size_t err_count;
  size_t ix;
  int opt;
  int num_runs = 1;
  int test_run;

  while (EOF != (opt = getopt(ac, av, "c:t:"))) {
    switch (opt) {
      case 'c':
        num_runs = atoi(optarg);
        break;
      case 't':
        test_dir_name = optarg;
        break;
      default:
        fprintf(stderr,
                "Usage: nacl_big_file_test [-c run_count]\n"
                "                          [-t test_temp_dir]\n");
        exit(1);
    }
  }

  NaClPlatformInit();

  err_count = 0;
  for (test_run = 0; test_run < num_runs; ++test_run) {
    printf("Test run %d\n\n", test_run);
    for (ix = 0; ix < NACL_ARRAY_SIZE(tests); ++ix) {
      char test_file_name[PATH_MAX];
      SNPRINTF(test_file_name, sizeof test_file_name,
               "%s/f%d.%"NACL_PRIuS, test_dir_name, test_run, ix);
      printf("%s\n", tests[ix].test_name);
      CreateTestFile(&hd, test_file_name, &tests[ix]);
      err_count += (*tests[ix].test_func)(&hd, tests[ix].test_specifics);
      CHECK(0 == NaClHostDescClose(&hd));
      if (
#if NACL_LINUX || NACL_OSX
          0 != unlink(test_file_name)
#elif NACL_WINDOWS
          !DeleteFileA(test_file_name)
#endif
          ) {
        fprintf(stderr, "Could not delete %s\n", test_file_name);
        ++err_count;
      }
    }
  }

  NaClPlatformFini();

  /*
   * We ignore the 2^32 or 2^64 total errors case.  By exiting with
   * the error count, even if the test infrastructure drops the
   * output, we should get an idea of the number of failures.
   */
  return (err_count > 255) ? 255 : err_count;
}
