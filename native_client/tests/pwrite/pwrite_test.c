/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static char const *g_dirname = "/tmp/pwrite_test";


/*
 * At least one test case bake in knowledge of relative sizes of these
 * quotes.
 */

static char const quote[] =
    "From a little after two oclock until almost sundown of the long still "
    "hot weary dead September afternoon they sat in what Miss Coldfield still "
    "called the office because her father had called it that---a dim hot "
    "airless room with the blinds all closed and fastened for forty-three "
    "summers because when she was a girl someone had believed that light and "
    "moving air carried heat and that dark was always cooler, and which (as "
    "the sun shone fuller and fuller on that side of the house) became "
    "latticed with yellow slashes full of dust motes which Quentin thought of "
    "as being flecks of the dead old dried paint itself blown inward from the "
    "scaling blinds as wind might have blown them.  There was a wistaria vine "
    "blooming for the second time that summer on a wooden trellis before one "
    "window, into which sparrows came now and then in random gusts, making a "
    "dry vivid dusty sound before going away: and opposite Quentin, Miss "
    "Coldfield in the eternal black which she had worn for forty-three years "
    "now, whether for her sister, father, or nothusband none knew, sitting so "
    "bolt upright in the straight hard chair that was so tall for her that "
    "her legs hung straight and rigid as if she had iron shinbones and "
    "ankles, clear of the floor with that air of impotent and static rage "
    "like children's feet, and talking in that grim haggard amazed voice "
    "until at last listening would renege and hearing-sense self-confound and "
    "the long-dead object of her impotent yet indomitable frustration wuld "
    "appear, as though by outraged recapitulation evoked, quiet inattentive "
    "and harmless, out of the biding and dreamy and victorious dust.";

static char const overwrite[] =
    "It was about eleven o'clock in the morning, mid October, with the sun "
    "not shining and a look of hard wet rain in the clearness of the "
    "foothills.  I was wearing my powder-blue suit, with dark blue shirt, tie "
    "and display handkerchief, black brogues, black wool socks with dark blue "
    "clocks on them.  I was neat, clean, shaved and sober, and I didn't care "
    "who knew it.  I was everything the well-dressed private detective ought "
    "to be.  I was calling on four million dollars.";

static char const first_seq_write[] =
    "Mr and Mrs Dursley, of number four, Privet Drive, were proud to say that "
    "they were perfectly normal, thank you very much. ";

static char const second_seq_write[] =
    "Not for the first time, an argument had broken out over breakfast at "
    "number four, Privet Drive.";

struct TestCase {
  char const *test_name;
  int (*test_func)(int wr_fd, int rd_fd, void *test_specifics);
  int wr_fd_open_flag;
  void *test_specifics;
};

static void InitializeFile(char const *filename,
                           struct TestCase const *test_case,
                           int *wr_fd,
                           int *rd_fd) {
  FILE *iob = fopen(filename, "w");
  size_t len = sizeof quote - 1;

  if (NULL == iob) {
    fprintf(stderr, "Could not open \"%s\" for initialization.\n", filename);
    exit(1);
  }

  if (fwrite(quote, 1, len, iob) != len) {
    fprintf(stderr, "Could not initialize contents of \"%s\".\n", filename);
    exit(1);
  }
  if (fclose(iob)) {
    fprintf(stderr, "Could not close \"%s\" post initialization.\n", filename);
    exit(1);
  }
  *wr_fd = open(filename, test_case->wr_fd_open_flag, 0);
  if (-1 == *wr_fd) {
    fprintf(stderr, "Could not open \"%s\" for test write access\n", filename);
    exit(1);
  }
  *rd_fd = open(filename, O_RDONLY, 0);
  if (-1 == *rd_fd) {
    fprintf(stderr, "Could not open \"%s\" for test read verfication access\n",
            filename);
    exit(1);
  }
}

static int VerifyContents(int rd_fd, char const *expected_data, size_t nbytes,
                          off_t offset) {
  char buffer[4096];
  size_t chunk_size;
  ssize_t result;

  while (nbytes > 0) {
    chunk_size = nbytes;
    if (chunk_size > sizeof buffer) {
      chunk_size = sizeof buffer;
    }
    result = pread(rd_fd, buffer, chunk_size, offset);
    if (result != chunk_size) {
      fprintf(stderr,
              "VerifyContents: pread failed, got %zd, errno %d, expected %zu\n",
              result, errno, chunk_size);
      return 0;
    }
    if (0 != memcmp(expected_data, buffer, chunk_size)) {
      fprintf(stderr,
              "VerifyContents: unexpected data starting at offset %llu\n"
              "Got:\n%.*s\nExpected:\n%.*s\n",
              offset,
              chunk_size, buffer, chunk_size, expected_data);
      return 0;
    }
    expected_data += chunk_size;
    nbytes -= chunk_size;
    offset += chunk_size;
  }
  return 1;
}

static int PwriteObeysOffset(int wr_fd, int rd_fd, void *test_specifics) {
  char buffer[4096];
  ssize_t nbytes;

  if (sizeof buffer < sizeof overwrite - 1) {
    fprintf(stderr,
            "PwriteObeysOffset: configuration error: buffer too small\n");
    return 1;
  }
  if (lseek(wr_fd, 0, SEEK_END) == (off_t) -1) {
    fprintf(stderr, "PwriteObeysOffset: seek to end before write failed\n");
    return 1;
  }
  if (write(wr_fd, "Z", 1) != 1) {
    fprintf(stderr, "PwriteObeysOffset: write at end failed\n");
    return 1;
  }
  nbytes = pwrite(wr_fd, overwrite, sizeof overwrite - 1, (off_t) 0);
  if (nbytes != sizeof overwrite - 1) {
    fprintf(stderr,
            "PwriteObeysOffset: pwrite to beginning failed,"
            " expected %zu, got %zd\n",
            sizeof overwrite - 1, nbytes);
    return 1;
  }
  if (!VerifyContents(rd_fd, overwrite, sizeof overwrite - 1, 0)) {
    fprintf(stderr,
            "PwriteObeysOffset: expected overwritten data wrong\n");
    return 1;
  }
  if (!VerifyContents(rd_fd, quote + sizeof overwrite - 1,
                      sizeof quote - sizeof overwrite,
                      sizeof overwrite - 1)) {
    fprintf(stderr,
            "PwriteObeysOffset: bytes should have been unchanged\n");
    return 1;
  }
  if (!VerifyContents(rd_fd, "Z", 1, sizeof quote - 1)) {
    fprintf(stderr,
            "PwriteObeysOffset: trailing Z missing?!?\n");
  }

  return 0;
}

static int PwriteDoesNotAffectReadPos(int wr_fd, int rd_fd,
                                      void *test_specifics) {
  size_t const overwrite_overlap = 100;
  size_t read_count;
  char buffer[4096];
  ssize_t nbytes;

  read_count = ((sizeof overwrite - 1) - overwrite_overlap);
  nbytes = read(wr_fd, buffer, read_count);
  if (nbytes != read_count) {
    fprintf(stderr,
            "PwriteDoesNotAffectReadPos: initial read to set file pos"
            " failed, got %zd, errno %d\n", nbytes, errno);
    return 1;
  }
  if (0 != memcmp(buffer, quote, read_count)) {
    fprintf(stderr,
            "PwriteDoesNotAffectReadPos: initial read result unexpected, got:\n"
            "%.*s\nexpected:\n%.*s\n",
            read_count, buffer, read_count, quote);
    return 1;
  }

  nbytes = pwrite(wr_fd, overwrite, sizeof overwrite - 1, 0);
  if (nbytes != sizeof overwrite - 1) {
    fprintf(stderr,
            "PwriteDoesNotAffectReadPos: pwrite failed, expected %zu,"
            " got %zd\n", sizeof overwrite - 1, nbytes);
    return 1;
  }
  read_count = sizeof quote - 1 - read_count;
  nbytes = read(wr_fd, buffer, read_count);
  if (nbytes != read_count) {
    fprintf(stderr,
            "PwriteDoesNotAffectReadPos: read after pwrite failed, got"
            " %zd, errno %d; expected %zu\n",
            nbytes, errno, read_count);
    return 1;
  }
  if (0 != memcmp(overwrite + sizeof overwrite - 1 - overwrite_overlap,
                  buffer, overwrite_overlap)) {
    fprintf(stderr,
            "PwriteDoesNotAffectReadPos: overwritten data did not show up.\n"
            "Got:\n%.*s\nExpected:\n%.*s\n",
            overwrite_overlap,
            buffer,
            overwrite_overlap,
            overwrite + sizeof overwrite - 1 - overwrite_overlap);
    return 1;
  }
  if (0 != memcmp(buffer + overwrite_overlap,
                  quote + sizeof overwrite - 1,
                  read_count - overwrite_overlap)) {
    fprintf(stderr,
            "PwriteDoesNotAffectReadPos: data beyond overwritten changed.\n"
            "Got:\n%.*s\nExpected:\n%.*s\n",
            read_count - overwrite_overlap,
            buffer + overwrite_overlap,
            read_count - overwrite_overlap,
            quote + sizeof overwrite - 1);
    return 1;
  }
  if (!VerifyContents(rd_fd,
                      overwrite, sizeof overwrite - 1,
                      0)) {
    fprintf(stderr, "PwriteDoesNotAffectReadPos: overwritten data bad\n");
    return 1;
  }
  if (!VerifyContents(rd_fd,
                      quote + sizeof overwrite - 1,
                      sizeof quote - sizeof overwrite,
                      sizeof overwrite - 1)) {
    fprintf(stderr, "PwriteDoesNotAffectReadPos: non-overwritten data bad\n");
    return 1;
  }

  return 0;
}

static int PwriteDoesNotAffectWritePos(int wr_fd, int rd_fd,
                                       void *test_specifics) {
  /*
   * Write first_seq_write to file, overwriting initial contents.
   * Next, pwrite to some a non-overlapping location (end of file).
   * Write second_seq_write to file, expecting that it would follow
   * the contents of first_seq_write.  Read file to verify.
   */
  ssize_t result;
  size_t seq_bytes;
  int expect_append = (int) test_specifics;

  result = write(wr_fd, first_seq_write, sizeof first_seq_write - 1);

  if (result != sizeof first_seq_write - 1) {
    fprintf(stderr,
            "PwriteDoesNotAffectWritePos: writing first_seq_write failed:"
            " got %zd, errno %d, expected %zu\n",
            result, errno, sizeof first_seq_write - 1);
    return 1;
  }
  result = pwrite(wr_fd, overwrite, sizeof overwrite - 1, sizeof quote - 1);
  if (result != sizeof overwrite - 1) {
    fprintf(stderr,
            "PwriteDoesNotAffectWritePos: pwriting overwrite failed:"
            " got %zd, errno %d, expected %zu\n",
            result, errno, sizeof overwrite - 1);
    return 1;
  }
  result = write(wr_fd, second_seq_write, sizeof second_seq_write - 1);
  if (result != sizeof second_seq_write - 1) {
    fprintf(stderr,
            "PwriteDoesNotAffectWritePos: writing second_seq_write failed:"
            " got %zd, errno %d, expected %zu\n",
            result, errno, sizeof second_seq_write - 1);
    return 1;
  }
  if (expect_append) {
    if (!VerifyContents(rd_fd, quote, sizeof quote - 1, 0)) {
      fprintf(stderr,
              "PwriteDoesNotAffectWritePos: original data overwritten?!?\n");
      return 1;
    }
    if (!VerifyContents(rd_fd, overwrite,
                        sizeof overwrite - 1,
                        sizeof quote - 1)) {
      fprintf(stderr,
              "PwriteDoesNotAffectWritePos: pwrite content clobbered?\n");
      return 1;
    }
    if (!VerifyContents(rd_fd, second_seq_write,
                        sizeof second_seq_write - 1,
                        sizeof quote - 1 + sizeof overwrite - 1)) {
      fprintf(stderr,
              "PwriteDoesNotAffectWritePos: second_seq_write content"
              " clobbered?\n");
      return 1;
    }
  } else {
    if (!VerifyContents(rd_fd, first_seq_write, sizeof first_seq_write - 1,
                        0)) {
      fprintf(stderr,
              "PwriteDoesNotAffectWritePos: first_seq_write data clobbered!\n");
      return 1;
    }
    if (!VerifyContents(rd_fd, second_seq_write, sizeof second_seq_write - 1,
                        sizeof first_seq_write - 1)) {
      fprintf(stderr,
              "PwriteDoesNotAffectWritePos: second_seq_write data"
              " clobbered!\n");
      return 1;
    }
    seq_bytes = sizeof first_seq_write - 1 + sizeof second_seq_write - 1;
    if (!VerifyContents(rd_fd,
                        quote + seq_bytes,
                        sizeof quote - 1 - seq_bytes,
                        seq_bytes)) {
      fprintf(stderr,
              "PwriteDoesNotAffectWritePos: original quote data clobbered!\n");
      return 1;
    }
    if (!VerifyContents(rd_fd, overwrite, sizeof overwrite - 1,
                        sizeof quote - 1)) {
      fprintf(stderr,
              "PwriteDoesNotAffectWritePos: pwrite data clobbered!\n");
      return 1;
    }
  }
  return 0;
}

static int PreadObeysOffsetAndDoesNotAffectReadPtr(int wr_fd, int rd_fd,
                                                   void *test_specifics) {
  ssize_t io_rv;
  static size_t const kFirstReadBytes = 128;
  static off_t const kPreadOffset = 256;
  static size_t const kPreadBytes = 128;
  char buffer[4096];

  assert(kFirstReadBytes <= sizeof buffer);
  assert(kFirstReadBytes <= sizeof quote - 1);
  io_rv = read(wr_fd, buffer, kFirstReadBytes);
  if (io_rv != kFirstReadBytes) {
    fprintf(stderr,
            "PreadObeysOffsetAndDoesNotAffectReadPtr: first read failed\n");
    return 1;
  }
  if (0 != memcmp(buffer, quote, kFirstReadBytes)) {
    fprintf(stderr,
            "PreadObeysOffsetAndDoesNotAffectReadPtr: first read contents bad\n"
            "Got:\n"
            "%.*s\n"
            "Expected:\n"
            "%.*s\n",
            (int) io_rv, buffer,
            (int) kFirstReadBytes, quote);
    return 1;
  }
  assert(kPreadBytes <= sizeof buffer);
  io_rv = pread(wr_fd, buffer, kPreadBytes, kPreadOffset);
  if (io_rv != kPreadBytes) {
    fprintf(stderr,
            "PreadObeysOffsetAndDoesNotAffectReadPtr:"
            " pread returned %d, errno %d, expected %d\n",
            (int) io_rv, errno, (int) kPreadBytes);
    return 1;
  }
  if (0 != memcmp(buffer, quote + kPreadOffset, kPreadBytes)) {
    fprintf(stderr,
            "PreadObeysOffsetAndDoesNotAffectReadPtr: pread content bad\n"
            "Got:\n"
            "%.*s\n"
            "Expected:\n"
            "%.*s\n",
            (int) io_rv, buffer,
            (int) kPreadBytes, quote + kPreadOffset);
    return 1;
  }
  assert(sizeof quote - 1 - kFirstReadBytes <= sizeof buffer);
  io_rv = read(wr_fd, buffer, sizeof buffer);
  if (io_rv != sizeof quote - 1 - kFirstReadBytes) {
    fprintf(stderr,
            "PreadObeysOffsetAndDoesNotAffectReadPtr:"
            " bad byte count on 2nd read\n");
    return 1;
  }
  if (0 != memcmp(buffer, quote + kFirstReadBytes,
                  sizeof quote - 1 - kFirstReadBytes)) {
    fprintf(stderr,
            "PreadObeysOffsetAndDoesNotAffectReadPtr: 2nd read contents bad\n"
            "Got:\n"
            "%.*s\n"
            "Expected:\n"
            "%.*s\n",
            (int) io_rv, buffer,
            (int) sizeof quote - 1 - kFirstReadBytes, quote + kFirstReadBytes);
    return 1;
  }
  return 0;
}

static struct TestCase const g_test_cases[] = {
  {
    "Pwrite obeys offset, O_RDWR",
    PwriteObeysOffset,
    O_RDWR,
    (void *) NULL
  }, {
    "Pwrite obeys offset, O_WRONLY",
    PwriteObeysOffset,
    O_WRONLY,
    (void *) NULL
  }, {
    "Pwrite obeys offset, O_RDWR | O_APPEND",
    PwriteObeysOffset,
    O_RDWR | O_APPEND,
    (void *) NULL
  }, {
    "Pwrite obeys offset, O_WRONLY | O_APPEND",
    PwriteObeysOffset,
    O_WRONLY | O_APPEND,
    (void *) NULL
  }, {
    "Pwrite does not affect shared file pos (read), O_RDWR",
    PwriteDoesNotAffectReadPos,
    O_RDWR,
    (void *) NULL
  }, {
    "Pwrite does not affect shared file pos (read), O_RDWR | O_APPEND",
    PwriteDoesNotAffectReadPos,
    O_RDWR | O_APPEND,
    (void *) NULL
  }, {
    "Pwrite does not affect shared file pos (write), O_RDWR",
    PwriteDoesNotAffectWritePos,
    O_RDWR,
    (void *) NULL
  }, {
    "Pwrite does not affect shared file pos (write), O_WRONLY",
    PwriteDoesNotAffectWritePos,
    O_WRONLY,
    (void *) NULL
  }, {
    "Pwrite does not affect shared file pos (write), O_RDWR | O_APPEND",
    PwriteDoesNotAffectWritePos,
    O_RDWR | O_APPEND,
    (void *) 1
  }, {
    "Pwrite does not affect shared file pos (write), O_WRONLY | O_APPEND",
    PwriteDoesNotAffectWritePos,
    O_WRONLY | O_APPEND,
    (void *) 1
  }, {
    "PreadObeysOffsetAndDoesNotAffectReadPtr, O_RDONLY",
    PreadObeysOffsetAndDoesNotAffectReadPtr,
    O_RDONLY,
    (void *) NULL
  }, {
    "PreadObeysOffsetAndDoesNotAffectReadPtr, O_RDWR",
    PreadObeysOffsetAndDoesNotAffectReadPtr,
    O_RDWR,
    (void *) NULL
  }, {
    "PreadObeysOffsetAndDoesNotAffectReadPtr, O_RDWR | O_APPEND",
    PreadObeysOffsetAndDoesNotAffectReadPtr,
    O_RDWR | O_APPEND,
    (void *) NULL
  },
};

int main(int ac, char **av) {
  int opt;
  char test_file_name[PATH_MAX];
  size_t ix;
  int error_occurred = 0;
  int wr_fd;
  int rd_fd;

  while (-1 != (opt = getopt(ac, av, "t:"))) {
    switch (opt) {
      case 't':
        g_dirname = optarg;
        break;
      default:
        fprintf(stderr, "Usage: pwrite_test [-t temp_dir]\n");
        return 1;
    }
  }
  for (ix = 0; ix < sizeof g_test_cases / sizeof g_test_cases[0]; ++ix) {
    printf("%s\n", g_test_cases[ix].test_name);
    snprintf(test_file_name, sizeof test_file_name,
             "%s/absalom%zu.txt", g_dirname, ix);
    /*
     * For the case of pwrite() with O_APPEND, SFI NaCl tries to match
     * the POSIX/Mac behaviour, which differs from Linux's behaviour.
     * SFI NaCl contains a workaround to implement the POSIX/Mac
     * behaviour on Linux.  Non-SFI NaCl currently does not implement
     * this workaround, so the tests for POSIX/Mac behaviour don't
     * pass.
     */
    if (NONSFI_MODE && (g_test_cases[ix].wr_fd_open_flag & O_APPEND)) {
      printf("Skipped\n");
      continue;
    }
    InitializeFile(test_file_name, &g_test_cases[ix], &wr_fd, &rd_fd);
    if ((*g_test_cases[ix].test_func)(wr_fd, rd_fd,
                                      g_test_cases[ix].test_specifics)) {
      printf("Failed test %zu: %s\n", ix, g_test_cases[ix].test_name);
      error_occurred = 1;
    } else {
      printf("OK\n");
    }
    (void) close(wr_fd);
    (void) close(rd_fd);
  }
  printf("%s\n", error_occurred ? "FAILED" : "PASSED");
  return error_occurred;
}
