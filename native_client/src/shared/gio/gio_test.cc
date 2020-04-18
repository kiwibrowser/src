/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/shared/gio/gio.h"
#include "native_client/src/shared/gio/gio_test_base.h"
#include "gtest/gtest.h"

namespace {

// Premade files set from argv. These are only read (not written to).
// Must be a file with contents ['A'+0, 'A'+1, ..., 'A'+31]
char* g_premade_textfile = NULL;
// Must be a file with contents [0, 1, 2, ..., 31]
char* g_premade_binfile = NULL;
char* g_temp_file = NULL;

/** Parse arguments for input files and checks that there are enough args. */
void parse_args(int argc, char *argv[]) {
  int opt;
  while (-1 != (opt = getopt(argc, argv, "t:x:b:"))) {
    switch (opt) {
      default:
        printf("Unknown commandline option %c", opt);
        FAIL();
      case 'x':
        g_premade_textfile = optarg;
        break;
      case 'b':
        g_premade_binfile = optarg;
        break;
      case 't':
        g_temp_file = optarg;
        break;
    }
  }

  printf("Testing with textfile %s\n", g_premade_textfile);
  printf("Testing with binfile %s\n", g_premade_binfile);
  printf("Testing with temp file %s\n", g_temp_file);

  ASSERT_NE(reinterpret_cast<char*>(NULL), g_temp_file);
  ASSERT_NE(reinterpret_cast<char*>(NULL), g_premade_textfile);
  ASSERT_NE(reinterpret_cast<char*>(NULL), g_premade_binfile);
}


TEST(GioTest, ReadTest) {
  struct GioFile my_file;
  int ret_code;

  ret_code = GioFileCtor(&my_file, g_premade_textfile, "r");
  EXPECT_EQ(1, ret_code);
  GioReadTestWithOffset(&my_file.base, 'A');
  GioCloseTest(&my_file.base);

  ret_code = GioFileCtor(&my_file, g_premade_binfile, "rb");
  EXPECT_EQ(1, ret_code);
  GioReadTestWithOffset(&my_file.base, 0);
  GioCloseTest(&my_file.base);
}


void MakeTempFile(struct GioFile* my_file) {
  int ret_code = GioFileCtor(my_file, g_temp_file, "w+b");
  EXPECT_EQ(1, ret_code);
}

void CloseAndCleanTempFile(struct GioFile* gf) {
  GioCloseTest(&gf->base);

  // Since we pass in the name of the temp file from Scons, and Scons does not
  // know when it is opened/closed to delete the temp file. Delete it here.
  ASSERT_EQ(0, remove(g_temp_file));
}

TEST(GioTest, WriteTest) {
  struct GioFile my_file;

  MakeTempFile(&my_file);
  GioWriteTest(&my_file.base, false);
  CloseAndCleanTempFile(&my_file);
}

TEST(GioTest, SeekTest) {
  struct GioFile my_file;
  int ret_code;

  ret_code = GioFileCtor(&my_file, g_premade_textfile, "r");
  EXPECT_EQ(1, ret_code);
  GioSeekTestWithOffset(&my_file.base, 'A', false);
  GioCloseTest(&my_file.base);

  ret_code = GioFileCtor(&my_file, g_premade_binfile, "rb");
  EXPECT_EQ(1, ret_code);
  GioSeekTestWithOffset(&my_file.base, 0, false);
  GioCloseTest(&my_file.base);
}

}  // namespace

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  parse_args(argc, argv);
  return RUN_ALL_TESTS();
}
