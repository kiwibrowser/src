/* Copyright (C) 2011 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <error.h>

#include "md5.h"
#include "sha1.h"

static const struct expected
{
  const char *sample;
  const char *md5_expected;
  const char *sha1_expected;
} tests[] =
  {
    {
      "abc",
      "\x90\x01\x50\x98\x3c\xd2\x4f\xb0\xd6\x96\x3f\x7d\x28\xe1\x7f\x72",
      "\xa9\x99\x3e\x36\x47\x06\x81\x6a\xba\x3e"
      "\x25\x71\x78\x50\xc2\x6c\x9c\xd0\xd8\x9d"
    },
    {
      "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
      "\x82\x15\xef\x07\x96\xa2\x0b\xca\xaa\xe1\x16\xd3\x87\x6c\x66\x4a",
      "\x84\x98\x3e\x44\x1c\x3b\xd2\x6e\xba\xae"
      "\x4a\xa1\xf9\x51\x29\xe5\xe5\x46\x70\xf1"
    },
    {
      "\0a",
      "\x77\x07\xd6\xae\x4e\x02\x7c\x70\xee\xa2\xa9\x35\xc2\x29\x6f\x21",
      "\x34\xaa\x97\x3c\xd4\xc4\xda\xa4\xf6\x1e"
      "\xeb\x2b\xdb\xad\x27\x31\x65\x34\x01\x6f",
    },
    {
      "When in the Course of human events it becomes necessary",
      "\x62\x6b\x5e\x22\xcd\x3d\x02\xea\x07\xde\xd4\x50\x62\x3d\xb9\x96",
      "\x66\xc3\xc6\x8d\x62\x91\xc5\x1e\x63\x0c"
      "\x85\xc8\x6c\xc4\x4b\x3a\x79\x3e\x07\x28",
    },
  };
#define NTESTS (sizeof tests / sizeof tests[0])

#define md5_size	16
#define sha1_size	20

static const char md5_expected[] =
  {
  };

static const char sha1_expected[] =
  {
  };

#define TEST_HASH(ALGO, I)						      \
  {									      \
    struct ALGO##_ctx ctx;						      \
    uint32_t result_buffer[(ALGO##_size + 3) / 4];			      \
    ALGO##_init_ctx (&ctx);						      \
    if (tests[I].sample[0] == '\0')					      \
      {									      \
	char input_buffer[1000];					      \
	memset (input_buffer, tests[I].sample[1], sizeof input_buffer);	      \
	for (int rept = 0; rept < 1000; ++rept)				      \
	  ALGO##_process_bytes (input_buffer, sizeof input_buffer, &ctx);     \
      }									      \
    else								      \
      ALGO##_process_bytes (tests[I].sample, strlen (tests[I].sample), &ctx); \
    char *result = ALGO##_finish_ctx (&ctx, result_buffer);		      \
    if (result != (void *) result_buffer				      \
	|| memcmp (result, tests[I].ALGO##_expected, ALGO##_size) != 0)	      \
      error (0, 0, #ALGO " test %zu failed", 1 + I);			      \
  }

int
main (void)
{
  for (size_t i = 0; i < NTESTS; ++i)
    {
      TEST_HASH (md5, i);
      TEST_HASH (sha1, i);
    }
  return error_message_count;
}
