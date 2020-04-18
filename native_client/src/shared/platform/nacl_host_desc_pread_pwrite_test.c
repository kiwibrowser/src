/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/include/portability_io.h"
#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/platform_init.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"

static const size_t kNumFileBytes = 2 * 0x10000;

/*
 * Quote to be written as initial data in test input data files.
 */
static char const quote[] = {
  "WHEN we look to the individuals of the same variety or sub-variety\n"
  "of our older cultivated plants and animals, one of the first points\n"
  "which strikes us, is, that they generally differ much more from\n"
  "each other, than do the individuals of any one species or variety\n"
  "in a state of nature. When we reflect on the vast diversity of the\n"
  "plants and animals which have been cultivated, and which have varied\n"
  "during all ages under the most different climates and treatment, I\n"
  "think we are driven to conclude that this greater variability is\n"
  "simply due to our domestic productions having been raised under\n"
  "conditions of life not so uniform as, and somewhat different from,\n"
  "those to which the parent-species have been exposed under nature.\n"
  "There is, also, I think, some probability in the view propounded by\n"
  "Andrew Knight, that this variability may be partly connected with\n"
  "excess of food. It seems pretty clear that organic beings must be\n"
  "exposed during several generations to the new conditions of life to\n"
  "cause any appreciable amount of variation; and that when the\n"
  "organisation has once begun to vary, it generally continues to vary\n"
  "for many generations. No case is on record of a variable being\n"
  "ceasing to be variable under cultivation. Our oldest cultivated\n"
  "plants, such as wheat, still often yield new varieties: our oldest\n"
  "domesticated animals are still capable of rapid improvement or\n"
  "modification.\n\n"
};

struct TestParams {
  char const *test_name;
  int open_flags;
  int file_perms;
  int (*test_func)(struct NaClHostDesc *test_file,
                   struct NaClHostDesc *ro_view,
                   void const *test_params);
  void const *test_params;
};

/*
 * Write to |d| |file_bytes| of data from |buffer|, which contains
 * |buffer_size| bytes.  The contents of buffer will be repeated as
 * needed so that the target |file_bytes| is reached.
 */
int WriteData(struct NaClHostDesc *d,
              size_t file_bytes,
              char const *buffer,
              size_t buffer_size) {
  size_t nbytes;
  size_t written = 0;
  ssize_t result;

  while (written < file_bytes) {
    nbytes = file_bytes - written;
    if (nbytes > buffer_size) {
      nbytes = buffer_size;
    }
    result = NaClHostDescWrite(d, buffer, nbytes);
    if (result < 0) {
      return (int) result;
    }
    written += result;
  }
  return 0;
}

/*
 * Write out kNumFileBytes bytes of data to |d|.
 */
int CreateTestData(struct NaClHostDesc *d) {
  return WriteData(d, kNumFileBytes, quote, sizeof quote - 1);
}

void CreateTestFile(struct NaClHostDesc *d_out,
                    struct NaClHostDesc *d_ronly_out,
                    char const *pathname,
                    struct TestParams const *param) {
  struct NaClHostDesc hd;
  int err;

  printf("pathname = %s, perms %#o\n", pathname, param->file_perms);
  if (0 != (err = NaClHostDescOpen(&hd,
                                   pathname,
                                   NACL_ABI_O_WRONLY |
                                   NACL_ABI_O_CREAT |
                                   NACL_ABI_O_TRUNC,
                                   param->file_perms))) {
    fprintf(stderr, "Could not open test scratch file: NaCl errno %d\n", -err);
    exit(1);
  }
  if (0 != (err = CreateTestData(&hd))) {
    fprintf(stderr,
            "Could not write test data into test scratch file: NaCl errno %d\n",
            -err);
    exit(1);
  }
  if (0 != (err = NaClHostDescClose(&hd))) {
    fprintf(stderr,
            "Error while closing test data file, NaCl errno %d\n", -err);
    exit(1);
  }
  if (0 != (err = NaClHostDescOpen(d_out,
                                   pathname,
                                   param->open_flags,
                                   param->file_perms))) {
    fprintf(stderr, "Could not open test scratch file: NaCl errno %d\n", -err);
    exit(1);
  }
  if (0 != (err = NaClHostDescOpen(d_ronly_out,
                                   pathname,
                                   NACL_ABI_O_RDONLY,
                                   0))) {
    fprintf(stderr,
            "Could not open read-only verification handle: NaCl errno %d\n",
            -err);
    exit(1);
  }
}

void CloseTestFile(struct NaClHostDesc *d) {
  CHECK(0 == NaClHostDescClose(d));
}

struct BasicPermChecksParams {
  size_t seq_read_bytes1;
  size_t seq_read_bytes2;

  nacl_off64_t pread_offset;
  char const *expected_pread_data;
  ssize_t expected_pread_result;

  nacl_off64_t pwrite_offset;
  char const *pwrite_data;
  ssize_t expected_pwrite_result;
};

static struct BasicPermChecksParams const gReadWriteOkay = {
  /* seq_read_bytes1= */ 105, /* seq_read_bytes2= */ 200,
  /* pread_offset= */ 730,
  /* expected_pread_data= */
  "There is, also, I think, some probability in the view propounded by\n"
  "Andrew Knight, that this variability may be partly connected with\n",
  /* expected_pread_result= */ 134,
  /* pwrite_offset= */ 100,
  /* pwrite_data= */
  "1: In the beginning God created the heaven and the earth.\n"
  "2: And the earth was without form, and void; and darkness was upon\n"
  "the face of the deep. And the Spirit of God moved upon the face of\n"
  "the waters.\n"
  "3: And God said, Let there be light: and there was light.\n",
  /* expected_pwrite_result= */ 262,
};

static struct BasicPermChecksParams const gReadWriteNoInitialRead = {
  /* seq_read_bytes1= */ 0, /* seq_read_bytes2= */ 300,
  /* pread_offset= */ 730,
  /* expected_pread_data= */
  "There is, also, I think, some probability in the view propounded by\n"
  "Andrew Knight, that this variability may be partly connected with\n",
  /* expected_pread_result= */ 134,
  /* pwrite_offset= */ 100,
  /* pwrite_data= */
  "1: In the beginning God created the heaven and the earth.\n"
  "2: And the earth was without form, and void; and darkness was upon\n"
  "the face of the deep. And the Spirit of God moved upon the face of\n"
  "the waters.\n"
  "3: And God said, Let there be light: and there was light.\n",
  /* expected_pwrite_result= */ 262,
};

static struct BasicPermChecksParams const gReadNoWrite = {
  /* seq_read_bytes1= */ 64, /* seq_read_bytes2= */ 128,
  /* pread_offset= */ 730,
  /* expected_pread_data= */
  "There is, also, I think, some probability in the view propounded by\n"
  "Andrew Knight, that this variability may be partly connected with\n",
  /* expected_pread_result= */ 134,
  /* pwrite_offset= */ 0,
  /* pwrite_data= */ NULL,
  /* expected_pwrite_result= */0,
};

static struct BasicPermChecksParams const gReadWriteFailure = {
  /* seq_read_bytes1= */ 64, /* seq_read_bytes2= */ 128,
  /* pread_offset= */ 730,
  /* expected_pread_data= */
  "There is, also, I think, some probability in the view propounded by\n"
  "Andrew Knight, that this variability may be partly connected with\n",
  /* expected_pread_result= */ 134,
  /* pwrite_offset= */ 100,
  /* pwrite_data= */
  "from hell's heart I stab at thee;"
  " for hate's sake I spit my last breath at thee",
  /* expected_pwrite_result= */ -NACL_ABI_EBADF,
};

static struct BasicPermChecksParams const gReadFailureWrite = {
  /* seq_read_bytes1= */ 0, /* seq_read_bytes2= */ 0,
  /* pread_offset= */ 730,
  /* expected_pread_data= */
  "There is, also, I think, some probability in the view propounded by\n"
  "Andrew Knight, that this variability may be partly connected with\n",
  /* expected_pread_result= */ -NACL_ABI_EBADF,
  /* pwrite_offset= */ 100,
  /* pwrite_data= */
  "from hell's heart I stab at thee;"
  " for hate's sake I spit my last breath at thee",
  /* expected_pwrite_result= */ 79,
};

int BasicPermChecks(struct NaClHostDesc *test_file,
                    struct NaClHostDesc *ro_view,
                    void const *test_params) {
  struct BasicPermChecksParams const *params =
      (struct BasicPermChecksParams const *) test_params;
  char buffer[1<<16];
  ssize_t result;
  nacl_off64_t seek_result;
  size_t len;

  CHECK(params->seq_read_bytes1 <= sizeof buffer);
  CHECK(params->seq_read_bytes2 <= sizeof buffer);
  seek_result = NaClHostDescSeek(test_file, 0, 0);
  ASSERT_EQ(0, seek_result);
  /*
   * Some tests skip the initial read.
   */
  if (0 != params->seq_read_bytes1) {
    memset(buffer, 0, sizeof buffer);
    CHECK(params->seq_read_bytes1 <= sizeof buffer);
    result = NaClHostDescRead(test_file, buffer, params->seq_read_bytes1);
    if (result != (ssize_t) params->seq_read_bytes1) {
      fprintf(stderr,
              "BasicPermChecks: first sequential read, got %"NACL_PRIdS
              ", expected %"NACL_PRIuS" bytes\n",
              result, params->seq_read_bytes1);
      return 0;
    }
    if (0 != memcmp(quote, buffer, params->seq_read_bytes1)) {
      fprintf(stderr,
              "BasicPermChecks: first sequential read result differs\n");
      fprintf(stderr, "got: %.*s\n", (int) params->seq_read_bytes1, buffer);
      return 0;
    }
  }
  len = strlen(params->expected_pread_data);
  memset(buffer, 0, sizeof buffer);
  CHECK(len <= sizeof buffer);
  result = NaClHostDescPRead(test_file, buffer, len, params->pread_offset);
  if (result != (ssize_t) params->expected_pread_result) {
    fprintf(stderr,
            "BasicPermChecks: pread failed, got %"NACL_PRIdS", expected %"
            NACL_PRIuS" bytes\n",
            result, params->expected_pread_result);
    return 0;
  }
  if (params->expected_pread_result > 0 &&
      0 != memcmp(params->expected_pread_data, buffer, len)) {
    fprintf(stderr,
            "BasicPermChecks: pread result: %.*s\n"
            "              expected output: %.*s\n",
            (int) len, buffer,
            (int) len, quote + params->pread_offset);
    return 0;
  }
  if (0 != params->seq_read_bytes2) {
    /*
     * Make sure that the read pointer did not change.
     */
    memset(buffer, 0, sizeof buffer);
    CHECK(params->seq_read_bytes2 <= sizeof buffer);
    result = NaClHostDescRead(test_file, buffer, params->seq_read_bytes2);
    if (result != (ssize_t) params->seq_read_bytes2) {
      fprintf(stderr,
              "BasicPermChecks: second sequential read, got %"NACL_PRIdS
              ", expected %"NACL_PRIuS" bytes\n",
              result, params->seq_read_bytes2);
      return 0;
    }
    if (0 != memcmp(quote + params->seq_read_bytes1, buffer,
                    params->seq_read_bytes2)) {
      fprintf(stderr,
              "BasicPermChecks: second sequential read result differs\n");
      fprintf(stderr, "got: %.*s\n", (int) params->seq_read_bytes1, buffer);
      return 0;
    }
  }
  /*
   * Now try to write.
   */
  if (NULL != params->pwrite_data) {
    len = strlen(params->pwrite_data);
    result = NaClHostDescPWrite(test_file, params->pwrite_data, len,
                                params->pwrite_offset);
    if (result != params->expected_pwrite_result) {
      fprintf(stderr, "BasicPermChecks: pwrite failed, got %"NACL_PRIdS
              ", expected %"NACL_PRIdS"\n",
              result, params->expected_pwrite_result);
      return 0;
    }
    if (result == (ssize_t) len) {
      /* re-read by seek/sequential read and check data, using ro_view */
      seek_result = NaClHostDescSeek(ro_view, params->pwrite_offset, 0);
      ASSERT_EQ(params->pwrite_offset, seek_result);
      memset(buffer, 0, sizeof buffer);
      CHECK(len <= sizeof buffer);
      result = NaClHostDescRead(ro_view, buffer, len);
      if (result != (ssize_t) len) {
        fprintf(stderr, "BasicPermChecks: pwrite verification read failed\n");
        fprintf(stderr, " got %"NACL_PRIdS", expected %"NACL_PRIuS"\n",
                result, len);
        return 0;
      }
      if (0 != memcmp(buffer, params->pwrite_data, len)) {
        fprintf(stderr, "BasicPermChecks: pwrite verification data mismatch\n");
        fprintf(stderr,
                "     got %.*s\n"
                "expected %.*s\n",
                (int) len, buffer,
                (int) len, params->pwrite_data);
        return 0;
      }
    }
  }
  return 1;
}

int SeekPastEndAndWriteTest(struct NaClHostDesc *test_file,
                            struct NaClHostDesc *ro_view,
                            void const *test_params) {
  nacl_host_stat_t stbuf;
  nacl_off64_t new_size;
  ssize_t io_err;
  int err;
  nacl_off64_t seek_result;

  UNREFERENCED_PARAMETER(test_params);
  err = NaClHostDescFstat(test_file, &stbuf);
  if (0 != err) {
    fprintf(stderr, "SeekPastEndAndWriteTest: fstat failed\n");
    return 0;
  }
  new_size = stbuf.st_size + (2 << 16);
  seek_result = NaClHostDescSeek(test_file, new_size, 0);
  if (seek_result != new_size) {
    fprintf(stderr, "SeekPastEndAndWriteTest: seek failed\n");
    return 0;
  }
  io_err = NaClHostDescWrite(test_file, "1", 1);
  if (1 != io_err) {
    fprintf(stderr, "SeekPastEndAndWriteTest: write failed: %"NACL_PRIdS"\n",
            io_err);
  }
  err = NaClHostDescFstat(test_file, &stbuf);
  if (0 != err) {
    fprintf(stderr, "SeekPastEndAndWriteTest: post-write fstat failed\n");
    return 0;
  }
  ASSERT_EQ(stbuf.st_size, new_size + 1);
  err = NaClHostDescFstat(ro_view, &stbuf);
  if (0 != err) {
    fprintf(stderr,
            "SeekPastEndAndWriteTest: post-write ro_view fstat failed\n");
    return 0;
  }
  ASSERT_EQ(stbuf.st_size, new_size + 1);
  return 1;
}

int PReadWithNegativeOffset(struct NaClHostDesc *test_file,
                            struct NaClHostDesc *ro_view,
                            void const *test_params) {
  char buffer[1<<16];
  ssize_t result;

  UNREFERENCED_PARAMETER(ro_view);
  UNREFERENCED_PARAMETER(test_params);

  result = NaClHostDescPRead(test_file, buffer, sizeof buffer, -10);
  if (result != -NACL_ABI_EINVAL) {
    fprintf(stderr, "PReadWithNegativeOffset: got error %d, expected %d\n",
            -(int) result, NACL_ABI_EINVAL);
    return 0;
  }
  return 1;
}

static char const another_quote[] = {
  "Il y a deux classes de savants. Les uns, sui-\n"
  "vant les traces de leurs pr\\'ed\\'ecesseurs, agrandis-\n"
  "sent le domaine de la science et ajoutent des\n"
  "d\\'ecouvertes \\`a celles qui ont \\'et\\'e faites avant eux ;\n"
  "leurs travaux sont imm\\'ediatement appr\\'eci\\'es, et\n"
  "ils jouissent pleinement d'une r\\'eputation bien\n"
  "m\\'erit\\'ee. Les autres, quittant les sentiers battus,\n"
  "s'affranchissent de la tradition, font \\'eclore les\n"
  "germes de l'avenir, latents pour ainsi dire dans\n"
  "les enseignements du pass\\'e : quelquefois ils sont\n"
  "estim\\'es pendant leur vie \\`a leur juste valeur; plus\n"
  "souvent encore ils passent m\\'econnus du public\n"
  "scientifique de leur \\'epoque , incapable de les com\n"
};

int PWriteUsesOffsetSeekReadVerification(struct NaClHostDesc *test_file,
                                         struct NaClHostDesc *ro_view,
                                         void const *test_params) {
  size_t len = strlen(another_quote);
  char buffer[4096];
  ssize_t io_rv;
  nacl_off64_t seek_err;

  UNREFERENCED_PARAMETER(ro_view);
  UNREFERENCED_PARAMETER(test_params);

  CHECK(len <= sizeof buffer);

  io_rv = NaClHostDescPWrite(test_file, another_quote, len, GG_LONGLONG(0));
  if (io_rv < 0) {
    fprintf(stderr,
            "PWriteUsesOffsetSeekReadVerification:"
            " pwrite (ostensibly to beginning) failed,"
            " errno %d\n", -(int) io_rv);
    return 0;
  }
  if ((size_t) io_rv != len) {
    fprintf(stderr,
            "PWriteUsesOffsetSeekReadVerification:"
            " pwrite short write, expected %"NACL_PRIuS
            ", got %"NACL_PRIuS"\n",
            len, (size_t) io_rv);
    return 0;
  }
  seek_err = NaClHostDescSeek(test_file, 0, 0);
  if (seek_err < 0) {
    fprintf(stderr,
            "PWriteUsesOffsetSeekReadVerification:"
            " seek failed, errno %d\n", -(int) seek_err);
    return 0;
  }
  ASSERT_EQ(seek_err, (nacl_off64_t) 0);
  CHECK(len <= sizeof buffer);
  io_rv = NaClHostDescRead(test_file, buffer, len);
  if (io_rv < 0) {
    fprintf(stderr,
            "PWriteUsesOffsetSeekReadVerification:"
            " NaClHostDescRead failed, errno %d\n",
            -(int) io_rv);
    return 0;
  }
  if ((size_t) io_rv != len) {
    fprintf(stderr,
            "PWriteUsesOffsetSeekReadVerification:"
            " short read, expected %"NACL_PRIuS
            ", got %"NACL_PRIuS"\n",
            len, (size_t) io_rv);
    return 0;
  }
  if (0 != memcmp(buffer, another_quote, len)) {
    fprintf(stderr,
            "PWriteUsesOffsetSeekReadVerification:"
            " data at beginning differs\n"
            " Expected: %.*s\n"
            "      Got: %.*s\n",
            (int) len, another_quote,
            (int) len, buffer);
    return 0;
  }
  return 1;
}
int PWriteUsesOffsetPReadVerification(struct NaClHostDesc *test_file,
                                      struct NaClHostDesc *ro_view,
                                      void const *test_params) {
  size_t len = strlen(another_quote);
  char buffer[4096];
  ssize_t io_rv;

  UNREFERENCED_PARAMETER(test_params);

  CHECK(len <= sizeof buffer);

  io_rv = NaClHostDescPWrite(test_file, another_quote, len, GG_LONGLONG(0));
  if (io_rv < 0) {
    fprintf(stderr,
            "PWriteUsesOffsetPReadVerification:"
            " pwrite (ostensibly to beginning) failed,"
            " errno %d\n", -(int) io_rv);
    return 0;
  }
  if ((size_t) io_rv != len) {
    fprintf(stderr,
            "PWriteUsesOffsetPReadVerification:"
            " pwrite short write, expected %"NACL_PRIuS
            ", got %"NACL_PRIuS"\n",
            len, (size_t) io_rv);
    return 0;
  }
  CHECK(len <= sizeof buffer);
  io_rv = NaClHostDescPRead(ro_view, buffer, len, GG_LONGLONG(0));
  if (io_rv < 0) {
    fprintf(stderr,
            "PWriteUsesOffsetPReadVerification:"
            " NaClHostPDescRead failed, errno %d\n",
            -(int) io_rv);
    return 0;
  }
  if ((size_t) io_rv != len) {
    fprintf(stderr,
            "PWriteUsesOffsetPReadVerification:"
            " short read, expected %"NACL_PRIuS
            ", got %"NACL_PRIuS"\n",
            len, (size_t) io_rv);
    return 0;
  }
  if (0 != memcmp(buffer, another_quote, len)) {
    fprintf(stderr,
            "PWriteUsesOffsetPReadVerification:"
            " data at end differs\n"
            " Expected: %.*s\n"
            "      Got: %.*s\n",
            (int) len, another_quote,
            (int) len, buffer);
    return 0;
  }
  return 1;
}

int PWriteUsesOffsetReadPtrVerification(struct NaClHostDesc *test_file,
                                        struct NaClHostDesc *ro_view,
                                        void const *test_params) {
  nacl_off64_t seek_err;
  char buffer[512];
  ssize_t io_rv;
  const size_t pwrite_position = 4096;

  UNREFERENCED_PARAMETER(test_params);

  seek_err = NaClHostDescSeek(test_file, 0, 0);
  if (seek_err < 0) {
    fprintf(stderr,
            "PWriteUsesOffsetReadPtrVerification: seek failed, errno %d\n",
            -(int) seek_err);
    return 0;
  }
  io_rv = NaClHostDescRead(test_file, buffer, sizeof buffer);
  if (sizeof buffer != io_rv) {
    fprintf(stderr,
            "PWriteUsesOffsetReadPtrVerification: read failed, got %d,"
            " expected %d",
            (int) io_rv, (int) sizeof buffer);
    return 0;
  }
  if (0 != memcmp(buffer, quote, sizeof buffer)) {
    fprintf(stderr,
            "PWriteUsesOffsetReadPtrVerification: initial bytes mangled?!?\n");
    return 0;
  }
  io_rv = NaClHostDescPWrite(test_file, another_quote, sizeof another_quote - 1,
                             pwrite_position);
  if (io_rv != sizeof another_quote - 1) {
    fprintf(stderr,
            "PWriteUsesOffsetReadPtrVerification: pwrite failed\n");
    return 0;
  }
  io_rv = NaClHostDescRead(test_file, buffer, sizeof buffer);
  if (sizeof buffer != io_rv) {
    fprintf(stderr,
            "PWriteUsesOffsetReadPtrVerification:"
            " 2nd implicit file ptr read failed, got %d,"
            " expected %d\n",
            (int) io_rv, (int) sizeof buffer);
    return 0;
  }
  if (0 != memcmp(buffer, quote + sizeof buffer, sizeof buffer)) {
    fprintf(stderr,
            "PWriteUsesOffsetReadPtrVerification: 2nd read contents mangled\n"
            "Expected:\n"
            "%.*s\n"
            "Got:\n"
            "%.*s\n",
            (int) sizeof buffer, quote + sizeof buffer,
            (int) sizeof buffer, buffer);
    return 0;
  }

  io_rv = NaClHostDescPRead(ro_view, buffer, sizeof buffer,
                            pwrite_position);
  if (io_rv != sizeof buffer) {
    fprintf(stderr,
            "PWriteUsesOffsetReadPtrVerification: verification pread failed,"
            " got %d, expected %d",
            (int) io_rv, (int) sizeof buffer);
    return 0;
  }
  if (0 != memcmp(buffer, another_quote, sizeof buffer)) {
    fprintf(stderr,
            "PWriteUsesOffsetReadPtrVerification: verification content bad\n"
            "Got:\n"
            "%.*s\n"
            "Expected:\n"
            "%.*s\n",
            (int) sizeof buffer, buffer,
            (int) sizeof buffer, another_quote);
    return 0;
  }

  return 1;
}

static struct TestParams const tests[] = {
  {
    "O_RDWR, read, pread, read, pwrite",
    NACL_ABI_O_RDWR, 0666,
    BasicPermChecks,
    &gReadWriteOkay,
  }, {
    "O_RDWR, pread, read, pwrite",
    NACL_ABI_O_RDWR, 0666,
    BasicPermChecks,
    &gReadWriteNoInitialRead,
  }, {
    "O_RDWR, read, pread, read",
    NACL_ABI_O_RDWR, 0666,
    BasicPermChecks,
    &gReadNoWrite,
  }, {
    "O_RDONLY, read, pread, read, pwrite-fail",
    NACL_ABI_O_RDONLY, 0666,
    BasicPermChecks,
    &gReadWriteFailure,
  }, {
    "O_WRONLY, pread-fail, pwrite",
    NACL_ABI_O_WRONLY, 0666,
    BasicPermChecks,
    &gReadFailureWrite,
  }, {
    "O_RDWR, seek past end-of-file and write",
    NACL_ABI_O_RDWR, 0666,
    SeekPastEndAndWriteTest,
    NULL,
  }, {
    "O_RDWR, pread with negative offset",
    NACL_ABI_O_RDWR, 0666,
    PReadWithNegativeOffset,
    NULL,
  }, {
    "O_RDWR | O_APPEND, check pwrites show up w/ seek/read",
    NACL_ABI_O_RDWR | NACL_ABI_O_APPEND, 0666,
    PWriteUsesOffsetSeekReadVerification,
    NULL,
  }, {
    "O_RDWR | O_APPEND, check pwrites show up w/ pread",
    NACL_ABI_O_RDWR | NACL_ABI_O_APPEND, 0666,
    PWriteUsesOffsetPReadVerification,
    NULL,
  }, {

    "O_WRONLY | O_APPEND, check pwrites show up w/ pread",
    NACL_ABI_O_WRONLY | NACL_ABI_O_APPEND, 0666,
    PWriteUsesOffsetPReadVerification,
    NULL,
  }, {
    "O_RDWR | O_APPEND, check seek 0, read, pwrite, read behaves consistently",
    NACL_ABI_O_RDWR | NACL_ABI_O_APPEND, 0666,
    PWriteUsesOffsetReadPtrVerification,
    NULL,
  },
};

/*
 * It is the responsibility of the invoking environment to delete the
 * file passed as command-line argument.  See the build.scons file.
 */
int main(int ac, char **av) {
  char const *test_dir_name = "/tmp/nacl_host_desc_test";
  struct NaClHostDesc hd;
  struct NaClHostDesc hd_ro;
  size_t error_count;
  size_t ix;
  int test_passed;
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
                "Usage: nacl_host_desc_mmap_test [-c run_count]\n"
                "                                [-t test_temp_dir]\n");
        exit(1);
    }
  }

  NaClPlatformInit();

  error_count = 0;
  for (test_run = 0; test_run < num_runs; ++test_run) {
    printf("Test run %d\n\n", test_run);
    for (ix = 0; ix < NACL_ARRAY_SIZE(tests); ++ix) {
      char test_file_name[PATH_MAX];
      SNPRINTF(test_file_name, sizeof test_file_name,
               "%s/f%d.%"NACL_PRIuS, test_dir_name, test_run, ix);
      printf("%s\n", tests[ix].test_name);
      CreateTestFile(&hd, &hd_ro, test_file_name, &tests[ix]);
      test_passed = (*tests[ix].test_func)(&hd, &hd_ro, tests[ix].test_params);
      CloseTestFile(&hd);
      error_count += !test_passed;
      printf("%s\n", test_passed ? "PASSED" : "FAILED");
    }
  }

  NaClPlatformFini();

  /* we ignore the 2^32 or 2^64 total errors case */
  return (error_count > 255) ? 255 : error_count;
}
