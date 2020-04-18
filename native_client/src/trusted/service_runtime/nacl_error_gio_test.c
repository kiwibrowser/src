/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/nacl_base/nacl_refcount.h"
#include "native_client/src/trusted/service_runtime/nacl_error_gio.h"

#define MAX_WRITE_CHUNK_SIZE  32

static int g_verbosity = 0;
static size_t g_output_bytes_min = 32;
static size_t g_output_bytes_max = 2048;

struct TailGio {
  struct GioVtbl const *vtbl;
  char buffer[NACL_ERROR_GIO_MAX_BYTES];
  size_t num_bytes;
  size_t total_output_bytes;
};

struct GioVtbl const kTailGioVtbl;

static int TailGioCtor(struct TailGio *self) {
  self->vtbl = &kTailGioVtbl;
  self->num_bytes = 0;
  self->total_output_bytes = 0;
  return 1;
}

static void TailGioDtor(struct Gio *vself) {
  UNREFERENCED_PARAMETER(vself);
  return;
}

static ssize_t TailGioRead(struct Gio *vself, void *buf, size_t count) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(buf);
  UNREFERENCED_PARAMETER(count);
  return 0;
}

static ssize_t TailGioWrite(struct Gio *vself, void const *buf, size_t count) {
  struct TailGio *self = (struct TailGio *) vself;

  if (count > NACL_ARRAY_SIZE(self->buffer)) {
    memcpy(self->buffer, (char *) buf + count - NACL_ARRAY_SIZE(self->buffer),
           NACL_ARRAY_SIZE(self->buffer));
    self->num_bytes = NACL_ARRAY_SIZE(self->buffer);
  } else {
    /* 0 <= count <= NACL_ARRAY_SIZE(self->buffer) */
    size_t max_from_buffer = NACL_ARRAY_SIZE(self->buffer) - count;

    /* count + max_from_buffer == NACL_ARRAY_SIZE(self->buffer) */
    /* 0 <= max_from_buffer <= NACL_ARRAY_SIZE(self->buffer) */
    if (max_from_buffer < self->num_bytes) {
      memmove(self->buffer,
              self->buffer + self->num_bytes - max_from_buffer,
              max_from_buffer);
      memcpy(self->buffer + max_from_buffer,
             buf, count);
      self->num_bytes = NACL_ARRAY_SIZE(self->buffer);
    } else {
      /*
       * might still be a partial buffer:
       *
       * self->num_bytes <= max_from_buffer
       * count == NACL_ARRAY_SIZE(self->buffer) - max_from_buffer
       * therefore self->num_bytes + count <= NACL_ARRAY_SIZE(self->buffer)
       */
      memcpy(self->buffer + self->num_bytes,
             buf, count);
      self->num_bytes += count;
    }
  }
  self->total_output_bytes += count;
  return count;
}

static off_t TailGioSeek(struct Gio *vself,
                  off_t offset,
                  int whence) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(offset);
  UNREFERENCED_PARAMETER(whence);
  return 0;
}

static int TailGioFlush(struct Gio *vself) {
  UNREFERENCED_PARAMETER(vself);
  return 0;
}

static int TailGioClose(struct Gio *vself) {
  UNREFERENCED_PARAMETER(vself);
  return 0;
}

struct GioVtbl const kTailGioVtbl = {
  TailGioDtor,
  TailGioRead,
  TailGioWrite,
  TailGioSeek,
  TailGioFlush,
  TailGioClose,
};

struct NaClErrorGio *TestGioFactory(void) {
  struct TailGio *tgio = NULL;
  struct NaClErrorGio *negio = NULL;

  if (NULL == (tgio = (struct TailGio *) malloc(sizeof *tgio))) {
    fprintf(stderr, "TestGioFactory: Out of memory for tgio\n");
    goto giveup;
  }
  if (NULL == (negio = (struct NaClErrorGio *) malloc(sizeof *negio))) {
    fprintf(stderr, "TestGioFactory: Out of memory for negio\n");
    goto giveup;
  }
  if (!TailGioCtor(tgio)) {
    fprintf(stderr, "TestGioFactory: TailGioCtor failed\n");
    goto giveup;
  }
  if (!NaClErrorGioCtor(negio, (struct Gio *) tgio)) {
    fprintf(stderr, "TestGioFactory: NaClErrorGioCtor failed\n");
    goto giveup_dtor_tgio;
  }
  return negio;

 giveup_dtor_tgio:
  (*NACL_VTBL(Gio, tgio)->Dtor)((struct Gio *) tgio);
 giveup:
  free(negio);
  free(tgio);
  return NULL;
}

void TestGioRecycler(struct NaClErrorGio *trash) {
  struct TailGio *tgio;

  if (NULL == trash) {
    return;
  }
  tgio = (struct TailGio *) trash->pass_through;
  /* violate abstraction; this is test code */
  (*NACL_VTBL(Gio, trash)->Dtor)((struct Gio *) trash);
  free(trash);
  (*NACL_VTBL(Gio, tgio)->Dtor)((struct Gio *) tgio);
  free(tgio);
}

size_t UnitTest_BasicCtorDtor(void) {
  struct NaClErrorGio neg;
  size_t num_errors = 0;

  if (g_verbosity > 0) {
    printf("UnitTest_BasicCtorDtor\n");
  }
  if (!NaClErrorGioCtor(&neg, NULL)) {
    ++num_errors;
    fprintf(stderr, "UnitTest_BasicCtorDtor: could not construct\n");
    goto giveup;
  }
  (*NACL_VTBL(Gio, &neg)->Dtor)((struct Gio *) &neg);
 giveup:
  if (g_verbosity) {
    printf("UnitTest_BasicCtorDtor: %"NACL_PRIuS" error(s)\n", num_errors);
  }
  return num_errors;
}

size_t UnitTest_WeirdBuffers(void) {
  struct NaClErrorGio *neg;
  size_t num_errors = 0;
  size_t written;
  char const ktest_string[] = "123456789";
  size_t test_string_len = strlen(ktest_string);
  char small_buffer[1];
  char large_buffer[10 * NACL_ERROR_GIO_MAX_BYTES + 13];
  /* a much larger buffer, in case indexing calculations go awry */
  size_t actual;
  size_t ix;

  if (g_verbosity > 0) {
    printf("UnitTest_WeirdBuffers\n");
  }
  neg = TestGioFactory();
  if (NULL == neg) {
    ++num_errors;
    fprintf(stderr, "UnitTest_WeirdBuffers: could not construct\n");
    goto giveup;
  }
  if (0 != NaClErrorGioGetOutput(neg, (char *) NULL, 0)) {
    ++num_errors;
    fprintf(stderr,
            "UnitTest_WeirdBuffers: NULL buffer for NaClErrorGioGetOutput"
            " failed\n");
    goto giveup;
  }
  if (0 != NaClErrorGioGetOutput(neg,
                                 small_buffer,
                                 NACL_ARRAY_SIZE(small_buffer))) {
    ++num_errors;
    fprintf(stderr,
            "UnitTest_WeirdBuffers: small buffer with no output failed\n");
    goto giveup;
  }

  /* now make the buffer non-empty */

  written = (*NACL_VTBL(Gio, neg)->Write)((struct Gio *) neg,
                                          ktest_string, test_string_len);
  if (test_string_len != written) {
    ++num_errors;
    fprintf(stderr,
            "UnitTest_WeirdBuffers: writing %"NACL_PRIuS" bytes failed?!?\n",
            test_string_len);
    goto giveup;
  }
  small_buffer[0] = 'x';  /* different from '1' */
  actual = NaClErrorGioGetOutput(neg,
                                 small_buffer, NACL_ARRAY_SIZE(small_buffer));
  if (test_string_len != actual) {
    ++num_errors;
    fprintf(stderr,
            ("UnitTest_WeirdBuffers: too small buffer did not report actual"
             " required size, expected %"NACL_PRIuS", got %"NACL_PRIuS"\n"),
            test_string_len, actual);
    goto giveup;
  }
  if (small_buffer[0] != ktest_string[0]) {
    ++num_errors;
    fprintf(stderr,
            ("UnitTest_WeirdBuffers: partial buffer not filled, expected"
             " '1', got '%c'\n"),
            small_buffer[0]);
    goto giveup;
  }
  actual = NaClErrorGioGetOutput(neg,
                                 large_buffer, NACL_ARRAY_SIZE(large_buffer));
  if (test_string_len != actual) {
    ++num_errors;
    fprintf(stderr,
            ("UnitTest_WeirdBuffers: large buffer fill did not report actual"
             " output size, expected %"NACL_PRIuS", got %"NACL_PRIuS"\n"),
            test_string_len, actual);
    goto giveup;
  }
  for (ix = 0; ix < test_string_len; ++ix) {
    if (large_buffer[ix] != ktest_string[ix]) {
      ++num_errors;
      fprintf(stderr,
              ("UnitTest_WeirdBuffers: large buffer content wrong at ix %"
               NACL_PRIuS": expected '%c', got '%c'\n"),
              ix, 0xff & ktest_string[ix], 0xff & large_buffer[ix]);
    }
  }
 giveup:
  TestGioRecycler(neg);
  if (g_verbosity) {
    printf("UnitTest_BasicCtorDtor: %"NACL_PRIuS" error(s)\n", num_errors);
  }
  return num_errors;
}

size_t UnitTest_GenerateRandomOutput(void) {
  struct NaClErrorGio *neg;
  size_t num_errors = 0;
  char output_tail[NACL_ERROR_GIO_MAX_BYTES];
  size_t output_bytes;
  size_t output_discarded_bytes;
  size_t output_saved_tail_bytes;
  size_t ix;
  size_t write_chunk_size;
  char write_chunk[MAX_WRITE_CHUNK_SIZE];
  size_t jx;
  ssize_t written;
  char error_gio_content[NACL_ERROR_GIO_MAX_BYTES];
  size_t error_gio_content_bytes;

  if (g_verbosity > 0) {
    printf("UnitTest_GenerateRandomOutput\n");
  }

  neg = TestGioFactory();
  if (NULL == neg) {
    fprintf(stderr,
            ("UnitTest_GenerateRandomOutput: TestGioFactory"
             " construction failed\n"));
    ++num_errors;
    goto giveup;
  }

  output_bytes = g_output_bytes_min +
      (rand() % (g_output_bytes_max - g_output_bytes_min));

  if (output_bytes > NACL_ARRAY_SIZE(output_tail)) {
    output_saved_tail_bytes = NACL_ARRAY_SIZE(output_tail);
    output_discarded_bytes = output_bytes - output_saved_tail_bytes;
  } else {
    output_saved_tail_bytes = output_bytes;
    output_discarded_bytes = 0;
  }

  for (ix = 0; ix < output_discarded_bytes; ) {
    write_chunk_size = rand() % NACL_ARRAY_SIZE(write_chunk);
    if (write_chunk_size > output_discarded_bytes - ix) {
      write_chunk_size = output_discarded_bytes - ix;
    }
    for (jx = 0; jx < write_chunk_size; ++jx) {
      write_chunk[jx] = rand();  /* not efficient use of rng */
    }
    written = (*NACL_VTBL(Gio, neg)->Write)((struct Gio *) neg,
                                            write_chunk, write_chunk_size);
    if (written < 0) {
      ++num_errors;
      fprintf(stderr,
              ("UnitTest_GenerateRandomOutput: Write method returned %"
               NACL_PRIdS"\n"),
              written);
      goto abort_test;
    }
    ix += (size_t) written;
  }
  for (ix = 0; ix < output_saved_tail_bytes; ) {
    write_chunk_size = rand() % NACL_ARRAY_SIZE(write_chunk);
    if (write_chunk_size > output_saved_tail_bytes - ix) {
      write_chunk_size = output_saved_tail_bytes - ix;
    }
    for (jx = 0; jx < write_chunk_size; ++jx) {
      write_chunk[jx] = rand();  /* not efficient use of rng */
    }
    written = (*NACL_VTBL(Gio, neg)->Write)((struct Gio *) neg,
                                            write_chunk, write_chunk_size);
    if (written < 0) {
      ++num_errors;
      fprintf(stderr,
              ("UnitTest_GenerateRandomOutput: Write method returned %"
               NACL_PRIdS"\n"),
              written);
      goto abort_test;
    }
    for (jx = 0; jx < (size_t) written; ++jx) {
      output_tail[ix + jx] = write_chunk[jx];
    }
    ix += (size_t) written;
  }
  /*
   * Check that the final output Gio actually captured the number of bytes
   * that we think should have been captured.
   */
  if (output_saved_tail_bytes !=
      ((struct TailGio *) neg->pass_through)->num_bytes) {
    ++num_errors;
    fprintf(stderr,
            ("UnitTest_GenerateRandomOutput: TailGio buffer wrong captured"
             " size: got %"NACL_PRIuS", expected %"NACL_PRIuS"!\n"),
            ((struct TailGio *) neg->pass_through)->num_bytes,
            output_saved_tail_bytes);
  }
  /*
   * Check that the final output Gio actually saw the number of bytes
   * that we think should have been output.
   */
  if (output_bytes !=
      ((struct TailGio *) neg->pass_through)->total_output_bytes) {
    ++num_errors;
    fprintf(stderr,
            ("UnitTest_GenerateRandomOutput: TailGio buffer wrong total"
             " output size: got %"NACL_PRIuS", expected %"NACL_PRIuS"!\n"),
            ((struct TailGio *) neg->pass_through)->total_output_bytes,
            output_bytes);
  }
  /*
   * Compare output_tail with NaClErrorGioGetOutput output, and with
   * TailGio buffer.
   */
  error_gio_content_bytes =
      NaClErrorGioGetOutput(neg, error_gio_content,
                            NACL_ARRAY_SIZE(error_gio_content));
  if (output_saved_tail_bytes != error_gio_content_bytes) {
    ++num_errors;
    fprintf(stderr,
            ("UnitTest_GenerateRandomOutput: NaClErrorGio buffer wrong size:"
             " got %"NACL_PRIuS", expected %"NACL_PRIuS"!\n"),
            error_gio_content_bytes, output_saved_tail_bytes);
  }
  for (ix = 0; ix < error_gio_content_bytes; ++ix) {
    if (output_tail[ix] != error_gio_content[ix]) {
      fprintf(stderr,
              ("UnitTest_GenerateRandomOutput: byte %"NACL_PRIuS
               ": wrote %02x, got %02x\n"),
              ix, 0xff & output_tail[ix], 0xff & error_gio_content[ix]);
      ++num_errors;
    }
    if (output_tail[ix] !=
        ((struct TailGio *) neg->pass_through)->buffer[ix]) {
      fprintf(stderr,
              ("UnitTest_GenerateRandomOutput: byte %"NACL_PRIuS
               ": wrote %02x, wrote through %02x\n"),
              ix, 0xff & output_tail[ix], 0xff & error_gio_content[ix]);
      ++num_errors;
    }
  }
 abort_test:
  TestGioRecycler(neg);
 giveup:
  if (g_verbosity > 0) {
    printf("UnitTest_GenerateRandomOutput: %"NACL_PRIuS" error(s)\n",
           num_errors);
  }
  return num_errors;
}

/*
 * This test could use gtest, but does not.  Here is why:
 *
 * gtest does not have a way to specify a test parameter such as an
 * RNG seed from the command line -- except by using internal APIs to
 * specify a --gtest_<foo> flag, or by a side channel such as using an
 * environment variable.  This makes randomized tests that can serve
 * to do API fuzzing during development as well as act as a quick
 * unittest more awkward to implement.  Additionally, while gtest
 * itself has the ability to set an RNG seed, it is gtest's RNG for
 * shuffling the order of tests to be run -- there shouldn't be
 * interactions between gtest's RNG usage and the test's RNG usage, so
 * sharing the RNG and making use of gtest's ability to set RNG seed
 * is not a great idea.  Furthermore, this test should *never* fail --
 * the xml output for getting test history (e.g., to deal with flaky
 * tests) is useless: if this test fails, somebody must have modified
 * the code in an erroneous way, and the change should be rolled back.
 */
int main(int ac, char **av) {
  int opt;
  size_t num_errors = 0;
  size_t num_iter = 100000;
  size_t iter;
  unsigned seed = (unsigned) time((time_t *) NULL);

  while (-1 != (opt = getopt(ac, av, "b:B:n:s:v"))) {
    switch (opt) {
      case 'b':
        g_output_bytes_min = strtoul(optarg, (char **) NULL, 0);
        break;
      case 'B':
        g_output_bytes_max = strtoul(optarg, (char **) NULL, 0);
        break;
      case 'n':
        num_iter = strtoul(optarg, (char **) NULL, 0);
        break;
      case 's':
        seed = strtoul(optarg, (char **) NULL, 0);
      case 'v':
        ++g_verbosity;
        break;
    }
  }
  if (g_output_bytes_max < g_output_bytes_min) {
    fprintf(stderr,
            "nacl_error_gio_test: -b min_bytes value should not be larger"
            " than -B max_bytes value\n");
    return 1;
  }
  srand(seed);
  printf("seed %u\n", seed);

  num_errors += UnitTest_BasicCtorDtor();
  num_errors += UnitTest_WeirdBuffers();

  for (iter = 0; iter < num_iter; ++iter) {
    num_errors += UnitTest_GenerateRandomOutput();
  }

  return num_errors;
}
