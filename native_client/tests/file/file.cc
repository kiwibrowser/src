/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PRINT_HEADER 0
#define TEXT_LINE_SIZE 1024


/*
 * global text for text file generation and
 * testing.  table is NULL terminated.
 */

static const char *gText[] = {
  "Text output test\n",
  "This is test text for text IO\n",
  "Duplicate line\n",
  "Duplicate line\n",
  "Same as above\n",
  "0123456789~!@#$%^&*()_+\n",
  NULL
};



/*
 * function failed(testname, msg)
 *   print failure message and exit with a return code of -1
 */

bool failed(const char *testname, const char *msg) {
  printf("TEST FAILED: %s: %s\n", testname, msg);
  return false;
}


/*
 * function passed(testname, msg)
 *   print success message
 */

bool passed(const char *testname, const char *msg) {
  printf("TEST PASSED: %s: %s\n", testname, msg);
  return true;
}


/*
 * function createBinaryTestData(testname)
 *   Create binary 'testdata256' file.
 *   This file contains bytes 0..255
 *   It is exactly 256 bytes in size.
 * returns true if test passed
 */

bool fopen_createBinaryFile(const char *testname) {

  // create and write 0..255 into binary testdata file

  FILE *f = fopen("testdata256", "wb");
  if (NULL == f) {
    return failed(testname, "failed on fopen()");
  }
  for (int i = 0; i < 256; ++i) {
    unsigned char c = (unsigned char)i;
    size_t j = fwrite(&c, 1, 1, f);
    if (1 != j) {
      fclose(f);
      return failed(testname, "can't fwrite(&c, 1, 1, f)");
    }
  }
  fclose(f);
  return passed(testname, "fopen_createBinaryFile()");
}


/*
 * function fread_bytes(testname, filename)
 *   Read and compare expected bytes from binary filename.
 *   Read is done byte at a time.
 *   The testdata256 file is exactly 256 bytes in length.
 *   We expect to find 0, 1, 2... 253, 254, 255.
 * returns true if test passed
 */

bool fread_bytes(const char *testname, const char *filename) {

  FILE *f = fopen(filename, "rb");
  if (NULL == f) {
    return failed(testname, "failed on fopen()");
  }
  for (int i = 0; i < 256; ++i) {
    unsigned char c;
    size_t j = fread(&c, 1, 1, f);
    if (1 != j) {
      fclose(f);
      return failed(testname, "couldn't fread(&c, 1, 1, f)");
    }
    if (c != (unsigned char)i) {
      fclose(f);
      return failed(testname, "read bytes don't match");
    }
  }
  fclose(f);
  return passed(testname, "fread(&c, 1, 1, f) x 256");
}


/*
 * function fopen_fail(testname)
 *   Attempt to fopen a file that isn't there.
 *   This should fail(!)
 * returns true if test passed (fopen failed)
 */

bool fopen_fail(const char *testname) {

  FILE *f = fopen("noexist.abc", "rb");

  if (NULL != f) {
    fclose(f);
    return failed(testname, "fopen() succeeded on non-existant file!");
  }
  if (errno != ENOENT) {
    return failed(testname, "fopen() wrong error on non-existant file!");
  }
  return passed(testname, "fopen_fail() on non-existant file");
}


/*
 * function fread_256x1_block(testname)
 *   read and compare expected bytes from binary file
 *   read as one 256x1 chunk
 *   The testdata256 file is exactly 256 bytes in length.
 *   We expect to find 0, 1, 2... 253, 254, 255.
 * returns true if test passed
 */

bool fread_256x1_block(const char *testname) {

  FILE *f = fopen("testdata256", "rb");
  if (NULL == f) {
    return failed(testname, "failed on fopen()");
  }
  unsigned char c[256];
  memset(c, 0, sizeof(unsigned char) * 256);
  // read as 256x1 chunk
  size_t j = fread(&c, 256, 1, f);
  fclose(f);
  if (1 != j) {
    return failed(testname, "couldn't fread(&c, 256, 1, f)");
  }
  for (int i = 0; i < 256; ++i) {
    if (c[i] != (unsigned char)i) {
      return failed(testname, "read bytes don't match");
    }
  }
  return passed(testname, "fread_256x1_block()");
}


/*
 * function fread_1x256_block(testname)
 *   read and compare expected bytes from binary file
 *   read as one 1x256 chunks
 *   The testdata256 file is exactly 256 bytes in length.
 *   We expect to find 0, 1, 2... 253, 254, 255.
 * returns true if test passed
 */

bool fread_1x256_block(const char *testname) {

  FILE *f = fopen("testdata256", "rb");
  if (NULL == f) {
    return failed(testname, "failed on fopen()");
  }
  unsigned char c[256];
  memset(c, 0, sizeof(unsigned char) * 256);
  // read as 1x256 chunk
  size_t j = fread(&c, 1, 256, f);
  fclose(f);
  if (256 != j) {
    return failed(testname, "couldn't fread(&c, 1, 256, f)");
  }
  for (int i = 0; i < 256; ++i) {
    if (c[i] != (unsigned char)i) {
      return failed(testname, "read bytes don't match");
    }
  }
  return passed(testname, "fread_1x256_block()");
}


/*
 * function fopen_createTextFile(testname)
 *   create and write text file
 *   Uses 'gText' NULL terminated array to populate text file
 * returns true if test passed
 */

bool fopen_createTextFile(const char *testname) {

  FILE *f = fopen("testdata.txt", "wt");
  if (NULL == f) {
    return failed(testname, "fopen() testdata.txt (wt)");
  }

  for (int i = 0; NULL != gText[i]; ++i) {
    int numWritten = fprintf(f, "%s", gText[i]);
    if (numWritten < 0) {
      fclose(f);
      return failed(testname, "fprintf() returned a negative value.");
    }
  }
  fclose(f);
  return passed(testname, "fopen_createTextFile()");
}


/*
 * function fgets_readText(testname)
 *   Read and compare expected text from test file.
 *   The test file should contain the same text as
 *   the 'gText' NULL terminated array.
 * returns true if test passed
 */

bool fgets_readText(const char *testname) {

  FILE *f = fopen("testdata.txt", "rt");
  if (NULL == f) {
    return failed(testname, "Unable to open file");
  }
  char buffer[TEXT_LINE_SIZE];
  int index;
  memset(buffer, 0, sizeof(char) * TEXT_LINE_SIZE);
  for (index = 0; NULL != fgets(buffer, TEXT_LINE_SIZE - 1, f); ++index) {
    if (NULL == gText[index]) {
      fclose(f);
      return failed(testname, "unexpected mismatch");
    }
    if (0 != strcmp(buffer, gText[index])) {
      fclose(f);
      return failed(testname, "read text does not match");
    }
  }
  fclose(f);
  if (NULL != gText[index]) {
    return failed(testname, "unexpected eof() encountered");
  }
  return passed(testname, "fgets_readText()");
}


/*
 * function fseek_filesize256(testname)
 *   Use fseek to determine expected filesize of test file filedata256.
 *   It is expected to be exactly 256 bytes in size.
 * returns true if test passed
 */

bool fseek_filesize256(const char *testname) {

  FILE *f = fopen("testdata256", "rb");
  if (NULL == f) {
    return failed(testname, "fopen failed");
  }
  int x = fseek(f, 0, SEEK_END);
  if (0 == x) {
    int p = ftell(f);
    if (256 != p) {
      fclose(f);
      return failed(testname, "ftell mismatch");
    }
    fclose(f);
  } else {
    fclose(f);
    return failed(testname, "fseek failed");
  }
  return passed(testname, "fseek_filesize256()");
}


/*
 * function fseek_simple_testdata256(testname)
 *   Verify a simple combination of fseek & fread.
 *   The testdata256 file is exactly 256 bytes in length,
 *   and consists of 0, 1, 2... 253, 254, 255.
 *   If we seek to the Nth byte, we expect to find
 *   the value N there.
 * returns true if test passed
 */

bool fseek_simple_testdata256(const char *testname) {

  // test simple file seeking within testdata256

  FILE *f = fopen("testdata256", "rb");
  if (NULL == f) {
    return failed(testname, "fseek_simple_testdata256() could not fopen()");
  }

  // seek to offset 1 from start of file
  int x = fseek(f, 1, SEEK_SET);
  if (x != 0) {
    fclose(f);
    return failed(testname, "fseek(1, SEEK_SET) failed");
  }
  unsigned char c = 0;
  fread(&c, 1, 1, f);
  if (1 != c) {
    fclose(f);
    return failed(testname, "fread(f, 1, 1, &c) at SEEK_SET+1 mismatch");
  }
  x = fseek(f, -2, SEEK_END);
  if (0 != x) {
    fclose(f);
    return failed(testname, "fseek(-1, SEEK_END) failed");
  }
  fread(&c, 1, 1, f);
  if (254 != c) {
    fclose(f);
    return failed(testname, "fread(f, 1, 1, &c) at SEEK_END-1 mismatch");
  }
  x = fseek(f, 2, SEEK_SET);
  if (0 != x) {
    fclose(f);
    return failed(testname, "fseek(2, SEEK_SET) failed");
  }
  x = fseek(f, 1, SEEK_CUR);
  if (0 != x) {
    fclose(f);
    return failed(testname, "fseek(1, SEEK_CUR) failed");
  }
  fread(&c, 1, 1, f);
  if (3 != c) {
    fclose(f);
    return failed(testname, "fseek(f, 1, 1, &c) at SEEK_CUR+1 mismatch");
  }
  return passed(testname, "fseek_simple_testdata256()");
}


/*
 * function fgets_filesize(testname)
 *   Determine the length of a text file using fgets & strlen
 *   This value should match the length of gText[]
 * returns true if test passed
 */

bool fgets_filesize(const char *testname) {

  size_t charcount = 0;
  size_t filecharcount = 0;

  // count chars in gText[]
  for (int i = 0; NULL != gText[i]; ++i) {
    charcount += strlen(gText[i]);
  }

  FILE *f = fopen("testdata.txt", "r");
  if (NULL == f) {
    return failed(testname, "fgets_filesize() failed on fopen()");
  }

  char buffer[TEXT_LINE_SIZE];
  memset(buffer, 0, sizeof(char) * TEXT_LINE_SIZE);
  while(NULL != fgets(buffer, TEXT_LINE_SIZE - 1, f)) {
    filecharcount += strlen(buffer);
  }
  fclose(f);

  if (charcount != filecharcount) {
    return failed(testname, "fgets_filesize() mismatch");
  }

  return passed(testname, "fgets_filesize()");
}


/*
 * function fopen_appendBinaryFile(testname)
 *   First create half a binary test file.
 *   Then re-open it for binary append, and add the other half.
 *   The intention is for testdata256 to contain 0, 1, 2... 253, 254, 255.
 * returns true if test passed
 */

bool fopen_appendBinaryFile(const char *testname)
{
  // first half: create a small file
  // write 0..99

  FILE *f = fopen("testdata256", "wb");
  if (NULL == f) {
    return failed(testname, "failed on fopen()\n");
  }

  for (int i = 0; i < 100; ++i) {
    unsigned char c = (unsigned char)i;
    size_t j = fwrite(&c, 1, 1, f);
    if (1 != j) {
      fclose(f);
      return failed(testname, "can't fwrite(&c, 1, 1, f)");
    }
  }
  fclose(f);

  // second half: re-open and append the rest
  // write 99..255

  f = fopen("testdata256", "ab");
  if (NULL == f) {
    return failed(testname, "fopen() testdata256 (ab)");
  }

  for (int i = 100; i < 256; ++i) {
    unsigned char c = (unsigned char)i;
    size_t j = fwrite(&c, 1, 1, f);
    if (1 != j) {
      fclose(f);
      return failed(testname, "can't fwrite(&c, 1, 1, f)");
    }
  }
  fclose(f);

  return passed(testname, "fopen_appendBinaryFile()");
}


/*
 * function fopen_appendTextFile(testname)
 *   First create a text file with first 2 lines.
 *   Then re-open it for append, and add the remaining lines.
 *   The intention is for testdata to contain gText[].
 * returns true if test passed
 */

bool fopen_appendTextFile(const char *testname) {

  // create & write first 2 lines of gText[]

  FILE *f = fopen("testdata.txt", "w");
  int i = 0;
  if (NULL == f) {
    return failed(testname, "fopen() testdata.txt (w+)");
  }

  if ((gText[0]) && (gText[1])) {
    // write first two lines
    fprintf(f, "%s", gText[i++]);
    fprintf(f, "%s", gText[i++]);
  } else {
    failed(testname, "gText table is too small!");
  }
  fclose(f);

  // re-open and append the rest of gText[]

  f = fopen("testdata.txt", "a");
  if (NULL == f) {
    return failed(testname, "fopen() testdata.txt (w+)");
  }
  while (gText[i]) {
    fprintf(f, "%s", gText[i]);
    i++;
  }
  fclose(f);

  return passed(testname, "fopen_appendTextFile()");
}



/*
 * function test*()
 *
 *   Simple tests follow below.  Each test may call one or more
 *   of the functions above.  They all have a boolean return value
 *   to indicate success (all tests passed) or failure (one or more
 *   tests failed)  Order matters - the parent should call
 *   test1() before test2(), and so on.
 */

bool test1()
{
  // test the creation of a binary file
  return fopen_createBinaryFile("test1");
}


bool test2()
{
  // test the creation of a text file
  return fopen_createTextFile("test2");
}


bool test3()
{
  // test reading bytes from binary file
  return fread_bytes("test3", "testdata256");
}


bool test4()
{
  // test reading block from binary file
  return fread_256x1_block("test4");
}


bool test5()
{
  // test reading block from binary file
  return fread_1x256_block("test5");
}


bool test6()
{
  // test reading from text file
  return fgets_readText("test6");
}


bool test7()
{
  // test binary file size
  return fseek_filesize256("test7");
}


bool test8()
{
  // test text file size
  return fgets_filesize("test8");
}


bool test9()
{
  // create binary file twice
  bool ret = true;
  ret &= fopen_createBinaryFile("test7a");
  ret &= fopen_createBinaryFile("test7b");
  return ret;
}


bool test10()
{
  // create text file twice
  bool ret = true;
  ret &= fopen_createTextFile("test8a");
  ret &= fopen_createTextFile("test8b");
  return ret;
}


bool test11()
{
  // verify binary file size again
  return fseek_filesize256("test11");
}


bool test12()
{
  // verify test file size again
  return fgets_filesize("test12");
}


bool test13()
{
  // verify seeks followed by reads
  return fseek_simple_testdata256("test13");
}


bool test14()
{
  // create and then append to binary file
  // then verify contents & filesize
  bool ret = true;
  ret &= fopen_appendBinaryFile("test14a");
  ret &= fread_bytes("test14b", "testdata256");
  ret &= fseek_filesize256("test14c");
  return ret;
}


bool test15()
{
  // create and then append to a text file
  // then verify contents & filesize
  bool ret = true;
  ret &= fopen_appendTextFile("test15a");
  ret &= fgets_readText("test15b");
  ret &= fgets_filesize("test15c");
  return ret;
}


bool test16()
{
  // try a slightly different path
  bool ret = true;
  ret &= fread_bytes("test16", "./testdata256");
  return ret;
}


bool test17()
{
  // try a slightly different path
  bool ret = true;
  ret &= fread_bytes("test17", ".././file/testdata256");
  return ret;
}


bool test18()
{
  bool ret = true;
  ret &= fopen_fail("test18");
  return ret;
}


/*
 * function testSuite()
 *
 *   Run through a complete sequence of file tests.
 *
 * returns true if all tests succeed.  false if one or more fail.
 */

bool testSuite()
{
  bool ret = true;
  // The order of executing these tests matters!
  ret &= test1();
  ret &= test2();
  ret &= test3();
  ret &= test4();
  ret &= test5();
  ret &= test6();
  ret &= test7();
  ret &= test8();
  ret &= test9();
  ret &= test10();
  ret &= test11();
  ret &= test12();
  ret &= test13();
  ret &= test14();
  ret &= test15();
  ret &= test16();
  ret &= test17();
  ret &= test18();
  return ret;
}


/*
 * function remove_testfiles()
 *
 * cleanup and remove all testfiles here.
 * (at the moment, nacl cannot remove files, so this function
 * will do nothing in a nacl nexe.  It will function as expected
 * when the test suite is built as a non-nacl executable.  The
 * NaCl version of the test suite uses a python script to remove
 * the testfiles.)
 */

void remove_testfiles()
{
#if !defined(__native_client__)
  remove("testdata256");
  remove("testdata.txt");
#endif
}


/*
 * main entry point.
 *
 * run all tests and call system exit with appropriate value
 *   0 - success, all tests passed.
 *  -1 - one or more tests failed.
 */

int main(const int argc, const char *argv[])
{
  bool passed;

  // remove test files from previous runs
  remove_testfiles();

  // run the full test suite
  passed = testSuite();

  // remove test files that were created.
  remove_testfiles();

  if (passed) {
    printf("All tests PASSED\n");
    exit(0);
  } else {
    printf("One or more tests FAILED\n");
    exit(-1);
  }
}
