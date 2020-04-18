#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "liblouis.h"
#include "louis.h"
#include "brl_checks.h"

int check_with_mode(
    const char *tableList, const char *str, const char *typeform,
    const char *expected, int mode, int direction);

void
print_int_array(const char *prefix, int *pos_list, int len)
{
  int i;
  printf("%s ", prefix);
  for (i = 0; i < len; i++)
    printf("%d ", pos_list[i]);
  printf("\n");
}

void
print_widechars(widechar * buf, int len)
{
  int i;
  for (i = 0; i < len; i++)
    printf("%c", buf[i]);
}

/* Helper function to convert a typeform string of '0's, '1's, '2's etc.
   to the required format, which is an array of 0s, 1s, 2s, etc.
   For example, "0000011111000" is converted to {0,0,0,0,0,1,1,1,1,1,0,0,0}
   The caller is responsible for freeing the returned array. */
char *
convert_typeform(const char* typeform_string)
{
  int len = strlen(typeform_string);
  char *typeform = malloc(len * sizeof(char));
  int i;
  for (i=0; i<len; i++)
    typeform[i] = typeform_string[i] - '0';
  return typeform;
}

/* Check if a string is translated as expected. Return 0 if the
   translation is as expected and 1 otherwise. */
int
check_translation(const char *tableList, const char *str,
		  const char *typeform, const char *expected)
{
  return check_translation_with_mode(tableList, str, typeform, expected, 0);
}

/* Check if a string is translated as expected. Return 0 if the
   translation is as expected and 1 otherwise. */
int
check_translation_with_mode(const char *tableList, const char *str,
			    const char *typeform, const char *expected, 
			    int mode)
{
    return check_with_mode(tableList, str, typeform, expected, mode, 0);
}

/* Check if a string is backtranslated as expected. Return 0 if the
   backtranslation is as expected and 1 otherwise. */
int
check_backtranslation(const char *tableList, const char *str,
		  const char *typeform, const char *expected)
{
  return check_backtranslation_with_mode(tableList, str, typeform, expected, 0);
}

/* Check if a string is backtranslated as expected. Return 0 if the
   backtranslation is as expected and 1 otherwise. */
int
check_backtranslation_with_mode(const char *tableList, const char *str,
			    const char *typeform, const char *expected, 
			    int mode)
{
    return check_with_mode(tableList, str, typeform, expected, mode, 1);
}

/* direction, 0=forward, otherwise backwards */
int check_with_mode(
    const char *tableList, const char *str, const char *typeform,
    const char *expected, int mode, int direction)
{
  widechar *inbuf;
  widechar *outbuf;
  int inlen;
  int outlen;
  int i, rv = 0;
  int funcStatus = 0;

  char *typeformbuf = NULL;

  int expectedlen = strlen(expected);

  inlen = strlen(str);
  outlen = inlen * 10;
  inbuf = malloc(sizeof(widechar) * inlen);
  outbuf = malloc(sizeof(widechar) * outlen);
  if (typeform != NULL)
    {
      typeformbuf = malloc(outlen);
      memcpy(typeformbuf, typeform, outlen);
    }
  inlen = extParseChars(str, inbuf);
  if (!inlen)
    {
      printf("Cannot parse input string.\n");
      return 1;
    }
  if (direction == 0)
    {
      funcStatus = lou_translate(tableList, inbuf, &inlen, outbuf, &outlen,
		     typeformbuf, NULL, NULL, NULL, NULL, mode);
    } else {
      funcStatus = lou_backTranslate(tableList, inbuf, &inlen, outbuf, &outlen,
		     typeformbuf, NULL, NULL, NULL, NULL, mode);
    }
  if (!funcStatus)
    {
      printf("Translation failed.\n");
      return 1;
    }

  for (i = 0; i < outlen && i < expectedlen && expected[i] == outbuf[i]; i++);
  if (i < outlen || i < expectedlen)
    {
      rv = 1;
      outbuf[outlen] = 0;
      printf("Input: '%s'\n", str);
      printf("Expected: '%s'\n", expected);
      printf("Received: '");
      print_widechars(outbuf, outlen);
      printf("'\n");
      if (i < outlen && i < expectedlen) 
	{
	  printf("Diff: Expected '%c' but recieved '%c' in index %d\n",
		 expected[i], outbuf[i], i);
	}
      else if (i < expectedlen)
	{
	  printf("Diff: Expected '%c' but recieved nothing in index %d\n",
		 expected[i], i);
	}
      else 
	{
	  printf("Diff: Expected nothing but recieved '%c' in index %d\n",
		  outbuf[i], i);
	}
    }

  free(inbuf);
  free(outbuf);
  free(typeformbuf);
  lou_free();
  return rv;
}

int
check_inpos(const char *tableList, const char *str, const int *expected_poslist)
{
  widechar *inbuf;
  widechar *outbuf;
  int *inpos;
  int inlen;
  int outlen;
  int i, rv = 0;

  inlen = strlen(str) * 2;
  outlen = inlen;
  inbuf = malloc(sizeof(widechar) * inlen);
  outbuf = malloc(sizeof(widechar) * outlen);
  inpos = malloc(sizeof(int) * inlen);
  for (i = 0; i < inlen; i++)
    {
      inbuf[i] = str[i];
    }
  lou_translate(tableList, inbuf, &inlen, outbuf, &outlen,
		NULL, NULL, NULL, inpos, NULL, 0);
  for (i = 0; i < outlen; i++)
    {
      if (expected_poslist[i] != inpos[i])
	{
	  rv = 1;
	  printf("Expected %d, recieved %d in index %d\n",
		 expected_poslist[i], inpos[i], i);
	}
    }

  free(inbuf);
  free(outbuf);
  free(inpos);
  lou_free();
  return rv;

}

int
check_outpos(const char *tableList, const char *str, const int *expected_poslist)
{
  widechar *inbuf;
  widechar *outbuf;
  int *inpos, *outpos;
  int origInlen, inlen;
  int outlen;
  int i, rv = 0;

  origInlen = inlen = strlen(str);
  outlen = inlen * 2;
  inbuf = malloc(sizeof(widechar) * inlen);
  outbuf = malloc(sizeof(widechar) * outlen);
  /* outputPos can be affected by inputPos, so pass inputPos as well. */
  inpos = malloc(sizeof(int) * outlen);
  outpos = malloc(sizeof(int) * inlen);
  for (i = 0; i < inlen; i++)
    {
      inbuf[i] = str[i];
    }
  lou_translate(tableList, inbuf, &inlen, outbuf, &outlen,
		NULL, NULL, outpos, inpos, NULL, 0);
  if (inlen != origInlen)
    {
      printf("original inlen %d and returned inlen %d differ\n",
        origInlen, inlen);
    }

  for (i = 0; i < inlen; i++)
    {
      if (expected_poslist[i] != outpos[i])
	{
	  rv = 1;
	  printf("Expected %d, recieved %d in index %d\n",
		 expected_poslist[i], outpos[i], i);
	}
    }

  free(inbuf);
  free(outbuf);
  free(inpos);
  free(outpos);
  lou_free();
  return rv;

}

int
check_cursor_pos(const char *str, const int *expected_pos)
{
  widechar *inbuf;
  widechar *outbuf;
  int *inpos, *outpos;
  int orig_inlen;
  int outlen;
  int cursor_pos;
  int i, rv = 0;

  orig_inlen = strlen(str);
  outlen = orig_inlen;
  inbuf = malloc(sizeof(widechar) * orig_inlen);
  outbuf = malloc(sizeof(widechar) * orig_inlen);
  inpos = malloc(sizeof(int) * orig_inlen);
  outpos = malloc(sizeof(int) * orig_inlen);
  for (i = 0; i < orig_inlen; i++)
    {
      inbuf[i] = str[i];
    }

  for (i = 0; i < orig_inlen; i++)
    {
      int inlen = orig_inlen;
      cursor_pos = i;
      lou_translate(TRANSLATION_TABLE, inbuf, &inlen, outbuf, &outlen,
		    NULL, NULL, NULL, NULL, &cursor_pos, compbrlAtCursor);
      if (expected_pos[i] != cursor_pos)
	{
	  rv = 1;
	  printf("string='%s' cursor=%d ('%c') expected=%d \
recieved=%d ('%c')\n", str, i, str[i], expected_pos[i], cursor_pos, (char) outbuf[cursor_pos]);
	}
    }

  free(inbuf);
  free(outbuf);
  free(inpos);
  free(outpos);
  lou_free();

  return rv;
}

/* Check if a string is hyphenated as expected. Return 0 if the
   hyphenation is as expected and 1 otherwise. */
int
check_hyphenation(const char *tableList, const char *str, const char *expected)
{
  widechar *inbuf;
  char *hyphens;
  int inlen;
  int rv = 0;

  inlen = strlen(str);
  inbuf = malloc(sizeof(widechar) * inlen);
  inlen = extParseChars(str, inbuf);
  if (!inlen)
    {
      printf("Cannot parse input string.\n");
      return 1;
    }
  hyphens = calloc(inlen+1, sizeof(char));

  if (!lou_hyphenate(tableList, inbuf, inlen, hyphens, 0))
    {
      printf("Hyphenation failed.\n");
      return 1;
    }

  if (strcmp(expected, hyphens))
    {
      printf("Input:    '%s'\n", str);
      printf("Expected: '%s'\n", expected);
      printf("Received: '%s'\n", hyphens);
      rv = 1;
    }

  free(inbuf);
  free(hyphens);
  lou_free();
  return rv;

}
