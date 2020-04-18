/**************************************************************************
 *
 * Copyright 2009-2010 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <float.h>

#include "util/u_half.h"
#include "util/u_format.h"
#include "util/u_format_tests.h"
#include "util/u_format_s3tc.h"


static boolean
compare_float(float x, float y)
{
   float error = y - x;

   if (error < 0.0f)
      error = -error;

   if (error > FLT_EPSILON) {
      return FALSE;
   }

   return TRUE;
}


static void
print_packed(const struct util_format_description *format_desc,
             const char *prefix,
             const uint8_t *packed,
             const char *suffix)
{
   unsigned i;
   const char *sep = "";

   printf("%s", prefix);
   for (i = 0; i < format_desc->block.bits/8; ++i) {
      printf("%s%02x", sep, packed[i]);
      sep = " ";
   }
   printf("%s", suffix);
   fflush(stdout);
}


static void
print_unpacked_rgba_doubl(const struct util_format_description *format_desc,
                     const char *prefix,
                     const double unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4],
                     const char *suffix)
{
   unsigned i, j;
   const char *sep = "";

   printf("%s", prefix);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         printf("%s{%f, %f, %f, %f}", sep, unpacked[i][j][0], unpacked[i][j][1], unpacked[i][j][2], unpacked[i][j][3]);
         sep = ", ";
      }
      sep = ",\n";
   }
   printf("%s", suffix);
   fflush(stdout);
}


static void
print_unpacked_rgba_float(const struct util_format_description *format_desc,
                     const char *prefix,
                     float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4],
                     const char *suffix)
{
   unsigned i, j;
   const char *sep = "";

   printf("%s", prefix);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         printf("%s{%f, %f, %f, %f}", sep, unpacked[i][j][0], unpacked[i][j][1], unpacked[i][j][2], unpacked[i][j][3]);
         sep = ", ";
      }
      sep = ",\n";
   }
   printf("%s", suffix);
   fflush(stdout);
}


static void
print_unpacked_rgba_8unorm(const struct util_format_description *format_desc,
                      const char *prefix,
                      uint8_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4],
                      const char *suffix)
{
   unsigned i, j;
   const char *sep = "";

   printf("%s", prefix);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         printf("%s{0x%02x, 0x%02x, 0x%02x, 0x%02x}", sep, unpacked[i][j][0], unpacked[i][j][1], unpacked[i][j][2], unpacked[i][j][3]);
         sep = ", ";
      }
   }
   printf("%s", suffix);
   fflush(stdout);
}


static void
print_unpacked_z_float(const struct util_format_description *format_desc,
                       const char *prefix,
                       float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH],
                       const char *suffix)
{
   unsigned i, j;
   const char *sep = "";

   printf("%s", prefix);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         printf("%s%f", sep, unpacked[i][j]);
         sep = ", ";
      }
      sep = ",\n";
   }
   printf("%s", suffix);
   fflush(stdout);
}


static void
print_unpacked_z_32unorm(const struct util_format_description *format_desc,
                         const char *prefix,
                         uint32_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH],
                         const char *suffix)
{
   unsigned i, j;
   const char *sep = "";

   printf("%s", prefix);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         printf("%s0x%08x", sep, unpacked[i][j]);
         sep = ", ";
      }
   }
   printf("%s", suffix);
   fflush(stdout);
}


static void
print_unpacked_s_8uint(const struct util_format_description *format_desc,
                       const char *prefix,
                       uint8_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH],
                       const char *suffix)
{
   unsigned i, j;
   const char *sep = "";

   printf("%s", prefix);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         printf("%s0x%02x", sep, unpacked[i][j]);
         sep = ", ";
      }
   }
   printf("%s", suffix);
   fflush(stdout);
}


static boolean
test_format_fetch_rgba_float(const struct util_format_description *format_desc,
                             const struct util_format_test_case *test)
{
   float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4] = { { { 0 } } };
   unsigned i, j, k;
   boolean success;

   success = TRUE;
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         format_desc->fetch_rgba_float(unpacked[i][j], test->packed, j, i);
         for (k = 0; k < 4; ++k) {
            if (!compare_float(test->unpacked[i][j][k], unpacked[i][j][k])) {
               success = FALSE;
            }
         }
      }
   }

   if (!success) {
      print_unpacked_rgba_float(format_desc, "FAILED: ", unpacked, " obtained\n");
      print_unpacked_rgba_doubl(format_desc, "        ", test->unpacked, " expected\n");
   }

   return success;
}


static boolean
test_format_unpack_rgba_float(const struct util_format_description *format_desc,
                              const struct util_format_test_case *test)
{
   float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4] = { { { 0 } } };
   unsigned i, j, k;
   boolean success;

   format_desc->unpack_rgba_float(&unpacked[0][0][0], sizeof unpacked[0],
                             test->packed, 0,
                             format_desc->block.width, format_desc->block.height);

   success = TRUE;
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         for (k = 0; k < 4; ++k) {
            if (!compare_float(test->unpacked[i][j][k], unpacked[i][j][k])) {
               success = FALSE;
            }
         }
      }
   }

   if (!success) {
      print_unpacked_rgba_float(format_desc, "FAILED: ", unpacked, " obtained\n");
      print_unpacked_rgba_doubl(format_desc, "        ", test->unpacked, " expected\n");
   }

   return success;
}


static boolean
test_format_pack_rgba_float(const struct util_format_description *format_desc,
                            const struct util_format_test_case *test)
{
   float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4];
   uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   unsigned i, j, k;
   boolean success;

   if (test->format == PIPE_FORMAT_DXT1_RGBA) {
      /*
       * Skip S3TC as packed representation is not canonical.
       *
       * TODO: Do a round trip conversion.
       */
      return TRUE;
   }

   memset(packed, 0, sizeof packed);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         for (k = 0; k < 4; ++k) {
            unpacked[i][j][k] = (float) test->unpacked[i][j][k];
         }
      }
   }

   format_desc->pack_rgba_float(packed, 0,
                           &unpacked[0][0][0], sizeof unpacked[0],
                           format_desc->block.width, format_desc->block.height);

   success = TRUE;
   for (i = 0; i < format_desc->block.bits/8; ++i) {
      if ((test->packed[i] & test->mask[i]) != (packed[i] & test->mask[i]))
         success = FALSE;
   }

   /* Ignore NaN */
   if (util_is_double_nan(test->unpacked[0][0][0]))
      success = TRUE;

   if (!success) {
      print_packed(format_desc, "FAILED: ", packed, " obtained\n");
      print_packed(format_desc, "        ", test->packed, " expected\n");
   }

   return success;
}


static boolean
convert_float_to_8unorm(uint8_t *dst, const double *src)
{
   unsigned i;
   boolean accurate = TRUE;

   for (i = 0; i < UTIL_FORMAT_MAX_UNPACKED_HEIGHT*UTIL_FORMAT_MAX_UNPACKED_WIDTH*4; ++i) {
      if (src[i] < 0.0) {
         accurate = FALSE;
         dst[i] = 0;
      }
      else if (src[i] > 1.0) {
         accurate = FALSE;
         dst[i] = 255;
      }
      else {
         dst[i] = src[i] * 255.0;
      }
   }

   return accurate;
}


static boolean
test_format_unpack_rgba_8unorm(const struct util_format_description *format_desc,
                               const struct util_format_test_case *test)
{
   uint8_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4] = { { { 0 } } };
   uint8_t expected[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4] = { { { 0 } } };
   unsigned i, j, k;
   boolean success;

   format_desc->unpack_rgba_8unorm(&unpacked[0][0][0], sizeof unpacked[0],
                              test->packed, 0,
                              format_desc->block.width, format_desc->block.height);

   convert_float_to_8unorm(&expected[0][0][0], &test->unpacked[0][0][0]);

   success = TRUE;
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         for (k = 0; k < 4; ++k) {
            if (expected[i][j][k] != unpacked[i][j][k]) {
               success = FALSE;
            }
         }
      }
   }

   /* Ignore NaN */
   if (util_is_double_nan(test->unpacked[0][0][0]))
      success = TRUE;

   if (!success) {
      print_unpacked_rgba_8unorm(format_desc, "FAILED: ", unpacked, " obtained\n");
      print_unpacked_rgba_8unorm(format_desc, "        ", expected, " expected\n");
   }

   return success;
}


static boolean
test_format_pack_rgba_8unorm(const struct util_format_description *format_desc,
                             const struct util_format_test_case *test)
{
   uint8_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4];
   uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   unsigned i;
   boolean success;

   if (test->format == PIPE_FORMAT_DXT1_RGBA) {
      /*
       * Skip S3TC as packed representation is not canonical.
       *
       * TODO: Do a round trip conversion.
       */
      return TRUE;
   }

   if (!convert_float_to_8unorm(&unpacked[0][0][0], &test->unpacked[0][0][0])) {
      /*
       * Skip test cases which cannot be represented by four unorm bytes.
       */
      return TRUE;
   }

   memset(packed, 0, sizeof packed);

   format_desc->pack_rgba_8unorm(packed, 0,
                            &unpacked[0][0][0], sizeof unpacked[0],
                            format_desc->block.width, format_desc->block.height);

   success = TRUE;
   for (i = 0; i < format_desc->block.bits/8; ++i)
      if ((test->packed[i] & test->mask[i]) != (packed[i] & test->mask[i]))
         success = FALSE;

   /* Ignore NaN */
   if (util_is_double_nan(test->unpacked[0][0][0]))
      success = TRUE;

   /* Ignore failure cases due to unorm8 format */
   if (test->unpacked[0][0][0] > 1.0f || test->unpacked[0][0][0] < 0.0f)
      success = TRUE;

   /* Multiple of 255 */
   if ((test->unpacked[0][0][0] * 255.0) != (int)(test->unpacked[0][0][0] * 255.0))
      success = TRUE;

   if (!success) {
      print_packed(format_desc, "FAILED: ", packed, " obtained\n");
      print_packed(format_desc, "        ", test->packed, " expected\n");
   }

   return success;
}


static boolean
test_format_unpack_z_float(const struct util_format_description *format_desc,
                              const struct util_format_test_case *test)
{
   float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH] = { { 0 } };
   unsigned i, j;
   boolean success;

   format_desc->unpack_z_float(&unpacked[0][0], sizeof unpacked[0],
                               test->packed, 0,
                               format_desc->block.width, format_desc->block.height);

   success = TRUE;
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         if (!compare_float(test->unpacked[i][j][0], unpacked[i][j])) {
            success = FALSE;
         }
      }
   }

   if (!success) {
      print_unpacked_z_float(format_desc, "FAILED: ", unpacked, " obtained\n");
      print_unpacked_rgba_doubl(format_desc, "        ", test->unpacked, " expected\n");
   }

   return success;
}


static boolean
test_format_pack_z_float(const struct util_format_description *format_desc,
                            const struct util_format_test_case *test)
{
   float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH];
   uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   unsigned i, j;
   boolean success;

   memset(packed, 0, sizeof packed);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         unpacked[i][j] = (float) test->unpacked[i][j][0];
         if (test->unpacked[i][j][1]) {
            return TRUE;
         }
      }
   }

   format_desc->pack_z_float(packed, 0,
                             &unpacked[0][0], sizeof unpacked[0],
                             format_desc->block.width, format_desc->block.height);

   success = TRUE;
   for (i = 0; i < format_desc->block.bits/8; ++i)
      if ((test->packed[i] & test->mask[i]) != (packed[i] & test->mask[i]))
         success = FALSE;

   if (!success) {
      print_packed(format_desc, "FAILED: ", packed, " obtained\n");
      print_packed(format_desc, "        ", test->packed, " expected\n");
   }

   return success;
}


static boolean
test_format_unpack_z_32unorm(const struct util_format_description *format_desc,
                               const struct util_format_test_case *test)
{
   uint32_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH] = { { 0 } };
   uint32_t expected[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH] = { { 0 } };
   unsigned i, j;
   boolean success;

   format_desc->unpack_z_32unorm(&unpacked[0][0], sizeof unpacked[0],
                                 test->packed, 0,
                                 format_desc->block.width, format_desc->block.height);

   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         expected[i][j] = test->unpacked[i][j][0] * 0xffffffff;
      }
   }

   success = TRUE;
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         if (expected[i][j] != unpacked[i][j]) {
            success = FALSE;
         }
      }
   }

   if (!success) {
      print_unpacked_z_32unorm(format_desc, "FAILED: ", unpacked, " obtained\n");
      print_unpacked_z_32unorm(format_desc, "        ", expected, " expected\n");
   }

   return success;
}


static boolean
test_format_pack_z_32unorm(const struct util_format_description *format_desc,
                             const struct util_format_test_case *test)
{
   uint32_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH];
   uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   unsigned i, j;
   boolean success;

   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         unpacked[i][j] = test->unpacked[i][j][0] * 0xffffffff;
         if (test->unpacked[i][j][1]) {
            return TRUE;
         }
      }
   }

   memset(packed, 0, sizeof packed);

   format_desc->pack_z_32unorm(packed, 0,
                               &unpacked[0][0], sizeof unpacked[0],
                               format_desc->block.width, format_desc->block.height);

   success = TRUE;
   for (i = 0; i < format_desc->block.bits/8; ++i)
      if ((test->packed[i] & test->mask[i]) != (packed[i] & test->mask[i]))
         success = FALSE;

   if (!success) {
      print_packed(format_desc, "FAILED: ", packed, " obtained\n");
      print_packed(format_desc, "        ", test->packed, " expected\n");
   }

   return success;
}


static boolean
test_format_unpack_s_8uint(const struct util_format_description *format_desc,
                               const struct util_format_test_case *test)
{
   uint8_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH] = { { 0 } };
   uint8_t expected[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH] = { { 0 } };
   unsigned i, j;
   boolean success;

   format_desc->unpack_s_8uint(&unpacked[0][0], sizeof unpacked[0],
                                  test->packed, 0,
                                  format_desc->block.width, format_desc->block.height);

   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         expected[i][j] = test->unpacked[i][j][1];
      }
   }

   success = TRUE;
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         if (expected[i][j] != unpacked[i][j]) {
            success = FALSE;
         }
      }
   }

   if (!success) {
      print_unpacked_s_8uint(format_desc, "FAILED: ", unpacked, " obtained\n");
      print_unpacked_s_8uint(format_desc, "        ", expected, " expected\n");
   }

   return success;
}


static boolean
test_format_pack_s_8uint(const struct util_format_description *format_desc,
                             const struct util_format_test_case *test)
{
   uint8_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH];
   uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   unsigned i, j;
   boolean success;

   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         unpacked[i][j] = test->unpacked[i][j][1];
         if (test->unpacked[i][j][0]) {
            return TRUE;
         }
      }
   }

   memset(packed, 0, sizeof packed);

   format_desc->pack_s_8uint(packed, 0,
                                &unpacked[0][0], sizeof unpacked[0],
                                format_desc->block.width, format_desc->block.height);

   success = TRUE;
   for (i = 0; i < format_desc->block.bits/8; ++i)
      if ((test->packed[i] & test->mask[i]) != (packed[i] & test->mask[i]))
         success = FALSE;

   if (!success) {
      print_packed(format_desc, "FAILED: ", packed, " obtained\n");
      print_packed(format_desc, "        ", test->packed, " expected\n");
   }

   return success;
}


typedef boolean
(*test_func_t)(const struct util_format_description *format_desc,
               const struct util_format_test_case *test);


static boolean
test_one_func(const struct util_format_description *format_desc,
              test_func_t func,
              const char *suffix)
{
   unsigned i;
   boolean success = TRUE;

   printf("Testing util_format_%s_%s ...\n",
          format_desc->short_name, suffix);
   fflush(stdout);

   for (i = 0; i < util_format_nr_test_cases; ++i) {
      const struct util_format_test_case *test = &util_format_test_cases[i];

      if (test->format == format_desc->format) {
         if (!func(format_desc, &util_format_test_cases[i])) {
           success = FALSE;
         }
      }
   }

   return success;
}


static boolean
test_all(void)
{
   enum pipe_format format;
   boolean success = TRUE;

   for (format = 1; format < PIPE_FORMAT_COUNT; ++format) {
      const struct util_format_description *format_desc;

      format_desc = util_format_description(format);
      if (!format_desc) {
         continue;
      }

      if (format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC &&
          !util_format_s3tc_enabled) {
         continue;
      }

#     define TEST_ONE_FUNC(name) \
      if (format_desc->name) { \
         if (!test_one_func(format_desc, &test_format_##name, #name)) { \
           success = FALSE; \
         } \
      }

      TEST_ONE_FUNC(fetch_rgba_float);
      TEST_ONE_FUNC(pack_rgba_float);
      TEST_ONE_FUNC(unpack_rgba_float);
      TEST_ONE_FUNC(pack_rgba_8unorm);
      TEST_ONE_FUNC(unpack_rgba_8unorm);

      TEST_ONE_FUNC(unpack_z_32unorm);
      TEST_ONE_FUNC(pack_z_32unorm);
      TEST_ONE_FUNC(unpack_z_float);
      TEST_ONE_FUNC(pack_z_float);
      TEST_ONE_FUNC(unpack_s_8uint);
      TEST_ONE_FUNC(pack_s_8uint);

#     undef TEST_ONE_FUNC
   }

   return success;
}


int main(int argc, char **argv)
{
   boolean success;

   util_format_s3tc_init();

   success = test_all();

   return success ? 0 : 1;
}
