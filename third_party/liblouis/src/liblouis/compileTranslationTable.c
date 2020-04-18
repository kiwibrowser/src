/* liblouis Braille Translation and Back-Translation 
Library

   Based on the Linux screenreader BRLTTY, copyright (C) 1999-2006 by
   The BRLTTY Team

   Copyright (C) 2004, 2005, 2006
   ViewPlus Technologies, Inc. www.viewplus.com
   and
   JJB Software, Inc. www.jjb-software.com
   All rights reserved

   This file is free software; you can redistribute it and/or modify it
   under the terms of the Lesser or Library GNU General Public License 
   as published by the
   Free Software Foundation; either version 3, or (at your option) any
   later version.

   This file is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
   Library GNU General Public License for more details.

   You should have received a copy of the Library GNU General Public 
   License along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Maintained by John J. Boyer john.boyer@jjb-software.com
   */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
//#include <unistd.h>

#include "louis.h"
#include "config.h"

#define QUOTESUB 28		/*Stand-in for double quotes in strings */


/* Contributed by Michel Such <michel.such@free.fr */
#ifdef _WIN32

/* Adapted from BRLTTY code (see sys_progs_wihdows.h) */

#include <shlobj.h>

static void *
reallocWrapper (void *address, size_t size)
{
  if (!(address = realloc (address, size)) && size)
    outOfMemory ();
  return address;
}

static char *
strdupWrapper (const char *string)
{
  char *address = strdup (string);
  if (!address)
    outOfMemory ();
  return address;
}


char *EXPORT_CALL
lou_getProgramPath ()
{
  char *path = NULL;
  HMODULE handle;

  if ((handle = GetModuleHandle (NULL)))
    {
      size_t size = 0X80;
      char *buffer = NULL;

      while (1)
	{
	  buffer = reallocWrapper (buffer, size <<= 1);

	  {
	    DWORD length = GetModuleFileName (handle, buffer, size);

	    if (!length)
	      {
		printf ("GetModuleFileName\n");
		exit (3);
	      }

	    if (length < size)
	      {
		buffer[length] = 0;
		path = strdupWrapper (buffer);

		while (length > 0)
		  if (path[--length] == '\\')
		    break;

		strncpy (path, path, length + 1);
		path[length + 1] = '\0';
		break;
	      }
	  }
	}

      free (buffer);
    }
  else
    {
      printf ("GetModuleHandle\n");
      exit (3);
    }

  return path;
}

#define PATH_SEP ';'
#define DIR_SEP '\\'
#else
#define PATH_SEP ':'
#define DIR_SEP '/'
#endif
/* End of MS contribution */

void
outOfMemory ()
{
  logMessage(LOG_FATAL, "liblouis: Insufficient memory\n");
  exit (3);
}

/* The folowing variables and functions make it possible to specify the 
* path on which all tables for liblouis and all files for liblouisutdml, 
* in their proper directories, will be found.
*/

static char dataPath[MAXSTRING];
static char *dataPathPtr;

char *EXPORT_CALL
lou_setDataPath (char *path)
{
  dataPathPtr = NULL;
  if (path == NULL)
    return NULL;
  strcpy (dataPath, path);
  dataPathPtr = dataPath;
  return dataPathPtr;
}

char *EXPORT_CALL
lou_getDataPath ()
{
  return dataPathPtr;
}

/* End of dataPath code.*/

static int
eqasc2uni (const unsigned char *a, const widechar * b, const int len)
{
  int k;
  for (k = 0; k < len; k++)
    if ((widechar) a[k] != b[k])
      return 0;
  return 1;
}

typedef struct
{
  widechar length;
  widechar chars[MAXSTRING];
}
CharsString;

static int errorCount;
static int warningCount;
static TranslationTableHeader *table;
static TranslationTableOffset tableSize;
static TranslationTableOffset tableUsed;

typedef struct
{
  void *next;
  void *table;
  int tableListLength;
  char tableList[1];
} ChainEntry;
static ChainEntry *tableChain = NULL;

static const char *characterClassNames[] = {
  "space",
  "letter",
  "digit",
  "punctuation",
  "uppercase",
  "lowercase",
  "math",
  "sign",
  "litdigit",
  NULL
};

struct CharacterClass
{
  struct CharacterClass *next;
  TranslationTableCharacterAttributes attribute;
  widechar length;
  widechar name[1];
};
static struct CharacterClass *characterClasses;
static TranslationTableCharacterAttributes characterClassAttribute;

static const char *opcodeNames[CTO_None] = {
  "include",
  "locale",
  "undefined",
  "capsign",
  "begcaps",
  "lenbegcaps",
  "endcaps",
  "firstwordcaps",
  "lastwordbeforecaps",
  "lastwordaftercaps",
  "lencapsphrase",
  "letsign",
  "noletsignbefore",
  "noletsign",
  "noletsignafter",
  "numsign",
  "firstwordital",
  "italsign",
  "lastworditalbefore",
  "lastworditalafter",
  "begital",
  "firstletterital",
  "endital",
  "lastletterital",
  "singleletterital",
  "italword",
  "lenitalphrase",
  "firstwordbold",
  "boldsign",
  "lastwordboldbefore",
  "lastwordboldafter",
  "begbold",
  "firstletterbold",
  "endbold",
  "lastletterbold",
  "singleletterbold",
  "boldword",
  "lenboldphrase",
  "firstwordunder",
  "undersign",
  "lastwordunderbefore",
  "lastwordunderafter",
  "begunder",
  "firstletterunder",
  "endunder",
  "lastletterunder",
  "singleletterunder",
  "underword",
  "lenunderphrase",
  "begcomp",
  "compbegemph1",
  "compendemph1",
  "compbegemph2",
  "compendemph2",
  "compbegemph3",
  "compendemph3",
  "compcapsign",
  "compbegcaps",
  "compendcaps",
  "endcomp",
  "multind",
  "compdots",
  "comp6",
  "class",
  "after",
  "before",
  "noback",
  "nofor",
  "swapcc",
  "swapcd",
  "swapdd",
  "space",
  "digit",
  "punctuation",
  "math",
  "sign",
  "letter",
  "uppercase",
  "lowercase",
  "grouping",
  "uplow",
  "litdigit",
  "display",
  "replace",
  "context",
  "correct",
  "pass2",
  "pass3",
  "pass4",
  "repeated",
  "repword",
  "capsnocont",
  "always",
  "exactdots",
  "nocross",
  "syllable",
  "nocont",
  "compbrl",
  "literal",
  "largesign",
  "word",
  "partword",
  "joinnum",
  "joinword",
  "lowword",
  "contraction",
  "sufword",
  "prfword",
  "begword",
  "begmidword",
  "midword",
  "midendword",
  "endword",
  "prepunc",
  "postpunc",
  "begnum",
  "midnum",
  "endnum",
  "decpoint",
  "hyphen",
  "nobreak"
};
static short opcodeLengths[CTO_None] = { 0 };

typedef enum
{ noEncoding, bigEndian, littleEndian, ascii8 } EncodingType;


typedef struct
{
  const char *fileName;
  FILE *in;
  int lineNumber;
  EncodingType encoding;
  int status;
  int linelen;
  int linepos;
  int checkencoding[2];
  widechar line[MAXSTRING];
}
FileInfo;

static char scratchBuf[MAXSTRING];

char *
showString (widechar const *chars, int length)
{
/*Translate a string of characters to the encoding used in character 
* operands */
  int charPos;
  int bufPos = 0;
  scratchBuf[bufPos++] = '\'';
  for (charPos = 0; charPos < length; charPos++)
    {
      if (chars[charPos] >= 32 && chars[charPos] < 127)
	scratchBuf[bufPos++] = (char) chars[charPos];
      else
	{
	  char hexbuf[20];
	  int hexLength;
	  char escapeLetter;

	  int leadingZeros;
	  int hexPos;
	  hexLength = sprintf (hexbuf, "%x", chars[charPos]);
	  switch (hexLength)
	    {
	    case 1:
	    case 2:
	    case 3:
	    case 4:
	      escapeLetter = 'x';
	      leadingZeros = 4 - hexLength;
	      break;
	    case 5:
	      escapeLetter = 'y';
	      leadingZeros = 0;
	      break;
	    case 6:
	    case 7:
	    case 8:
	      escapeLetter = 'z';
	      leadingZeros = 8 - hexLength;
	      break;
	    default:
	      escapeLetter = '?';
	      leadingZeros = 0;
	      break;
	    }
	  if ((bufPos + leadingZeros + hexLength + 4) >= sizeof (scratchBuf))
	    break;
	  scratchBuf[bufPos++] = '\\';
	  scratchBuf[bufPos++] = escapeLetter;
	  for (hexPos = 0; hexPos < leadingZeros; hexPos++)
	    scratchBuf[bufPos++] = '0';
	  for (hexPos = 0; hexPos < hexLength; hexPos++)
	    scratchBuf[bufPos++] = hexbuf[hexPos];
	}
    }
  scratchBuf[bufPos++] = '\'';
  scratchBuf[bufPos] = 0;
  return scratchBuf;
}

char *
showDots (widechar const *dots, int length)
{
/* Translate a sequence of dots to the encoding used in dots operands. 
*/
  int bufPos = 0;
  int dotsPos;
  for (dotsPos = 0; bufPos < sizeof (scratchBuf) && dotsPos < length;
       dotsPos++)
    {
      if ((dots[dotsPos] & B1))
	scratchBuf[bufPos++] = '1';
      if ((dots[dotsPos] & B2))
	scratchBuf[bufPos++] = '2';
      if ((dots[dotsPos] & B3))
	scratchBuf[bufPos++] = '3';
      if ((dots[dotsPos] & B4))
	scratchBuf[bufPos++] = '4';
      if ((dots[dotsPos] & B5))
	scratchBuf[bufPos++] = '5';
      if ((dots[dotsPos] & B6))
	scratchBuf[bufPos++] = '6';
      if ((dots[dotsPos] & B7))
	scratchBuf[bufPos++] = '7';
      if ((dots[dotsPos] & B8))
	scratchBuf[bufPos++] = '8';
      if ((dots[dotsPos] & B9))
	scratchBuf[bufPos++] = '9';
      if ((dots[dotsPos] & B10))
	scratchBuf[bufPos++] = 'A';
      if ((dots[dotsPos] & B11))
	scratchBuf[bufPos++] = 'B';
      if ((dots[dotsPos] & B12))
	scratchBuf[bufPos++] = 'C';
      if ((dots[dotsPos] & B13))
	scratchBuf[bufPos++] = 'D';
      if ((dots[dotsPos] & B14))
	scratchBuf[bufPos++] = 'E';
      if ((dots[dotsPos] & B15))
	scratchBuf[bufPos++] = 'F';
      if ((dots[dotsPos] == B16))
	scratchBuf[bufPos++] = '0';
      if (dotsPos != length - 1)
	scratchBuf[bufPos++] = '-';
    }
  scratchBuf[bufPos] = 0;
  return &scratchBuf[0];
}

char *
showAttributes (TranslationTableCharacterAttributes a)
{
/* Show attributes using the letters used after the $ in multipass 
* opcodes. */
  int bufPos = 0;
  if ((a & CTC_Space))
    scratchBuf[bufPos++] = 's';
  if ((a & CTC_Letter))
    scratchBuf[bufPos++] = 'l';
  if ((a & CTC_Digit))
    scratchBuf[bufPos++] = 'd';
  if ((a & CTC_Punctuation))
    scratchBuf[bufPos++] = 'p';
  if ((a & CTC_UpperCase))
    scratchBuf[bufPos++] = 'U';
  if ((a & CTC_LowerCase))
    scratchBuf[bufPos++] = 'u';
  if ((a & CTC_Math))
    scratchBuf[bufPos++] = 'm';
  if ((a & CTC_Sign))
    scratchBuf[bufPos++] = 'S';
  if ((a & CTC_LitDigit))
    scratchBuf[bufPos++] = 'D';
  if ((a & CTC_Class1))
    scratchBuf[bufPos++] = 'w';
  if ((a & CTC_Class2))
    scratchBuf[bufPos++] = 'x';
  if ((a & CTC_Class3))
    scratchBuf[bufPos++] = 'y';
  if ((a & CTC_Class4))
    scratchBuf[bufPos++] = 'z';
  scratchBuf[bufPos] = 0;
  return scratchBuf;
}

static void compileError (FileInfo * nested, char *format, ...);

static int
getAChar (FileInfo * nested)
{
/*Read a big endian, little *ndian or ASCII 8 file and convert it to 
* 16- or 32-bit unsigned integers */
  int ch1 = 0, ch2 = 0;
  widechar character;
  if (nested->encoding == ascii8)
    if (nested->status == 2)
      {
	nested->status++;
	return nested->checkencoding[1];
      }
  while ((ch1 = fgetc (nested->in)) != EOF)
    {
      if (nested->status < 2)
	nested->checkencoding[nested->status] = ch1;
      nested->status++;
      if (nested->status == 2)
	{
	  if (nested->checkencoding[0] == 0xfe
	      && nested->checkencoding[1] == 0xff)
	    nested->encoding = bigEndian;
	  else if (nested->checkencoding[0] == 0xff
		   && nested->checkencoding[1] == 0xfe)
	    nested->encoding = littleEndian;
	  else if (nested->checkencoding[0] < 128
		   && nested->checkencoding[1] < 128)
	    {
	      nested->encoding = ascii8;
	      return nested->checkencoding[0];
	    }
	  else
	    {
	      compileError (nested,
			    "encoding is neither big-endian, little-endian nor ASCII 8.");
	      ch1 = EOF;
	      break;;
	    }
	  continue;
	}
      switch (nested->encoding)
	{
	case noEncoding:
	  break;
	case ascii8:
	  return ch1;
	  break;
	case bigEndian:
	  ch2 = fgetc (nested->in);
	  if (ch2 == EOF)
	    break;
	  character = (ch1 << 8) | ch2;
	  return (int) character;
	  break;
	case littleEndian:
	  ch2 = fgetc (nested->in);
	  if (ch2 == EOF)
	    break;
	  character = (ch2 << 8) | ch1;
	  return (int) character;
	  break;
	}
      if (ch1 == EOF || ch2 == EOF)
	break;
    }
  return EOF;
}

static int
getALine (FileInfo * nested)
{
/*Read a line of widechar's from an input file */
  int ch;
  int pch = 0;
  nested->linelen = 0;
  while ((ch = getAChar (nested)) != EOF)
    {
      if (ch == 13)
	continue;
      if (pch == '\\' && ch == 10)
	{
	  nested->linelen--;
	  continue;
	}
      if (ch == 10 || nested->linelen >= MAXSTRING)
	break;
      nested->line[nested->linelen++] = (widechar) ch;
      pch = ch;
    }
  nested->line[nested->linelen] = 0;
  nested->linepos = 0;
  if (ch == EOF)
    return 0;
  nested->lineNumber++;
  return 1;
}

static int lastToken;
static int
getToken (FileInfo * nested, CharsString * result, const char *description)
{
/*Find the next string of contiguous non-whitespace characters. If this 
 * is the last token on the line, return 2 instead of 1. */
  while (nested->line[nested->linepos] && nested->line[nested->linepos] <= 32)
    nested->linepos++;
  result->length = 0;
  while (nested->line[nested->linepos] && nested->line[nested->linepos] > 32)
    {
    int maxlen = MAXSTRING;
    if (result->length >= maxlen)
    {
    compileError (nested, "more than %d characters (bytes)", maxlen);
    return 0;
    }
    else
    result->chars[result->length++] = nested->line[nested->linepos++];
    }
  if (!result->length)
    {
      /* Not enough tokens */
      if (description)
	compileError (nested, "%s not specified.", description);
      return 0;
    }
  result->chars[result->length] = 0;
  while (nested->line[nested->linepos] && nested->line[nested->linepos] <= 32)
    nested->linepos++;
  if (nested->line[nested->linepos] == 0)
    {
      lastToken = 1;
      return 2;
    }
  else
    {
      lastToken = 0;
      return 1;
    }
}

static void
compileError (FileInfo * nested, char *format, ...)
{
#ifndef __SYMBIAN32__
  char buffer[MAXSTRING];
  va_list arguments;
  va_start (arguments, format);
#ifdef _WIN32
  _vsnprintf (buffer, sizeof (buffer), format, arguments);
#else
  vsnprintf (buffer, sizeof (buffer), format, arguments);
#endif
  va_end (arguments);
  if (nested)
    logMessage (LOG_ERROR, "%s:%d: error: %s", nested->fileName,
		  nested->lineNumber, buffer);
  else
    logMessage (LOG_ERROR, "error: %s", buffer);
  errorCount++;
#endif
}

static void
compileWarning (FileInfo * nested, char *format, ...)
{
#ifndef __SYMBIAN32__
  char buffer[MAXSTRING];
  va_list arguments;
  va_start (arguments, format);
#ifdef _WIN32
  _vsnprintf (buffer, sizeof (buffer), format, arguments);
#else
  vsnprintf (buffer, sizeof (buffer), format, arguments);
#endif
  va_end (arguments);
  if (nested)
    logMessage (LOG_WARN, "%s:%d: warning: %s", nested->fileName,
		  nested->lineNumber, buffer);
  else
    logMessage (LOG_WARN, "warning: %s", buffer);
  warningCount++;
#endif
}

static int
allocateSpaceInTable (FileInfo * nested, TranslationTableOffset * offset,
		      int count)
{
/* allocate memory for translation table and expand previously allocated 
* memory if necessary */
  int spaceNeeded = ((count + OFFSETSIZE - 1) / OFFSETSIZE) * OFFSETSIZE;
  TranslationTableOffset size = tableUsed + spaceNeeded;
  if (size > tableSize)
    {
      void *newTable;
      size += (size / OFFSETSIZE);
      newTable = realloc (table, size);
      if (!newTable)
	{
	  compileError (nested, "Not enough memory for translation table.");
	  outOfMemory ();
	}
      memset (((unsigned char *) newTable) + tableSize, 0, size - tableSize);
      /* update references to the old table */
      {
	ChainEntry *entry;
	for (entry = tableChain; entry != NULL; entry = entry->next)
	  if (entry->table == table)
	    entry->table = (TranslationTableHeader *) newTable;
      }
      table = (TranslationTableHeader *) newTable;
      tableSize = size;
    }
  if (offset != NULL)
    {
      *offset = (tableUsed - sizeof (*table)) / OFFSETSIZE;
      tableUsed += spaceNeeded;
    }
  return 1;
}

static int
reserveSpaceInTable (FileInfo * nested, int count)
{
  return (allocateSpaceInTable (nested, NULL, count));
}

static int
allocateHeader (FileInfo * nested)
{
/*Allocate memory for the table header and a guess on the number of 
* rules */
  const TranslationTableOffset startSize = 2 * sizeof (*table);
  if (table)
    return 1;
  tableUsed = sizeof (*table) + OFFSETSIZE;	/*So no offset is ever zero */
  if (!(table = malloc (startSize)))
    {
      compileError (nested, "Not enough memory");
      if (table != NULL)
	free (table);
      table = NULL;
      outOfMemory ();
    }
  memset (table, 0, startSize);
  tableSize = startSize;
  return 1;
}

int
stringHash (const widechar * c)
{
/*hash function for strings */
  unsigned long int makeHash = (((unsigned long int) c[0] << 8) +
				(unsigned long int) c[1]) % HASHNUM;
  return (int) makeHash;
}

int
charHash (widechar c)
{
  unsigned long int makeHash = (unsigned long int) c % HASHNUM;
  return (int) makeHash;
}

static TranslationTableCharacter *
compile_findCharOrDots (widechar c, int m)
{
/*Look up a character or dot pattern. If m is 0 look up a character, 
* otherwise look up a dot pattern. Although the algorithms are almost 
* identical, different tables are needed for characters and dots because 
* of the possibility of conflicts.*/
  TranslationTableCharacter *character;
  TranslationTableOffset bucket;
  unsigned long int makeHash = (unsigned long int) c % HASHNUM;
  if (m == 0)
    bucket = table->characters[makeHash];
  else
    bucket = table->dots[makeHash];
  while (bucket)
    {
      character = (TranslationTableCharacter *) & table->ruleArea[bucket];
      if (character->realchar == c)
	return character;
      bucket = character->next;
    }
  return NULL;
}

static TranslationTableCharacter noChar = { 0, 0, 0, CTC_Space, 32, 32, 32 };
static TranslationTableCharacter noDots =
  { 0, 0, 0, CTC_Space, B16, B16, B16 };
static char *unknownDots (widechar dots);

static TranslationTableCharacter *
definedCharOrDots (FileInfo * nested, widechar c, int m)
{
  TranslationTableCharacter *notFound;
  TranslationTableCharacter *charOrDots = compile_findCharOrDots (c, m);
  if (charOrDots)
    return charOrDots;
  if (m == 0)
    {
      notFound = &noChar;
      compileError (nested,
		    "character %s should be defined at this point but is not",
		    showString (&c, 1));
    }
  else
    {
      notFound = &noDots;
      compileError (nested,
		    "cell %s should be defined at this point but is not",
		    unknownDots (c));
    }
  return notFound;
}

static TranslationTableCharacter *
addCharOrDots (FileInfo * nested, widechar c, int m)
{
/*See if a character or dot pattern is in the appropriate table. If not, 
* insert it. In either 
* case, return a pointer to it. */
  TranslationTableOffset bucket;
  TranslationTableCharacter *character;
  TranslationTableCharacter *oldchar;
  TranslationTableOffset offset;
  unsigned long int makeHash;
  if ((character = compile_findCharOrDots (c, m)))
    return character;
  if (!allocateSpaceInTable (nested, &offset, sizeof (*character)))
    return NULL;
  character = (TranslationTableCharacter *) & table->ruleArea[offset];
  memset (character, 0, sizeof (*character));
  character->realchar = c;
  makeHash = (unsigned long int) c % HASHNUM;
  if (m == 0)
    bucket = table->characters[makeHash];
  else
    bucket = table->dots[makeHash];
  if (!bucket)
    {
      if (m == 0)
	table->characters[makeHash] = offset;
      else
	table->dots[makeHash] = offset;
    }
  else
    {
      oldchar = (TranslationTableCharacter *) & table->ruleArea[bucket];
      while (oldchar->next)
	oldchar =
	  (TranslationTableCharacter *) & table->ruleArea[oldchar->next];
      oldchar->next = offset;
    }
  return character;
}

static CharOrDots *
getCharOrDots (widechar c, int m)
{
  CharOrDots *cdPtr;
  TranslationTableOffset bucket;
  unsigned long int makeHash = (unsigned long int) c % HASHNUM;
  if (m == 0)
    bucket = table->charToDots[makeHash];
  else
    bucket = table->dotsToChar[makeHash];
  while (bucket)
    {
      cdPtr = (CharOrDots *) & table->ruleArea[bucket];
      if (cdPtr->lookFor == c)
	return cdPtr;
      bucket = cdPtr->next;
    }
  return NULL;
}

widechar
getDotsForChar (widechar c)
{
  CharOrDots *cdPtr = getCharOrDots (c, 0);
  if (cdPtr)
    return cdPtr->found;
  return B16;
}

widechar
getCharFromDots (widechar d)
{
  CharOrDots *cdPtr = getCharOrDots (d, 1);
  if (cdPtr)
    return cdPtr->found;
  return ' ';
}

static int
putCharAndDots (FileInfo * nested, widechar c, widechar d)
{
  TranslationTableOffset bucket;
  CharOrDots *cdPtr;
  CharOrDots *oldcdPtr = NULL;
  TranslationTableOffset offset;
  unsigned long int makeHash;
  if (!(cdPtr = getCharOrDots (c, 0)))
    {
      if (!allocateSpaceInTable (nested, &offset, sizeof (*cdPtr)))
	return 0;
      cdPtr = (CharOrDots *) & table->ruleArea[offset];
      cdPtr->next = 0;
      cdPtr->lookFor = c;
      cdPtr->found = d;
      makeHash = (unsigned long int) c % HASHNUM;
      bucket = table->charToDots[makeHash];
      if (!bucket)
	table->charToDots[makeHash] = offset;
      else
	{
	  oldcdPtr = (CharOrDots *) & table->ruleArea[bucket];
	  while (oldcdPtr->next)
	    oldcdPtr = (CharOrDots *) & table->ruleArea[oldcdPtr->next];
	  oldcdPtr->next = offset;
	}
    }
  if (!(cdPtr = getCharOrDots (d, 1)))
    {
      if (!allocateSpaceInTable (nested, &offset, sizeof (*cdPtr)))
	return 0;
      cdPtr = (CharOrDots *) & table->ruleArea[offset];
      cdPtr->next = 0;
      cdPtr->lookFor = d;
      cdPtr->found = c;
      makeHash = (unsigned long int) d % HASHNUM;
      bucket = table->dotsToChar[makeHash];
      if (!bucket)
	table->dotsToChar[makeHash] = offset;
      else
	{
	  oldcdPtr = (CharOrDots *) & table->ruleArea[bucket];
	  while (oldcdPtr->next)
	    oldcdPtr = (CharOrDots *) & table->ruleArea[oldcdPtr->next];
	  oldcdPtr->next = offset;
	}
    }
  return 1;
}

static char *
unknownDots (widechar dots)
{
/*Print out dot numbers */
  static char buffer[20];
  int k = 1;
  buffer[0] = '\\';
  if ((dots & B1))
    buffer[k++] = '1';
  if ((dots & B2))
    buffer[k++] = '2';
  if ((dots & B3))
    buffer[k++] = '3';
  if ((dots & B4))
    buffer[k++] = '4';
  if ((dots & B5))
    buffer[k++] = '5';
  if ((dots & B6))
    buffer[k++] = '6';
  if ((dots & B7))
    buffer[k++] = '7';
  if ((dots & B8))
    buffer[k++] = '8';
  if ((dots & B9))
    buffer[k++] = '9';
  if ((dots & B10))
    buffer[k++] = 'A';
  if ((dots & B11))
    buffer[k++] = 'B';
  if ((dots & B12))
    buffer[k++] = 'C';
  if ((dots & B13))
    buffer[k++] = 'D';
  if ((dots & B14))
    buffer[k++] = 'E';
  if ((dots & B15))
    buffer[k++] = 'F';
  buffer[k++] = '/';
  buffer[k] = 0;
  return buffer;
}

static TranslationTableOffset newRuleOffset = 0;
static TranslationTableRule *newRule = NULL;

static int
charactersDefined (FileInfo * nested)
{
/*Check that all characters are defined by character-definition 
* opcodes*/
  int noErrors = 1;
  int k;
  if ((newRule->opcode >= CTO_Space && newRule->opcode <= CTO_LitDigit)
      || newRule->opcode == CTO_SwapDd
      ||
      newRule->opcode == CTO_Replace || newRule->opcode == CTO_MultInd
      || newRule->opcode == CTO_Repeated ||
      ((newRule->opcode >= CTO_Context && newRule->opcode <=
	CTO_Pass4) && newRule->opcode != CTO_Correct))
    return 1;
  for (k = 0; k < newRule->charslen; k++)
    if (!compile_findCharOrDots (newRule->charsdots[k], 0))
      {
	compileError (nested, "Character %s is not defined", showString
		      (&newRule->charsdots[k], 1));
	noErrors = 0;
      }
  if (!(newRule->opcode == CTO_Correct || newRule->opcode ==
	CTO_NoBreak || newRule->opcode == CTO_SwapCc || newRule->opcode ==
	CTO_SwapCd))
    {
      for (k = newRule->charslen; k < newRule->charslen + newRule->dotslen;
	   k++)
	if (!compile_findCharOrDots (newRule->charsdots[k], 1))
	  {
	    compileError (nested, "Dot pattern %s is not defined.",
			  unknownDots (newRule->charsdots[k]));
	    noErrors = 0;
	  }
    }
  return noErrors;
}

static int noback = 0;
static int nofor = 0;

/*The following functions are 
called by addRule to handle various 
* cases.*/

static void
add_0_single (FileInfo * nested)
{
/*direction = 0, newRule->charslen = 1*/
  TranslationTableRule *currentRule;
  TranslationTableOffset *currentOffsetPtr;
  TranslationTableCharacter *character;
  int m = 0;
  if (newRule->opcode == CTO_CompDots || newRule->opcode == CTO_Comp6)
    return;
  if (newRule->opcode >= CTO_Pass2 && newRule->opcode <= CTO_Pass4)
    m = 1;
  character = definedCharOrDots (nested, newRule->charsdots[0], m);
  if (m != 1 && character->attributes & CTC_Letter && (newRule->opcode
						       ==
						       CTO_WholeWord
						       || newRule->opcode ==
						       CTO_LargeSign))
    {
      if (table->noLetsignCount < LETSIGNSIZE)
	table->noLetsign[table->noLetsignCount++] = newRule->charsdots[0];
    }
  if (newRule->opcode >= CTO_Space && newRule->opcode < CTO_UpLow)
    character->definitionRule = newRuleOffset;
  currentOffsetPtr = &character->otherRules;
  while (*currentOffsetPtr)
    {
      currentRule = (TranslationTableRule *)
	& table->ruleArea[*currentOffsetPtr];
      if (currentRule->charslen == 0)
	break;
      if (currentRule->opcode >= CTO_Space && currentRule->opcode < CTO_UpLow)
	if (!(newRule->opcode >= CTO_Space && newRule->opcode < CTO_UpLow))
	  break;
      currentOffsetPtr = &currentRule->charsnext;
    }
  newRule->charsnext = *currentOffsetPtr;
  *currentOffsetPtr = newRuleOffset;
}

static void
add_0_multiple ()
{
/*direction = 0 newRule->charslen > 1*/
  TranslationTableRule *currentRule = NULL;
  TranslationTableOffset *currentOffsetPtr =
    &table->forRules[stringHash (&newRule->charsdots[0])];
  while (*currentOffsetPtr)
    {
      currentRule = (TranslationTableRule *)
	& table->ruleArea[*currentOffsetPtr];
      if (newRule->charslen > currentRule->charslen)
	break;
      if (newRule->charslen == currentRule->charslen)
	if ((currentRule->opcode == CTO_Always)
	    && (newRule->opcode != CTO_Always))
	  break;
      currentOffsetPtr = &currentRule->charsnext;
    }
  newRule->charsnext = *currentOffsetPtr;
  *currentOffsetPtr = newRuleOffset;
}

static void
add_1_single (FileInfo * nested)
{
/*direction = 1, newRule->dotslen = 1*/
  TranslationTableRule *currentRule;
  TranslationTableOffset *currentOffsetPtr;
  TranslationTableCharacter *dots;
  if (newRule->opcode == CTO_NoBreak || newRule->opcode == CTO_SwapCc ||
      (newRule->opcode >= CTO_Context
       &&
       newRule->opcode <= CTO_Pass4)
      || newRule->opcode == CTO_Repeated || (newRule->opcode == CTO_Always
					     && newRule->charslen == 1))
    return;			/*too ambiguous */
  dots = definedCharOrDots (nested, newRule->charsdots[newRule->charslen], 1);
  if (newRule->opcode >= CTO_Space && newRule->opcode < CTO_UpLow)
    dots->definitionRule = newRuleOffset;
  currentOffsetPtr = &dots->otherRules;
  while (*currentOffsetPtr)
    {
      currentRule = (TranslationTableRule *)
	& table->ruleArea[*currentOffsetPtr];
      if (newRule->charslen > currentRule->charslen ||
	  currentRule->dotslen == 0)
	break;
      if (currentRule->opcode >= CTO_Space && currentRule->opcode < CTO_UpLow)
	if (!(newRule->opcode >= CTO_Space && newRule->opcode < CTO_UpLow))
	  break;
      currentOffsetPtr = &currentRule->dotsnext;
    }
  newRule->dotsnext = *currentOffsetPtr;
  *currentOffsetPtr = newRuleOffset;
}

static void
add_1_multiple ()
{
/*direction = 1, newRule->dotslen > 1*/
  TranslationTableRule *currentRule = NULL;
  TranslationTableOffset *currentOffsetPtr = &table->backRules[stringHash
							       (&newRule->
								charsdots
								[newRule->
								 charslen])];
  if (newRule->opcode == CTO_NoBreak || newRule->opcode == CTO_SwapCc ||
      (newRule->opcode >= CTO_Context && newRule->opcode <= CTO_Pass4))
    return;
  while (*currentOffsetPtr)
    {
      int currentLength;
      int newLength;
      currentRule = (TranslationTableRule *)
	& table->ruleArea[*currentOffsetPtr];
      currentLength = currentRule->dotslen + currentRule->charslen;
      newLength = newRule->dotslen + newRule->charslen;
      if (newLength > currentLength)
	break;
      if (currentLength == newLength)
	if ((currentRule->opcode == CTO_Always)
	    && (newRule->opcode != CTO_Always))
	  break;
      currentOffsetPtr = &currentRule->dotsnext;
    }
  newRule->dotsnext = *currentOffsetPtr;
  *currentOffsetPtr = newRuleOffset;
}

static void
makeRuleChain (TranslationTableOffset * offsetPtr)
{
  TranslationTableRule *currentRule;
  while (*offsetPtr)
    {
      currentRule = (TranslationTableRule *) & table->ruleArea[*offsetPtr];
      offsetPtr = &currentRule->charsnext;
    }
  newRule->charsnext = *offsetPtr;
  *offsetPtr = newRuleOffset;
}

static int
addPassRule (FileInfo * nested)
{
  TranslationTableOffset *offsetPtr;
  switch (newRule->opcode)
    {
    case CTO_Correct:
      offsetPtr = &table->attribOrSwapRules[0];
      break;
    case CTO_Context:
      offsetPtr = &table->attribOrSwapRules[1];
      break;
    case CTO_Pass2:
      offsetPtr = &table->attribOrSwapRules[2];
      break;
    case CTO_Pass3:
      offsetPtr = &table->attribOrSwapRules[3];
      break;
    case CTO_Pass4:
      offsetPtr = &table->attribOrSwapRules[4];
      break;
    default:
      return 0;
    }
  makeRuleChain (offsetPtr);
  return 1;
}

static int
  addRule
  (FileInfo * nested,
   TranslationTableOpcode opcode,
   CharsString * ruleChars,
   CharsString * ruleDots,
   TranslationTableCharacterAttributes after,
   TranslationTableCharacterAttributes before)
{
/*Add a rule to the table, using the hash function to find the start of 
* chains and chaining both the chars and dots strings */
  int ruleSize = sizeof (TranslationTableRule) - (DEFAULTRULESIZE * CHARSIZE);
  int direction = 0;		/*0 = forward translation; 1 = bacward */
  if (ruleChars)
    ruleSize += CHARSIZE * ruleChars->length;
  if (ruleDots)
    ruleSize += CHARSIZE * ruleDots->length;
  if (!allocateSpaceInTable (nested, &newRuleOffset, ruleSize))
    return 0;
  newRule = (TranslationTableRule *) & table->ruleArea[newRuleOffset];
  newRule->opcode = opcode;
  newRule->after = after;
  newRule->before = before;
  if (ruleChars)
    memcpy (&newRule->charsdots[0], &ruleChars->chars[0],
	    CHARSIZE * (newRule->charslen = ruleChars->length));
  else
    newRule->charslen = 0;
  if (ruleDots)
    memcpy (&newRule->charsdots[newRule->charslen],
	    &ruleDots->chars[0], CHARSIZE * (newRule->dotslen =
					     ruleDots->length));
  else
    newRule->dotslen = 0;
  if (!charactersDefined (nested))
    return 0;

  /*link new rule into table. */
  if (opcode == CTO_SwapCc || opcode == CTO_SwapCd || opcode == CTO_SwapDd)
    return 1;
  if (opcode >= CTO_Context && opcode <= CTO_Pass4 && newRule->charslen == 0)
    return addPassRule (nested);
  if (newRule->charslen == 0 || nofor)
    direction = 1;
  while (direction < 2)
    {
      if (direction == 0 && newRule->charslen == 1)
	add_0_single (nested);
      else if (direction == 0 && newRule->charslen > 1)
	add_0_multiple ();
      else if (direction == 1 && newRule->dotslen == 1 && !noback)
	add_1_single (nested);
      else if (direction == 1 && newRule->dotslen > 1 && !noback)
	add_1_multiple ();
      else
	{
	}
      direction++;
      if (newRule->dotslen == 0)
	direction = 2;
    }
  return 1;
}

static const struct CharacterClass *
findCharacterClass (const CharsString * name)
{
/*Find a character class, whether predefined or user-defined */
  const struct CharacterClass *class = characterClasses;
  while (class)
    {
      if ((name->length == class->length) &&
	  (memcmp (&name->chars[0], class->name, CHARSIZE *
		   name->length) == 0))
	return class;
      class = class->next;
    }
  return NULL;
}

static struct CharacterClass *
addCharacterClass (FileInfo * nested, const widechar * name, int length)
{
/*Define a character class, Whether predefined or user-defined */
  struct CharacterClass *class;
  if (characterClassAttribute)
    {
      if (!(class = malloc (sizeof (*class) + CHARSIZE * (length - 1))))
	outOfMemory ();
      else
	{
	  memset (class, 0, sizeof (*class));
	  memcpy (class->name, name, CHARSIZE * (class->length = length));
	  class->attribute = characterClassAttribute;
	  characterClassAttribute <<= 1;
	  class->next = characterClasses;
	  characterClasses = class;
	  return class;
	}
    }
  compileError (nested, "character class table overflow.");
  return NULL;
}

static void
deallocateCharacterClasses ()
{
  while (characterClasses)
    {
      struct CharacterClass *class = characterClasses;
      characterClasses = characterClasses->next;
      if (class)
	free (class);
    }
}

static int
allocateCharacterClasses ()
{
/*Allocate memory for predifined character classes */
  int k = 0;
  characterClasses = NULL;
  characterClassAttribute = 1;
  while (characterClassNames[k])
    {
      widechar wname[MAXSTRING];
      int length = strlen (characterClassNames[k]);
      int kk;
      for (kk = 0; kk < length; kk++)
	wname[kk] = (widechar) characterClassNames[k][kk];
      if (!addCharacterClass (NULL, wname, length))
	{
	  deallocateCharacterClasses ();
	  return 0;
	}
      k++;
    }
  return 1;
}

static TranslationTableOpcode
getOpcode (FileInfo * nested, const CharsString * token)
{
  static TranslationTableOpcode lastOpcode = 0;
  TranslationTableOpcode opcode = lastOpcode;

  do
    {
      if (token->length == opcodeLengths[opcode])
	if (eqasc2uni ((unsigned char *) opcodeNames[opcode],
		       &token->chars[0], token->length))
	  {
	    lastOpcode = opcode;
	    return opcode;
	  }
      opcode++;
      if (opcode >= CTO_None)
	opcode = 0;
    }
  while (opcode != lastOpcode);
  compileError (nested, "opcode %s not defined.", showString
		(&token->chars[0], token->length));
  return CTO_None;
}

TranslationTableOpcode
findOpcodeNumber (const char *toFind)
{
/* Used by tools such as lou_debug */
  static TranslationTableOpcode lastOpcode = 0;
  TranslationTableOpcode opcode = lastOpcode;
  int length = strlen (toFind);
  do
    {
      if (length == opcodeLengths[opcode] && strcasecmp (toFind,
							 opcodeNames[opcode])
	  == 0)
	{
	  lastOpcode = opcode;
	  return opcode;
	}
      opcode++;
      if (opcode >= CTO_None)
	opcode = 0;
    }
  while (opcode != lastOpcode);
  return CTO_None;
}

const char *
findOpcodeName (TranslationTableOpcode opcode)
{
/* Used by tools such as lou_debug */
  if (opcode < 0 || opcode >= CTO_None)
    {
      sprintf (scratchBuf, "%d", opcode);
      return scratchBuf;
    }
  return opcodeNames[opcode];
}

static widechar
hexValue (FileInfo * nested, const widechar * digits, int length)
{
  int k;
  unsigned int binaryValue = 0;
  for (k = 0; k < length; k++)
    {
      unsigned int hexDigit = 0;
      if (digits[k] >= '0' && digits[k] <= '9')
	hexDigit = digits[k] - '0';
      else if (digits[k] >= 'a' && digits[k] <= 'f')
	hexDigit = digits[k] - 'a' + 10;
      else if (digits[k] >= 'A' && digits[k] <= 'F')
	hexDigit = digits[k] - 'A' + 10;
      else
	{
	  compileError (nested, "invalid %d-digit hexadecimal number",
			length);
	  return (widechar) 0xffffffff;
	}
      binaryValue |= hexDigit << (4 * (length - 1 - k));
    }
  return (widechar) binaryValue;
}

#define MAXBYTES 7
static int first0Bit[MAXBYTES] = { 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0XFE };

static int
parseChars (FileInfo * nested, CharsString * result, CharsString * token)
{
  int in = 0;
  int out = 0;
  int lastOutSize = 0;
  int lastIn;
  unsigned int ch = 0;
  int numBytes = 0;
  unsigned int utf32 = 0;
  int k;
  while (in < token->length)
    {
      ch = token->chars[in++] & 0xff;
      if (ch < 128)
	{
	  if (ch == '\\')
	    {			/* escape sequence */
	      switch (ch = token->chars[in])
		{
		case '\\':
		  break;
		case 'e':
		  ch = 0x1b;
		  break;
		case 'f':
		  ch = 12;
		  break;
		case 'n':
		  ch = 10;
		  break;
		case 'r':
		  ch = 13;
		  break;
		case 's':
		  ch = ' ';
		  break;
		case 't':
		  ch = 9;
		  break;
		case 'v':
		  ch = 22;
		  break;
		case 'w':
		  ch = ENDSEGMENT;
		  break;
		case 34:
		  ch = QUOTESUB;
		  break;
		case 'X':
		case 'x':
		  if (token->length - in > 4)
		    {
		      ch = hexValue (nested, &token->chars[in + 1], 4);
		      in += 4;
		    }
		  break;
		case 'y':
		case 'Y':
		  if (CHARSIZE == 2)
		    {
		    not32:
		      compileError (nested,
				    "liblouis has not been compiled for 32-bit Unicode");
		      break;
		    }
		  if (token->length - in > 5)
		    {
		      ch = hexValue (nested, &token->chars[in + 1], 5);
		      in += 5;
		    }
		  break;
		case 'z':
		case 'Z':
		  if (CHARSIZE == 2)
		    goto not32;
		  if (token->length - in > 8)
		    {
		      ch = hexValue (nested, &token->chars[in + 1], 8);
		      in += 8;
		    }
		  break;
		default:
		  compileError (nested, "invalid escape sequence '\\%c'", ch);
		  break;
		}
	      in++;
	    }
	  result->chars[out++] = (widechar) ch;
	  if (out >= MAXSTRING)
	    {
	      result->length = out;
	      return 1;
	    }
	  continue;
	}
      lastOutSize = out;
      lastIn = in;
      for (numBytes = MAXBYTES - 1; numBytes > 0; numBytes--)
	if (ch >= first0Bit[numBytes])
	  break;
      utf32 = ch & (0XFF - first0Bit[numBytes]);
      for (k = 0; k < numBytes; k++)
	{
	  if (in >= MAXSTRING)
	    break;
	  if (token->chars[in] < 128 || (token->chars[in] & 0x0040))
	    {
	      compileWarning (nested, "invalid UTF-8. Assuming Latin-1.");
	      result->chars[out++] = token->chars[lastIn];
	      in = lastIn + 1;
	      continue;
	    }
	  utf32 = (utf32 << 6) + (token->chars[in++] & 0x3f);
	}
      if (CHARSIZE == 2 && utf32 > 0xffff)
	utf32 = 0xffff;
      result->chars[out++] = (widechar) utf32;
      if (out >= MAXSTRING)
	{
	  result->length = lastOutSize;
	  return 1;
	}
    }
  result->length = out;
  return 1;
}

int
extParseChars (const char *inString, widechar * outString)
{
/* Parse external character strings */
  CharsString wideIn;
  CharsString result;
  int k;
  for (k = 0; inString[k] && k < MAXSTRING; k++)
    wideIn.chars[k] = inString[k];
  wideIn.chars[k] = 0;
  wideIn.length = k;
  parseChars (NULL, &result, &wideIn);
  if (errorCount)
    {
      errorCount = 0;
      return 0;
    }
  for (k = 0; k < result.length; k++)
    outString[k] = result.chars[k];
  return result.length;
}

static int
parseDots (FileInfo * nested, CharsString * cells, const CharsString * token)
{
/*get dot patterns */
  widechar cell = 0;		/*assembly place for dots */
  int cellCount = 0;
  int index;
  int start = 0;

  for (index = 0; index < token->length; index++)
    {
      int started = index != start;
      widechar character = token->chars[index];
      switch (character)
	{			/*or dots to make up Braille cell */
	  {
	    int dot;
	case '1':
	    dot = B1;
	    goto haveDot;
	case '2':
	    dot = B2;
	    goto haveDot;
	case '3':
	    dot = B3;
	    goto haveDot;
	case '4':
	    dot = B4;
	    goto haveDot;
	case '5':
	    dot = B5;
	    goto haveDot;
	case '6':
	    dot = B6;
	    goto haveDot;
	case '7':
	    dot = B7;
	    goto haveDot;
	case '8':
	    dot = B8;
	    goto haveDot;
	case '9':
	    dot = B9;
	    goto haveDot;
	case 'a':
	case 'A':
	    dot = B10;
	    goto haveDot;
	case 'b':
	case 'B':
	    dot = B11;
	    goto haveDot;
	case 'c':
	case 'C':
	    dot = B12;
	    goto haveDot;
	case 'd':
	case 'D':
	    dot = B13;
	    goto haveDot;
	case 'e':
	case 'E':
	    dot = B14;
	    goto haveDot;
	case 'f':
	case 'F':
	    dot = B15;
	  haveDot:
	    if (started && !cell)
	      goto invalid;
	    if (cell & dot)
	      {
		compileError (nested, "dot specified more than once.");
		return 0;
	      }
	    cell |= dot;
	    break;
	  }
	case '0':		/*blank */
	  if (started)
	    goto invalid;
	  break;
	case '-':		/*got all dots for this cell */
	  if (!started)
	    {
	      compileError (nested, "missing cell specification.");
	      return 0;
	    }
	  cells->chars[cellCount++] = cell | B16;
	  cell = 0;
	  start = index + 1;
	  break;
	default:
	invalid:
	  compileError (nested, "invalid dot number %s.", showString
			(&character, 1));
	  return 0;
	}
    }
  if (index == start)
    {
      compileError (nested, "missing cell specification.");
      return 0;
    }
  cells->chars[cellCount++] = cell | B16;	/*last cell */
  cells->length = cellCount;
  return 1;
}

int
extParseDots (const char *inString, widechar * outString)
{
/* Parse external dot patterns */
  CharsString wideIn;
  CharsString result;
  int k;
  for (k = 0; inString[k] && k < MAXSTRING; k++)
    wideIn.chars[k] = inString[k];
  wideIn.chars[k] = 0;
  wideIn.length = k;
  parseDots (NULL, &result, &wideIn);
  if (errorCount)
    {
      errorCount = 0;
      return 0;
    }
  for (k = 0; k < result.length; k++)
    outString[k] = result.chars[k];
  outString[k] = 0;
  return result.length;
}

static int
getCharacters (FileInfo * nested, CharsString * characters)
{
/*Get ruleChars string */
  CharsString token;
  if (getToken (nested, &token, "characters"))
    if (parseChars (nested, characters, &token))
      return 1;
  return 0;
}

static int
getRuleCharsText (FileInfo * nested, CharsString * ruleChars)
{
  CharsString token;
  if (getToken (nested, &token, "Characters operand"))
    if (parseChars (nested, ruleChars, &token))
      return 1;
  return 0;
}

static int
getRuleDotsText (FileInfo * nested, CharsString * ruleDots)
{
  CharsString token;
  if (getToken (nested, &token, "characters"))
    if (parseChars (nested, ruleDots, &token))
      return 1;
  return 0;
}

static int
getRuleDotsPattern (FileInfo * nested, CharsString * ruleDots)
{
/*Interpret the dets operand */
  CharsString token;
  if (getToken (nested, &token, "Dots operand"))
    {
      if (token.length == 1 && token.chars[0] == '=')
	{
	  ruleDots->length = 0;
	  return 1;
	}
      if (parseDots (nested, ruleDots, &token))
	return 1;
    }
  return 0;
}

static int
getCharacterClass (FileInfo * nested, const struct CharacterClass **class)
{
  CharsString token;
  if (getToken (nested, &token, "character class name"))
    {
      if ((*class = findCharacterClass (&token)))
	return 1;
      compileError (nested, "character class not defined.");
    }
  return 0;
}

static int includeFile (FileInfo * nested, CharsString * includedFile);

struct RuleName
{
  struct RuleName *next;
  TranslationTableOffset ruleOffset;
  widechar length;
  widechar name[1];
};
static struct RuleName *ruleNames = NULL;
static TranslationTableOffset
findRuleName (const CharsString * name)
{
  const struct RuleName *nameRule = ruleNames;
  while (nameRule)
    {
      if ((name->length == nameRule->length) &&
	  (memcmp (&name->chars[0], nameRule->name, CHARSIZE *
		   name->length) == 0))
	return nameRule->ruleOffset;
      nameRule = nameRule->next;
    }
  return 0;
}

static int
addRuleName (FileInfo * nested, CharsString * name)
{
  int k;
  struct RuleName *nameRule;
  if (!(nameRule = malloc (sizeof (*nameRule) + CHARSIZE *
			   (name->length - 1))))
    {
      compileError (nested, "not enough memory");
      outOfMemory ();
    }
  memset (nameRule, 0, sizeof (*nameRule));
  for (k = 0; k < name->length; k++)
    {
      TranslationTableCharacter *ch = definedCharOrDots
	(nested, name->chars[k],
	 0);
      if (!(ch->attributes & CTC_Letter))
	{
	  compileError (nested, "a name may contain only letters");
	  return 0;
	}
      nameRule->name[k] = name->chars[k];
    }
  nameRule->length = name->length;
  nameRule->ruleOffset = newRuleOffset;
  nameRule->next = ruleNames;
  ruleNames = nameRule;
  return 1;
}

static void
deallocateRuleNames ()
{
  while (ruleNames)
    {
      struct RuleName *nameRule = ruleNames;
      ruleNames = ruleNames->next;
      if (nameRule)
	free (nameRule);
    }
}

static int
compileSwapDots (FileInfo * nested, CharsString * source, CharsString * dest)
{
  int k = 0;
  int kk = 0;
  CharsString dotsSource;
  CharsString dotsDest;
  dest->length = 0;
  dotsSource.length = 0;
  while (k <= source->length)
    {
      if (source->chars[k] != ',' && k != source->length)
	dotsSource.chars[dotsSource.length++] = source->chars[k];
      else
	{
	  if (!parseDots (nested, &dotsDest, &dotsSource))
	    return 0;
	  dest->chars[dest->length++] = dotsDest.length + 1;
	  for (kk = 0; kk < dotsDest.length; kk++)
	    dest->chars[dest->length++] = dotsDest.chars[kk];
	  dotsSource.length = 0;
	}
      k++;
    }
  return 1;
}

static int
compileSwap (FileInfo * nested, TranslationTableOpcode opcode)
{
  CharsString ruleChars;
  CharsString ruleDots;
  CharsString name;
  CharsString matches;
  CharsString replacements;
  if (!getToken (nested, &name, "name operand"))
    return 0;
  if (!getToken (nested, &matches, "matches operand"))
    return 0;
  if (!getToken (nested, &replacements, "replacements operand"))
    return 0;
  if (opcode == CTO_SwapCc || opcode == CTO_SwapCd)
    {
      if (!parseChars (nested, &ruleChars, &matches))
	return 0;
    }
  else
    {
      if (!compileSwapDots (nested, &matches, &ruleChars))
	return 0;
    }
  if (opcode == CTO_SwapCc)
    {
      if (!parseChars (nested, &ruleDots, &replacements))
	return 0;
    }
  else
    {
      if (!compileSwapDots (nested, &replacements, &ruleDots))
	return 0;
    }
  if (!addRule (nested, opcode, &ruleChars, &ruleDots, 0, 0))
    return 0;
  if (!addRuleName (nested, &name))
    return 0;
  return 1;
}


static int
getNumber (widechar * source, widechar * dest)
{
/*Convert a string of wide character digits to an integer*/
  int k = 0;
  *dest = 0;
  while (source[k] >= '0' && source[k] <= '9')
    *dest = 10 * *dest + (source[k++] - '0');
  return k;
}

/* Start of multipass compiler*/
static CharsString passRuleChars;
static CharsString passRuleDots;
static CharsString passHoldString;
static CharsString passLine;
static int passLinepos;
static int passPrevLinepos;
static widechar passHoldNumber;
static widechar passEmphasis;
static TranslationTableCharacterAttributes passAttributes;
static FileInfo *passNested;
static TranslationTableOpcode passOpcode;
static widechar *passInstructions;
static int passIC;

static int
passGetAttributes ()
{
  int more = 1;
  passAttributes = 0;
  while (more)
    {
      switch (passLine.chars[passLinepos])
	{
	case pass_any:
	  passAttributes = 0xffffffff;
	  break;
	case pass_digit:
	  passAttributes |= CTC_Digit;
	  break;
	case pass_litDigit:
	  passAttributes |= CTC_LitDigit;
	  break;
	case pass_letter:
	  passAttributes |= CTC_Letter;
	  break;
	case pass_math:
	  passAttributes |= CTC_Math;
	  break;
	case pass_punctuation:
	  passAttributes |= CTC_Punctuation;
	  break;
	case pass_sign:
	  passAttributes |= CTC_Sign;
	  break;
	case pass_space:
	  passAttributes |= CTC_Space;
	  break;
	case pass_uppercase:
	  passAttributes |= CTC_UpperCase;
	  break;
	case pass_lowercase:
	  passAttributes |= CTC_LowerCase;
	  break;
	case pass_class1:
	  passAttributes |= CTC_Class1;
	  break;
	case pass_class2:
	  passAttributes |= CTC_Class2;
	  break;
	case pass_class3:
	  passAttributes |= CTC_Class3;
	  break;
	case pass_class4:
	  passAttributes |= CTC_Class4;
	  break;
	default:
	  more = 0;
	  break;
	}
      if (more)
	passLinepos++;
    }
  if (!passAttributes)
    {
      compileError (passNested, "Missing attribute");
      passLinepos--;
      return 0;
    }
  return 1;
}

static int
passGetEmphasis ()
{
  int more = 1;
  passLinepos++;
  passEmphasis = 0;
  while (more)
    {
      switch (passLine.chars[passLinepos])
	{
	case 'i':
	  passEmphasis |= italic;
	  break;
	case 'b':
	  passEmphasis |= bold;
	  break;
	case 'u':
	  passEmphasis |= underline;
	  break;
	case 'c':
	  passEmphasis |= computer_braille;
	  break;
	default:
	  more = 0;
	  break;
	}
      if (more)
	passLinepos++;
    }
  if (!passEmphasis)
    {
      compileError (passNested, "emphasis indicators expected");
      passLinepos--;
      return 0;
    }
  return 1;
}

static int
passGetDots ()
{
  CharsString collectDots;
  collectDots.length = 0;
  while (passLinepos < passLine.length && (passLine.chars[passLinepos]
					   == '-'
					   || (passLine.chars[passLinepos] >=
					       '0'
					       && passLine.
					       chars[passLinepos] <= '9')
					   ||
					   ((passLine.
					     chars[passLinepos] | 32) >= 'a'
					    && (passLine.
						chars[passLinepos] | 32) <=
					    'f')))
    collectDots.chars[collectDots.length++] = passLine.chars[passLinepos++];
  if (!parseDots (passNested, &passHoldString, &collectDots))
    return 0;
  return 1;
}

static int
passGetString ()
{
  passHoldString.length = 0;
  while (1)
    {
      if (!passLine.chars[passLinepos])
	{
	  compileError (passNested, "unterminated string");
	  return 0;
	}
      if (passLine.chars[passLinepos] == 34)
	break;
      if (passLine.chars[passLinepos] == QUOTESUB)
	passHoldString.chars[passHoldString.length++] = 34;
      else
	passHoldString.chars[passHoldString.length++] =
	  passLine.chars[passLinepos];
      passLinepos++;
    }
  passHoldString.chars[passHoldString.length] = 0;
  passLinepos++;
  return 1;
}

static int
passGetNumber ()
{
  /*Convert a string of wide character digits to an integer */
  passHoldNumber = 0;
  while (passLine.chars[passLinepos] >= '0'
	 && passLine.chars[passLinepos] <= '9')
    passHoldNumber =
      10 * passHoldNumber + (passLine.chars[passLinepos++] - '0');
  return 1;
}

static int
passGetName ()
{
  TranslationTableCharacterAttributes attr;
  passHoldString.length = 0;
  do
    {
      attr = definedCharOrDots (passNested, passLine.chars[passLinepos],
				0)->attributes;
      if (passHoldString.length == 0)
	{
	  if (!(attr & CTC_Letter))
	    {
	      passLinepos++;
	      continue;
	    }
	}
      if (!(attr & CTC_Letter))
	break;
      passHoldString.chars[passHoldString.length++] =
	passLine.chars[passLinepos];
      passLinepos++;
    }
  while (passLinepos < passLine.length);
  return 1;
}

static int
passIsKeyword (const char *token)
{
  int k;
  int length = strlen (token);
  int ch = passLine.chars[passLinepos + length + 1];
  if (((ch | 32) >= 'a' && (ch | 32) <= 'z') || (ch >= '0' && ch <= '9'))
    return 0;
  for (k = 0; k < length && passLine.chars[passLinepos + k + 1]
       == (widechar) token[k]; k++);
  if (k == length)
    {
      passLinepos += length + 1;
      return 1;
    }
  return 0;
}

struct PassName
{
  struct PassName *next;
  int varnum;
  widechar length;
  widechar name[1];
};
static struct PassName *passNames = NULL;

static int
passFindName (const CharsString * name)
{
  const struct PassName *curname = passNames;
  CharsString augmentedName;
  for (augmentedName.length = 0; augmentedName.length < name->length;
       augmentedName.length++)
    augmentedName.chars[augmentedName.length] =
      name->chars[augmentedName.length];
  augmentedName.chars[augmentedName.length++] = passOpcode;
  while (curname)
    {
      if ((augmentedName.length == curname->length) &&
	  (memcmp
	   (&augmentedName.chars[0], curname->name,
	    CHARSIZE * name->length) == 0))
	return curname->varnum;
      curname = curname->next;
    }
  compileError (passNested, "name not found");
  return 0;
}

static int
passAddName (CharsString * name, int var)
{
  int k;
  struct PassName *curname;
  CharsString augmentedName;
  for (augmentedName.length = 0;
       augmentedName.length < name->length; augmentedName.length++)
    augmentedName.
      chars[augmentedName.length] = name->chars[augmentedName.length];
  augmentedName.chars[augmentedName.length++] = passOpcode;
  if (!
      (curname =
       malloc (sizeof (*curname) + CHARSIZE * (augmentedName.length - 1))))
    {
      outOfMemory ();
    }
  memset (curname, 0, sizeof (*curname));
  for (k = 0; k < augmentedName.length; k++)
    {
      curname->name[k] = augmentedName.chars[k];
    }
  curname->length = augmentedName.length;
  curname->varnum = var;
  curname->next = passNames;
  passNames = curname;
  return 1;
}

static pass_Codes
passGetScriptToken ()
{
  while (passLinepos < passLine.length)
    {
      passPrevLinepos = passLinepos;
      switch (passLine.chars[passLinepos])
	{
	case '\"':
	  passLinepos++;
	  if (passGetString ())
	    return pass_string;
	  return pass_invalidToken;
	case '@':
	  passLinepos++;
	  if (passGetDots ())
	    return pass_dots;
	  return pass_invalidToken;
	case '#':		/*comment */
	  passLinepos = passLine.length + 1;
	  return pass_noMoreTokens;
	case '!':
	  if (passLine.chars[passLinepos + 1] == '=')
	    {
	      passLinepos += 2;
	      return pass_noteq;
	    }
	  passLinepos++;
	  return pass_not;
	case '-':
	  passLinepos++;
	  return pass_hyphen;
	case '=':
	  passLinepos++;
	  return pass_eq;
	case '<':
	  passLinepos++;
	  if (passLine.chars[passLinepos] == '=')
	    {
	      passLinepos++;
	      return pass_lteq;
	    }
	  return pass_lt;
	case '>':
	  passLinepos++;
	  if (passLine.chars[passLinepos] == '=')
	    {
	      passLinepos++;
	      return pass_gteq;
	    }
	  return pass_gt;
	case '+':
	  passLinepos++;
	  return pass_plus;
	case '(':
	  passLinepos++;
	  return pass_leftParen;
	case ')':
	  passLinepos++;
	  return pass_rightParen;
	case ',':
	  passLinepos++;
	  return pass_comma;
	case '&':
	  if (passLine.chars[passLinepos = 1] == '&')
	    {
	      passLinepos += 2;
	      return pass_and;
	    }
	  return pass_invalidToken;
	case '|':
	  if (passLine.chars[passLinepos + 1] == '|')
	    {
	      passLinepos += 2;
	      return pass_or;
	    }
	  return pass_invalidToken;
	case 'a':
	  if (passIsKeyword ("ttr"))
	    return pass_attributes;
	  passGetName ();
	  return pass_nameFound;
	case 'b':
	  if (passIsKeyword ("ack"))
	    return pass_lookback;
	  if (passIsKeyword ("ool"))
	    return pass_boolean;
	  passGetName ();
	  return pass_nameFound;
	case 'c':
	  if (passIsKeyword ("lass"))
	    return pass_class;
	  passGetName ();
	  return pass_nameFound;
	case 'd':
	  if (passIsKeyword ("ef"))
	    return pass_define;
	  passGetName ();
	  return pass_nameFound;
	case 'e':
	  if (passIsKeyword ("mph"))
	    return pass_emphasis;
	  passGetName ();
	  return pass_nameFound;
	case 'f':
	  if (passIsKeyword ("ind"))
	    return pass_search;
	  if (passIsKeyword ("irst"))
	    return pass_first;
	  passGetName ();
	  return pass_nameFound;
	case 'g':
	  if (passIsKeyword ("roup"))
	    return pass_group;
	  passGetName ();
	  return pass_nameFound;
	case 'i':
	  if (passIsKeyword ("f"))
	    return pass_if;
	  passGetName ();
	  return pass_nameFound;
	case 'l':
	  if (passIsKeyword ("ast"))
	    return pass_last;
	  passGetName ();
	  return pass_nameFound;
	case 'm':
	  if (passIsKeyword ("ark"))
	    return pass_mark;
	  passGetName ();
	  return pass_nameFound;
	case 'r':
	  if (passIsKeyword ("epgroup"))
	    return pass_repGroup;
	  if (passIsKeyword ("epcopy"))
	    return pass_copy;
	  if (passIsKeyword ("epomit"))
	    return pass_omit;
	  if (passIsKeyword ("ep"))
	    return pass_replace;
	  passGetName ();
	  return pass_nameFound;
	case 's':
	  if (passIsKeyword ("cript"))
	    return pass_script;
	  if (passIsKeyword ("wap"))
	    return pass_swap;
	  passGetName ();
	  return pass_nameFound;
	case 't':
	  if (passIsKeyword ("hen"))
	    return pass_then;
	  passGetName ();
	  return pass_nameFound;
	default:
	  if (passLine.chars[passLinepos] <= 32)
	    {
	      passLinepos++;
	      break;
	    }
	  if (passLine.chars[passLinepos] >= '0'
	      && passLine.chars[passLinepos] <= '9')
	    {
	      passGetNumber ();
	      return pass_numberFound;
	    }
	  else
	    {
	      if (!passGetName ())
		return pass_invalidToken;
	      else
		return pass_nameFound;
	    }
	}
    }
  return pass_noMoreTokens;
}

static int
passIsLeftParen ()
{
  pass_Codes passCode = passGetScriptToken ();
  if (passCode != pass_leftParen)
    {
      compileError (passNested, "'(' expected");
      return 0;
    }
  return 1;
}

static int
passIsName ()
{
  pass_Codes passCode = passGetScriptToken ();
  if (passCode != pass_nameFound)
    {
      compileError (passNested, "a name expected");
      return 0;
    }
  return 1;
}

static int
passIsComma ()
{
  pass_Codes passCode = passGetScriptToken ();
  if (passCode != pass_comma)
    {
      compileError (passNested, "',' expected");
      return 0;
    }
  return 1;
}

static int
passIsNumber ()
{
  pass_Codes passCode = passGetScriptToken ();
  if (passCode != pass_numberFound)
    {
      compileError (passNested, "a number expected");
      return 0;
    }
  return 1;
}

static int
passIsRightParen ()
{
  pass_Codes passCode = passGetScriptToken ();
  if (passCode != pass_rightParen)
    {
      compileError (passNested, "')' expected");
      return 0;
    }
  return 1;
}

static int
passGetRange ()
{
  pass_Codes passCode = passGetScriptToken ();
  if (!(passCode == pass_comma || passCode == pass_rightParen))
    {
      compileError (passNested, "invalid range");
      return 0;
    }
  if (passCode == pass_rightParen)
    {
      passInstructions[passIC++] = 1;
      passInstructions[passIC++] = 1;
      return 1;
    }
  if (!passIsNumber ())
    return 0;
  passInstructions[passIC++] = passHoldNumber;
  passCode = passGetScriptToken ();
  if (!(passCode == pass_comma || passCode == pass_rightParen))
    {
      compileError (passNested, "invalid range");
      return 0;
    }
  if (passCode == pass_rightParen)
    {
      passInstructions[passIC++] = passHoldNumber;
      return 1;
    }
  if (!passIsNumber ())
    return 0;
  passInstructions[passIC++] = passHoldNumber;
  if (!passIsRightParen ())
    return 0;
  return 1;
}

static int
passInsertAttributes ()
{
  passInstructions[passIC++] = pass_attributes;
  passInstructions[passIC++] = passAttributes >> 16;
  passInstructions[passIC++] = passAttributes & 0xffff;
  if (!passGetRange ())
    return 0;
  return 1;
}

static int
compilePassOpcode (FileInfo * nested, TranslationTableOpcode opcode)
{
/*Compile the operands of a pass opcode */
  TranslationTableCharacterAttributes after = 0;
  TranslationTableCharacterAttributes before = 0;
  widechar passSubOp;
  const struct CharacterClass *class;
  TranslationTableOffset ruleOffset = 0;
  TranslationTableRule *rule = NULL;
  int k;
  int kk = 0;
  pass_Codes passCode;
  int endTest = 0;
  int isScript = 1;
  passInstructions = passRuleDots.chars;
  passIC = 0;			/*Instruction counter */
  passRuleChars.length = 0;
  passNested = nested;
  passOpcode = opcode;
/* passHoldString and passLine are static variables declared 
 * previously.*/
  passLinepos = 0;
  passHoldString.length = 0;
  for (k = nested->linepos; k < nested->linelen; k++)
    passHoldString.chars[passHoldString.length++] = nested->line[k];
  if (!eqasc2uni ((unsigned char *) "script", passHoldString.chars, 6))
    {
      isScript = 0;
#define SEPCHAR 0x0001
      for (k = 0; k < passHoldString.length && passHoldString.chars[k] > 32;
	   k++);
      if (k < passHoldString.length)
	passHoldString.chars[k] = SEPCHAR;
      else
	{
	  compileError (passNested, "Invalid multipass operands");
	  return 0;
	}
    }
  parseChars (passNested, &passLine, &passHoldString);
  if (isScript)
    {
      int more = 1;
      passCode = passGetScriptToken ();
      if (passCode != pass_script)
	{
	  compileError (passNested, "Invalid multipass statement");
	  return 0;
	}
      /* Declaratives */
      while (more)
	{
	  passCode = passGetScriptToken ();
	  switch (passCode)
	    {
	    case pass_define:
	      if (!passIsLeftParen ())
		return 0;
	      if (!passIsName ())
		return 0;
	      if (!passIsComma ())
		return 0;
	      if (!passIsNumber ())
		return 0;
	      if (!passIsRightParen ())
		return 0;
	      passAddName (&passHoldString, passHoldNumber);
	      break;
	    case pass_if:
	      more = 0;
	      break;
	    default:
	      compileError (passNested,
			    "invalid definition in declarative part");
	      return 0;
	    }
	}
      /* if part */
      more = 1;
      while (more)
	{
	  passCode = passGetScriptToken ();
	  passSubOp = passCode;
	  switch (passCode)
	    {
	    case pass_not:
	      passInstructions[passIC++] = pass_not;
	      break;
	    case pass_first:
	      passInstructions[passIC++] = pass_first;
	      break;
	    case pass_last:
	      passInstructions[passIC++] = pass_last;
	      break;
	    case pass_search:
	      passInstructions[passIC++] = pass_search;
	      break;
	    case pass_string:
	      if (opcode != CTO_Context && opcode != CTO_Correct)
		{
		  compileError (passNested,
				"Character strings can only be used with the context and correct opcodes.");
		  return 0;
		}
	      passInstructions[passIC++] = pass_string;
	      goto ifDoCharsDots;
	    case pass_dots:
	      if (passOpcode == CTO_Correct || passOpcode == CTO_Context)
		{
		  compileError (passNested,
				"dot patterns cannot be specified in the if part\
 of the correct or context opcodes");
		  return 0;
		}
	      passInstructions[passIC++] = pass_dots;
	    ifDoCharsDots:
	      passInstructions[passIC++] = passHoldString.length;
	      for (kk = 0; kk < passHoldString.length; kk++)
		passInstructions[passIC++] = passHoldString.chars[kk];
	      break;
	    case pass_attributes:
	      if (!passIsLeftParen ())
		return 0;
	      if (!passGetAttributes ())
		return 0;
	      if (!passInsertAttributes ())
		return 0;
	      break;
	    case pass_emphasis:
	      if (!passIsLeftParen ())
		return 0;
	      if (!passGetEmphasis ())
		return 0;
	      /*Right parenthis handled by subfunctiion */
	      break;
	    case pass_lookback:
	      passInstructions[passIC++] = pass_lookback;
	      passCode = passGetScriptToken ();
	      if (passCode != pass_leftParen)
		{
		  passInstructions[passIC++] = 1;
		  passLinepos = passPrevLinepos;
		  break;
		}
	      if (!passIsNumber ())
		return 0;
	      if (!passIsRightParen ())
		return 0;
	      passInstructions[passIC] = passHoldNumber;
	      break;
	    case pass_group:
	      if (!passIsLeftParen ())
		return 0;
	      break;
	    case pass_mark:
	      passInstructions[passIC++] = pass_startReplace;
	      passInstructions[passIC++] = pass_endReplace;
	      break;
	    case pass_replace:
	      passInstructions[passIC++] = pass_startReplace;
	      if (!passIsLeftParen ())
		return 0;
	      break;
	    case pass_rightParen:
	      passInstructions[passIC++] = pass_endReplace;
	      break;
	    case pass_groupstart:
	    case pass_groupend:
	      if (!passIsLeftParen ())
		return 0;
	      if (!passGetName ())
		return 0;
	      if (!passIsRightParen ())
		return 0;
	      ruleOffset = findRuleName (&passHoldString);
	      if (ruleOffset)
		rule = (TranslationTableRule *) & table->ruleArea[ruleOffset];
	      if (rule && rule->opcode == CTO_Grouping)
		{
		  passInstructions[passIC++] = passSubOp;
		  passInstructions[passIC++] = ruleOffset >> 16;
		  passInstructions[passIC++] = ruleOffset & 0xffff;
		  break;
		}
	      else
		{
		  compileError (passNested, "%s is not a grouping name",
				showString (&passHoldString.chars[0],
					    passHoldString.length));
		  return 0;
		}
	      break;
	    case pass_class:
	      if (!passIsLeftParen ())
		return 0;
	      if (!passGetName ())
		return 0;
	      if (!passIsRightParen ())
		return 0;
	      if (!(class = findCharacterClass (&passHoldString)))
		return 0;
	      passAttributes = class->attribute;
	      passInsertAttributes ();
	      break;
	    case pass_swap:
	      ruleOffset = findRuleName (&passHoldString);
	      if (!passIsLeftParen ())
		return 0;
	      if (!passGetName ())
		return 0;
	      if (!passIsRightParen ())
		return 0;
	      ruleOffset = findRuleName (&passHoldString);
	      if (ruleOffset)
		rule = (TranslationTableRule *) & table->ruleArea[ruleOffset];
	      if (rule
		  && (rule->opcode == CTO_SwapCc || rule->opcode == CTO_SwapCd
		      || rule->opcode == CTO_SwapDd))
		{
		  passInstructions[passIC++] = pass_swap;
		  passInstructions[passIC++] = ruleOffset >> 16;
		  passInstructions[passIC++] = ruleOffset & 0xffff;
		  if (!passGetRange ())
		    return 0;
		  break;
		}
	      compileError (passNested,
			    "%s is not a swap name.",
			    showString (&passHoldString.chars[0],
					passHoldString.length));
	      return 0;
	    case pass_nameFound:
	      passHoldNumber = passFindName (&passHoldString);
	      passCode = passGetScriptToken ();
	      if (!(passCode == pass_eq || passCode == pass_lt || passCode
		    == pass_gt || passCode == pass_noteq || passCode ==
		    pass_lteq || passCode == pass_gteq))
		{
		  compileError (nested,
				"invalid comparison operator in if part");
		  return 0;
		}
	      passInstructions[passIC++] = passCode;
	      passInstructions[passIC++] = passHoldNumber;
	      if (!passIsNumber ())
		return 0;
	      passInstructions[passIC++] = passHoldNumber;
	      break;
	    case pass_then:
	      passInstructions[passIC++] = pass_endTest;
	      more = 0;
	      break;
	    default:
	      compileError (passNested, "invalid choice in if part");
	      return 0;
	    }
	}

      /* then part */
      more = 1;
      while (more)
	{
	  passCode = passGetScriptToken ();
	  passSubOp = passCode;
	  switch (passCode)
	    {
	    case pass_string:
	      if (opcode != CTO_Correct)
		{
		  compileError (passNested,
				"Character strings can only be used in the then part with the correct opcode.");
		  return 0;
		}
	      passInstructions[passIC++] = pass_string;
	      goto thenDoCharsDots;
	    case pass_dots:
	      if (opcode == CTO_Correct)
		{
		  compileError (passNested,
				"Dot patterns cannot be used with the correct opcode.");
		  return 0;
		}
	      passInstructions[passIC++] = pass_dots;
	    thenDoCharsDots:
	      passInstructions[passIC++] = passHoldString.length;
	      for (kk = 0; kk < passHoldString.length; kk++)
		passInstructions[passIC++] = passHoldString.chars[kk];
	      break;
	    case pass_nameFound:
	      passHoldNumber = passFindName (&passHoldString);
	      passCode = passGetScriptToken ();
	      if (!(passCode == pass_plus || passCode == pass_hyphen
		    || passCode == pass_eq))
		{
		  compileError (nested,
				"Invalid variable operator in then part");
		  return 0;
		}
	      passInstructions[passIC++] = passCode;
	      passInstructions[passIC++] = passHoldNumber;
	      if (!passIsNumber ())
		return 0;
	      passInstructions[passIC++] = passHoldNumber;
	      break;
	    case pass_copy:
	      passInstructions[passIC++] = pass_copy;
	      break;
	    case pass_omit:
	      passInstructions[passIC++] = pass_omit;
	      break;
	    case pass_swap:
	      ruleOffset = findRuleName (&passHoldString);
	      if (!passIsLeftParen ())
		return 0;
	      if (!passGetName ())
		return 0;
	      if (!passIsRightParen ())
		return 0;
	      ruleOffset = findRuleName (&passHoldString);
	      if (ruleOffset)
		rule = (TranslationTableRule *) & table->ruleArea[ruleOffset];
	      if (rule
		  && (rule->opcode == CTO_SwapCc || rule->opcode == CTO_SwapCd
		      || rule->opcode == CTO_SwapDd))
		{
		  passInstructions[passIC++] = pass_swap;
		  passInstructions[passIC++] = ruleOffset >> 16;
		  passInstructions[passIC++] = ruleOffset & 0xffff;
		  if (!passGetRange ())
		    return 0;
		  break;
		}
	      compileError (passNested,
			    "%s is not a swap name.",
			    showString (&passHoldString.chars[0],
					passHoldString.length));
	      return 0;
	    case pass_noMoreTokens:
	      more = 0;
	      break;
	    default:
	      compileError (passNested, "invalid action in then part");
	      return 0;
	    }
	}
    }
  else
    {
      /* Older machine-language-like "assembler". */

      /*Compile test part */
      for (k = 0; k < passLine.length && passLine.chars[k] != SEPCHAR; k++);
      endTest = k;
      passLine.chars[endTest] = pass_endTest;
      passLinepos = 0;
      while (passLinepos <= endTest)
	{
	  switch ((passSubOp = passLine.chars[passLinepos]))
	    {
	    case pass_lookback:
	      passInstructions[passIC++] = pass_lookback;
	      passLinepos++;
	      passGetNumber ();
	      if (passHoldNumber == 0)
		passHoldNumber = 1;
	      passInstructions[passIC++] = passHoldNumber;
	      break;
	    case pass_not:
	      passInstructions[passIC++] = pass_not;
	      passLinepos++;
	      break;
	    case pass_first:
	      passInstructions[passIC++] = pass_first;
	      passLinepos++;
	      break;
	    case pass_last:
	      passInstructions[passIC++] = pass_last;
	      passLinepos++;
	      break;
	    case pass_search:
	      passInstructions[passIC++] = pass_search;
	      passLinepos++;
	      break;
	    case pass_string:
	      if (opcode != CTO_Context && opcode != CTO_Correct)
		{
		  compileError (passNested,
				"Character strings can only be used with the context and correct opcodes.");
		  return 0;
		}
	      passLinepos++;
	      passInstructions[passIC++] = pass_string;
	      passGetString ();
	      goto testDoCharsDots;
	    case pass_dots:
	      passLinepos++;
	      passInstructions[passIC++] = pass_dots;
	      passGetDots ();
	    testDoCharsDots:
	      if (passHoldString.length == 0)
		return 0;
	      passInstructions[passIC++] = passHoldString.length;
	      for (kk = 0; kk < passHoldString.length; kk++)
		passInstructions[passIC++] = passHoldString.chars[kk];
	      break;
	    case pass_startReplace:
	      passInstructions[passIC++] = pass_startReplace;
	      passLinepos++;
	      break;
	    case pass_endReplace:
	      passInstructions[passIC++] = pass_endReplace;
	      passLinepos++;
	      break;
	    case pass_variable:
	      passLinepos++;
	      passGetNumber ();
	      switch (passLine.chars[passLinepos])
		{
		case pass_eq:
		  passInstructions[passIC++] = pass_eq;
		  goto doComp;
		case pass_lt:
		  if (passLine.chars[passLinepos + 1] == pass_eq)
		    {
		      passLinepos++;
		      passInstructions[passIC++] = pass_lteq;
		    }
		  else
		    passInstructions[passIC++] = pass_lt;
		  goto doComp;
		case pass_gt:
		  if (passLine.chars[passLinepos + 1] == pass_eq)
		    {
		      passLinepos++;
		      passInstructions[passIC++] = pass_gteq;
		    }
		  else
		    passInstructions[passIC++] = pass_gt;
		doComp:
		  passInstructions[passIC++] = passHoldNumber;
		  passLinepos++;
		  passGetNumber ();
		  passInstructions[passIC++] = passHoldNumber;
		  break;
		default:
		  compileError (passNested, "incorrect comparison operator");
		  return 0;
		}
	      break;
	    case pass_attributes:
	      passLinepos++;
	      passGetAttributes ();
	    insertAttributes:
	      passInstructions[passIC++] = pass_attributes;
	      passInstructions[passIC++] = passAttributes >> 16;
	      passInstructions[passIC++] = passAttributes & 0xffff;
	    getRange:
	      if (passLine.chars[passLinepos] == pass_until)
		{
		  passLinepos++;
		  passInstructions[passIC++] = 1;
		  passInstructions[passIC++] = 0xffff;
		  break;
		}
	      passGetNumber ();
	      if (passHoldNumber == 0)
		{
		  passHoldNumber = passInstructions[passIC++] = 1;
		  passInstructions[passIC++] = 1;	/*This is not an error */
		  break;
		}
	      passInstructions[passIC++] = passHoldNumber;
	      if (passLine.chars[passLinepos] != pass_hyphen)
		{
		  passInstructions[passIC++] = passHoldNumber;
		  break;
		}
	      passLinepos++;
	      passGetNumber ();
	      if (passHoldNumber == 0)
		{
		  compileError (passNested, "invalid range");
		  return 0;
		}
	      passInstructions[passIC++] = passHoldNumber;
	      break;
	    case pass_groupstart:
	    case pass_groupend:
	      passLinepos++;
	      passGetName ();
	      ruleOffset = findRuleName (&passHoldString);
	      if (ruleOffset)
		rule = (TranslationTableRule *) & table->ruleArea[ruleOffset];
	      if (rule && rule->opcode == CTO_Grouping)
		{
		  passInstructions[passIC++] = passSubOp;
		  passInstructions[passIC++] = ruleOffset >> 16;
		  passInstructions[passIC++] = ruleOffset & 0xffff;
		  break;
		}
	      else
		{
		  compileError (passNested, "%s is not a grouping name",
				showString (&passHoldString.chars[0],
					    passHoldString.length));
		  return 0;
		}
	      break;
	    case pass_swap:
	      passGetName ();
	      if ((class = findCharacterClass (&passHoldString)))
		{
		  passAttributes = class->attribute;
		  goto insertAttributes;
		}
	      ruleOffset = findRuleName (&passHoldString);
	      if (ruleOffset)
		rule = (TranslationTableRule *) & table->ruleArea[ruleOffset];
	      if (rule
		  && (rule->opcode == CTO_SwapCc || rule->opcode == CTO_SwapCd
		      || rule->opcode == CTO_SwapDd))
		{
		  passInstructions[passIC++] = pass_swap;
		  passInstructions[passIC++] = ruleOffset >> 16;
		  passInstructions[passIC++] = ruleOffset & 0xffff;
		  goto getRange;
		}
	      compileError (passNested,
			    "%s is neither a class name nor a swap name.",
			    showString (&passHoldString.chars[0],
					passHoldString.length));
	      return 0;
	    case pass_endTest:
	      passInstructions[passIC++] = pass_endTest;
	      passLinepos++;
	      break;
	    default:
	      compileError (passNested,
			    "incorrect operator '%c ' in test part",
			    passLine.chars[passLinepos]);
	      return 0;
	    }

	}			/*Compile action part */

      /* Compile action part */
      while (passLinepos < passLine.length &&
	     passLine.chars[passLinepos] <= 32)
	passLinepos++;
      while (passLinepos < passLine.length &&
	     passLine.chars[passLinepos] > 32)
	{
	  switch ((passSubOp = passLine.chars[passLinepos]))
	    {
	    case pass_string:
	      if (opcode != CTO_Correct)
		{
		  compileError (passNested,
				"Character strings can only be used with the ccorrect opcode.");
		  return 0;
		}
	      passLinepos++;
	      passInstructions[passIC++] = pass_string;
	      passGetString ();
	      goto actionDoCharsDots;
	    case pass_dots:
	      if (opcode == CTO_Correct)
		{
		  compileError (passNested,
				"Dot patterns cannot be used with the correct opcode.");
		  return 0;
		}
	      passLinepos++;
	      passGetDots ();
	      passInstructions[passIC++] = pass_dots;
	    actionDoCharsDots:
	      if (passHoldString.length == 0)
		return 0;
	      passInstructions[passIC++] = passHoldString.length;
	      for (kk = 0; kk < passHoldString.length; kk++)
		passInstructions[passIC++] = passHoldString.chars[kk];
	      break;
	    case pass_variable:
	      passLinepos++;
	      passGetNumber ();
	      switch (passLine.chars[passLinepos])
		{
		case pass_eq:
		  passInstructions[passIC++] = pass_eq;
		  passInstructions[passIC++] = passHoldNumber;
		  passLinepos++;
		  passGetNumber ();
		  passInstructions[passIC++] = passHoldNumber;
		  break;
		case pass_plus:
		case pass_hyphen:
		  passInstructions[passIC++] = passLine.chars[passLinepos];
		  passInstructions[passIC++] = passHoldNumber;
		  break;
		default:
		  compileError (passNested,
				"incorrect variable operator in action part");
		  return 0;
		}
	      break;
	    case pass_copy:
	      passInstructions[passIC++] = pass_copy;
	      passLinepos++;
	      break;
	    case pass_omit:
	      passInstructions[passIC++] = pass_omit;
	      passLinepos++;
	      break;
	    case pass_groupreplace:
	    case pass_groupstart:
	    case pass_groupend:
	      passLinepos++;
	      passGetName ();
	      ruleOffset = findRuleName (&passHoldString);
	      if (ruleOffset)
		rule = (TranslationTableRule *) & table->ruleArea[ruleOffset];
	      if (rule && rule->opcode == CTO_Grouping)
		{
		  passInstructions[passIC++] = passSubOp;
		  passInstructions[passIC++] = ruleOffset >> 16;
		  passInstructions[passIC++] = ruleOffset & 0xffff;
		  break;
		}
	      compileError (passNested, "%s is not a grouping name",
			    showString (&passHoldString.chars[0],
					passHoldString.length));
	      return 0;
	    case pass_swap:
	      passLinepos++;
	      passGetName ();
	      ruleOffset = findRuleName (&passHoldString);
	      if (ruleOffset)
		rule = (TranslationTableRule *) & table->ruleArea[ruleOffset];
	      if (rule
		  && (rule->opcode == CTO_SwapCc || rule->opcode == CTO_SwapCd
		      || rule->opcode == CTO_SwapDd))
		{
		  passInstructions[passIC++] = pass_swap;
		  passInstructions[passIC++] = ruleOffset >> 16;
		  passInstructions[passIC++] = ruleOffset & 0xffff;
		  break;
		}
	      compileError (passNested, "%s is not a swap name.",
			    showString (&passHoldString.chars[0],
					passHoldString.length));
	      return 0;
	      break;
	    default:
	      compileError (passNested, "incorrect operator in action part");
	      return 0;
	    }
	}
    }

  /*Analyze and add rule */
  passRuleDots.length = passIC;
  passIC = 0;
  while (passIC < passRuleDots.length)
    {
      int start = 0;
      switch (passInstructions[passIC])
	{
	case pass_string:
	case pass_dots:
	case pass_attributes:
	case pass_swap:
	  start = 1;
	  break;
	case pass_groupstart:
	case pass_groupend:
	  start = 1;
	  break;
	case pass_eq:
	case pass_lt:
	case pass_gt:
	case pass_lteq:
	case pass_gteq:
	  passIC += 3;
	  break;
	case pass_lookback:
	  passIC += 2;
	  break;
	case pass_not:
	case pass_startReplace:
	case pass_endReplace:
	case pass_first:
	  passIC++;
	  break;
	default:
	  compileError (passNested,
			"Test/if part must contain characters, dots, attributes or class \
swap.");
	  return 0;
	}
      if (start)
	break;
    }

  switch (passInstructions[passIC])
    {
    case pass_string:
    case pass_dots:
      for (k = 0; k < passInstructions[passIC + 1]; k++)
	passRuleChars.chars[k] = passInstructions[passIC + 2 + k];
      passRuleChars.length = k;
      after = before = 0;
      break;
    case pass_attributes:
    case pass_groupstart:
    case pass_groupend:
    case pass_swap:
      after = passRuleDots.length;
      before = 0;
      break;
    default:
      break;
    }
  if (!addRule (passNested, opcode, &passRuleChars, &passRuleDots,
		after, before))
    return 0;
  return 1;
}

/* End of multipass compiler */

static int
compileBrailleIndicator (FileInfo * nested, char *ermsg,
			 TranslationTableOpcode opcode,
			 TranslationTableOffset * rule)
{
  CharsString token;
  CharsString cells;
  if (getToken (nested, &token, ermsg))
    if (parseDots (nested, &cells, &token))
      if (!addRule (nested, opcode, NULL, &cells, 0, 0))
	return 0;
  *rule = newRuleOffset;
  return 1;
}

static int
compileNumber (FileInfo * nested)
{
  CharsString token;
  widechar dest;
  if (!getToken (nested, &token, "number"))
    return 0;
  getNumber (&token.chars[0], &dest);
  if (!(dest > 0))
    {
      compileError (nested, "a nonzero positive number is required");
      return 0;
    }
  return dest;
}

static int
compileGrouping (FileInfo * nested)
{
  int k;
  CharsString name;
  CharsString groupChars;
  CharsString groupDots;
  CharsString dotsParsed;
  TranslationTableCharacter *charsDotsPtr;
  widechar endChar;
  widechar endDots;
  if (!getToken (nested, &name, "name operand"))
    return 0;
  if (!getRuleCharsText (nested, &groupChars))
    return 0;
  if (!getToken (nested, &groupDots, "dots operand"))
    return 0;
  for (k = 0; k < groupDots.length && groupDots.chars[k] != ','; k++);
  if (k == groupDots.length)
    {
      compileError (nested,
		    "Dots operand must consist of two cells separated by a comma");
      return 0;
    }
  groupDots.chars[k] = '-';
  if (!parseDots (nested, &dotsParsed, &groupDots))
    return 0;
  if (groupChars.length != 2 || dotsParsed.length != 2)
    {
      compileError (nested,
		    "two Unicode characters and two cells separated by a comma are needed.");
      return 0;
    }
  charsDotsPtr = addCharOrDots (nested, groupChars.chars[0], 0);
  charsDotsPtr->attributes |= CTC_Math;
  charsDotsPtr->uppercase = charsDotsPtr->realchar;
  charsDotsPtr->lowercase = charsDotsPtr->realchar;
  charsDotsPtr = addCharOrDots (nested, groupChars.chars[1], 0);
  charsDotsPtr->attributes |= CTC_Math;
  charsDotsPtr->uppercase = charsDotsPtr->realchar;
  charsDotsPtr->lowercase = charsDotsPtr->realchar;
  charsDotsPtr = addCharOrDots (nested, dotsParsed.chars[0], 1);
  charsDotsPtr->attributes |= CTC_Math;
  charsDotsPtr->uppercase = charsDotsPtr->realchar;
  charsDotsPtr->lowercase = charsDotsPtr->realchar;
  charsDotsPtr = addCharOrDots (nested, dotsParsed.chars[1], 1);
  charsDotsPtr->attributes |= CTC_Math;
  charsDotsPtr->uppercase = charsDotsPtr->realchar;
  charsDotsPtr->lowercase = charsDotsPtr->realchar;
  if (!addRule (nested, CTO_Grouping, &groupChars, &dotsParsed, 0, 0))
    return 0;
  if (!addRuleName (nested, &name))
    return 0;
  putCharAndDots (nested, groupChars.chars[0], dotsParsed.chars[0]);
  putCharAndDots (nested, groupChars.chars[1], dotsParsed.chars[1]);
  endChar = groupChars.chars[1];
  endDots = dotsParsed.chars[1];
  groupChars.length = dotsParsed.length = 1;
  if (!addRule (nested, CTO_Math, &groupChars, &dotsParsed, 0, 0))
    return 0;
  groupChars.chars[0] = endChar;
  dotsParsed.chars[0] = endDots;
  if (!addRule (nested, CTO_Math, &groupChars, &dotsParsed, 0, 0))
    return 0;
  return 1;
}

static int
compileUplow (FileInfo * nested)
{
  int k;
  TranslationTableCharacter *upperChar;
  TranslationTableCharacter *lowerChar;
  TranslationTableCharacter *upperCell = NULL;
  TranslationTableCharacter *lowerCell = NULL;
  CharsString ruleChars;
  CharsString ruleDots;
  CharsString upperDots;
  CharsString lowerDots;
  int haveLowerDots = 0;
  TranslationTableCharacterAttributes attr;
  if (!getRuleCharsText (nested, &ruleChars))
    return 0;
  if (!getToken (nested, &ruleDots, "dots operand"))
    return 0;
  for (k = 0; k < ruleDots.length && ruleDots.chars[k] != ','; k++);
  if (k == ruleDots.length)
    {
      if (!parseDots (nested, &upperDots, &ruleDots))
	return 0;
      lowerDots.length = upperDots.length;
      for (k = 0; k < upperDots.length; k++)
	lowerDots.chars[k] = upperDots.chars[k];
      lowerDots.chars[k] = 0;
    }
  else
    {
      haveLowerDots = ruleDots.length;
      ruleDots.length = k;
      if (!parseDots (nested, &upperDots, &ruleDots))
	return 0;
      ruleDots.length = 0;
      k++;
      for (; k < haveLowerDots; k++)
	ruleDots.chars[ruleDots.length++] = ruleDots.chars[k];
      if (!parseDots (nested, &lowerDots, &ruleDots))
	return 0;
    }
  if (ruleChars.length != 2 || upperDots.length < 1)
    {
      compileError (nested,
		    "Exactly two Unicode characters and at least one cell are required.");
      return 0;
    }
  if (haveLowerDots && lowerDots.length < 1)
    {
      compileError (nested, "at least one cell is required after the comma.");
      return 0;
    }
  upperChar = addCharOrDots (nested, ruleChars.chars[0], 0);
  upperChar->attributes |= CTC_Letter | CTC_UpperCase;
  upperChar->uppercase = ruleChars.chars[0];
  upperChar->lowercase = ruleChars.chars[1];
  lowerChar = addCharOrDots (nested, ruleChars.chars[1], 0);
  lowerChar->attributes |= CTC_Letter | CTC_LowerCase;
  lowerChar->uppercase = ruleChars.chars[0];
  lowerChar->lowercase = ruleChars.chars[1];
  for (k = 0; k < upperDots.length; k++)
    if (!compile_findCharOrDots (upperDots.chars[k], 1))
      {
	attr = CTC_Letter | CTC_UpperCase;
	upperCell = addCharOrDots (nested, upperDots.chars[k], 1);
	if (upperDots.length != 1)
	  attr = CTC_Space;
	upperCell->attributes |= attr;
	upperCell->uppercase = upperCell->realchar;
      }
  if (haveLowerDots)
    {
      for (k = 0; k < lowerDots.length; k++)
	if (!compile_findCharOrDots (lowerDots.chars[k], 1))
	  {
	    attr = CTC_Letter | CTC_LowerCase;
	    lowerCell = addCharOrDots (nested, lowerDots.chars[k], 1);
	    if (lowerDots.length != 1)
	      attr = CTC_Space;
	    lowerCell->attributes |= attr;
	    lowerCell->lowercase = lowerCell->realchar;
	  }
    }
  else if (upperCell != NULL && upperDots.length == 1)
    upperCell->attributes |= CTC_LowerCase;
  if (lowerDots.length == 1)
    putCharAndDots (nested, ruleChars.chars[1], lowerDots.chars[0]);
  if (upperCell != NULL)
    upperCell->lowercase = lowerDots.chars[0];
  if (lowerCell != NULL)
    lowerCell->uppercase = upperDots.chars[0];
  if (upperDots.length == 1)
    putCharAndDots (nested, ruleChars.chars[0], upperDots.chars[0]);
  ruleChars.length = 1;
  ruleChars.chars[2] = ruleChars.chars[0];
  ruleChars.chars[0] = ruleChars.chars[1];
  if (!addRule (nested, CTO_LowerCase, &ruleChars, &lowerDots, 0, 0))
    return 0;
  ruleChars.chars[0] = ruleChars.chars[2];
  if (!addRule (nested, CTO_UpperCase, &ruleChars, &upperDots, 0, 0))
    return 0;
  return 1;
}

/*Functions for compiling hyphenation tables*/

typedef struct			/*hyphenation dictionary: finite state machine */
{
  int numStates;
  HyphenationState *states;
} HyphenDict;

#define DEFAULTSTATE 0xffff
#define HYPHENHASHSIZE 8191

typedef struct
{
  void *next;
  CharsString *key;
  int val;
} HyphenHashEntry;

typedef struct
{
  HyphenHashEntry *entries[HYPHENHASHSIZE];
} HyphenHashTab;

/* a hash function from ASU - adapted from Gtk+ */
static unsigned int
hyphenStringHash (const CharsString * s)
{
  int k;
  unsigned int h = 0, g;
  for (k = 0; k < s->length; k++)
    {
      h = (h << 4) + s->chars[k];
      if ((g = h & 0xf0000000))
	{
	  h = h ^ (g >> 24);
	  h = h ^ g;
	}
    }
  return h;
}

static HyphenHashTab *
hyphenHashNew ()
{
  HyphenHashTab *hashTab;
  if (!(hashTab = malloc (sizeof (HyphenHashTab))))
    outOfMemory ();
  memset (hashTab, 0, sizeof (HyphenHashTab));
  return hashTab;
}

static void
hyphenHashFree (HyphenHashTab * hashTab)
{
  int i;
  HyphenHashEntry *e, *next;
  for (i = 0; i < HYPHENHASHSIZE; i++)
    for (e = hashTab->entries[i]; e; e = next)
      {
	next = e->next;
	free (e->key);
	free (e);
      }
  free (hashTab);
}

/* assumes that key is not already present! */
static void
hyphenHashInsert (HyphenHashTab * hashTab, const CharsString * key, int val)
{
  int i, j;
  HyphenHashEntry *e;
  i = hyphenStringHash (key) % HYPHENHASHSIZE;
  if (!(e = malloc (sizeof (HyphenHashEntry))))
    outOfMemory ();
  e->next = hashTab->entries[i];
  e->key = malloc ((key->length + 1) * CHARSIZE);
  if (!e->key)
    outOfMemory ();
  e->key->length = key->length;
  for (j = 0; j < key->length; j++)
    e->key->chars[j] = key->chars[j];
  e->val = val;
  hashTab->entries[i] = e;
}

/* return val if found, otherwise DEFAULTSTATE */
static int
hyphenHashLookup (HyphenHashTab * hashTab, const CharsString * key)
{
  int i, j;
  HyphenHashEntry *e;
  if (key->length == 0)
    return 0;
  i = hyphenStringHash (key) % HYPHENHASHSIZE;
  for (e = hashTab->entries[i]; e; e = e->next)
    {
      if (key->length != e->key->length)
	continue;
      for (j = 0; j < key->length; j++)
	if (key->chars[j] != e->key->chars[j])
	  break;
      if (j == key->length)
	return e->val;
    }
  return DEFAULTSTATE;
}

static int
hyphenGetNewState (HyphenDict * dict, HyphenHashTab * hashTab, const
		   CharsString * string)
{
  hyphenHashInsert (hashTab, string, dict->numStates);
  /* predicate is true if dict->numStates is a power of two */
  if (!(dict->numStates & (dict->numStates - 1)))
    dict->states = realloc (dict->states, (dict->numStates << 1) *
			    sizeof (HyphenationState));
  if (!dict->states)
    outOfMemory ();
  dict->states[dict->numStates].hyphenPattern = 0;
  dict->states[dict->numStates].fallbackState = DEFAULTSTATE;
  dict->states[dict->numStates].numTrans = 0;
  dict->states[dict->numStates].trans.pointer = NULL;
  return dict->numStates++;
}

/* add a transition from state1 to state2 through ch - assumes that the
   transition does not already exist */
static void
hyphenAddTrans (HyphenDict * dict, int state1, int state2, widechar ch)
{
  int numTrans;
  numTrans = dict->states[state1].numTrans;
  if (numTrans == 0)
    dict->states[state1].trans.pointer = malloc (sizeof (HyphenationTrans));
  else if (!(numTrans & (numTrans - 1)))
    dict->states[state1].trans.pointer = realloc
      (dict->states[state1].trans.pointer,
       (numTrans << 1) * sizeof (HyphenationTrans));
  dict->states[state1].trans.pointer[numTrans].ch = ch;
  dict->states[state1].trans.pointer[numTrans].newState = state2;
  dict->states[state1].numTrans++;
}

static int
compileHyphenation (FileInfo * nested, CharsString * encoding)
{
  CharsString hyph;
  HyphenationTrans *holdPointer;
  HyphenHashTab *hashTab;
  CharsString word;
  char pattern[MAXSTRING];
  unsigned int stateNum = 0, lastState = 0;
  int i, j, k = encoding->length;
  widechar ch;
  int found;
  HyphenHashEntry *e;
  HyphenDict dict;
  TranslationTableOffset holdOffset;
  /*Set aside enough space for hyphenation states and transitions in 
   * translation table. Must be done before anything else*/
  reserveSpaceInTable (nested, 250000);
  hashTab = hyphenHashNew ();
  dict.numStates = 1;
  dict.states = malloc (sizeof (HyphenationState));
  if (!dict.states)
    outOfMemory ();
  dict.states[0].hyphenPattern = 0;
  dict.states[0].fallbackState = DEFAULTSTATE;
  dict.states[0].numTrans = 0;
  dict.states[0].trans.pointer = NULL;
  do
    {
      if (encoding->chars[0] == 'I')
	{
	  if (!getToken (nested, &hyph, NULL))
	    continue;
	}
      else
	{
	  /*UTF-8 */
	  if (!getToken (nested, &word, NULL))
	    continue;
	  parseChars (nested, &hyph, &word);
	}
      if (hyph.length == 0 || hyph.chars[0] == '#' || hyph.chars[0] ==
	  '%' || hyph.chars[0] == '<')
	continue;		/*comment */
      for (i = 0; i < hyph.length; i++)
	definedCharOrDots (nested, hyph.chars[i], 0);
      j = 0;
      pattern[j] = '0';
      for (i = 0; i < hyph.length; i++)
	{
	  if (hyph.chars[i] >= '0' && hyph.chars[i] <= '9')
	    pattern[j] = (char) hyph.chars[i];
	  else
	    {
	      word.chars[j] = hyph.chars[i];
	      pattern[++j] = '0';
	    }
	}
      word.chars[j] = 0;
      word.length = j;
      pattern[j + 1] = 0;
      for (i = 0; pattern[i] == '0'; i++);
      found = hyphenHashLookup (hashTab, &word);
      if (found != DEFAULTSTATE)
	stateNum = found;
      else
	stateNum = hyphenGetNewState (&dict, hashTab, &word);
      k = j + 2 - i;
      if (k > 0)
	{
	  allocateSpaceInTable (nested,
				&dict.states[stateNum].hyphenPattern, k);
	  memcpy (&table->ruleArea[dict.states[stateNum].hyphenPattern],
		  &pattern[i], k);
	}
      /* now, put in the prefix transitions */
      while (found == DEFAULTSTATE)
	{
	  lastState = stateNum;
	  ch = word.chars[word.length-- - 1];
	  found = hyphenHashLookup (hashTab, &word);
	  if (found != DEFAULTSTATE)
	    stateNum = found;
	  else
	    stateNum = hyphenGetNewState (&dict, hashTab, &word);
	  hyphenAddTrans (&dict, stateNum, lastState, ch);
	}
    }
  while (getALine (nested));
  /* put in the fallback states */
  for (i = 0; i < HYPHENHASHSIZE; i++)
    {
      for (e = hashTab->entries[i]; e; e = e->next)
	{
	  for (j = 1; j <= e->key->length; j++)
	    {
	      word.length = 0;
	      for (k = j; k < e->key->length; k++)
		word.chars[word.length++] = e->key->chars[k];
	      stateNum = hyphenHashLookup (hashTab, &word);
	      if (stateNum != DEFAULTSTATE)
		break;
	    }
	  if (e->val)
	    dict.states[e->val].fallbackState = stateNum;
	}
    }
  hyphenHashFree (hashTab);
/*Transfer hyphenation information to table*/
  for (i = 0; i < dict.numStates; i++)
    {
      if (dict.states[i].numTrans == 0)
	dict.states[i].trans.offset = 0;
      else
	{
	  holdPointer = dict.states[i].trans.pointer;
	  allocateSpaceInTable (nested,
				&dict.states[i].trans.offset,
				dict.states[i].numTrans *
				sizeof (HyphenationTrans));
	  memcpy (&table->ruleArea[dict.states[i].trans.offset],
		  holdPointer,
		  dict.states[i].numTrans * sizeof (HyphenationTrans));
	  free (holdPointer);
	}
    }
  allocateSpaceInTable (nested,
			&holdOffset, dict.numStates *
			sizeof (HyphenationState));
  table->hyphenStatesArray = holdOffset;
  /* Prevents segmentajion fault if table is reallocated */
  memcpy (&table->ruleArea[table->hyphenStatesArray], &dict.states[0],
	  dict.numStates * sizeof (HyphenationState));
  free (dict.states);
  return 1;
}

static int
compileNoBreak (FileInfo * nested)
{
  int k;
  CharsString ruleDots;
  CharsString otherDots;
  CharsString dotsBefore;
  CharsString dotsAfter;
  int haveDotsAfter = 0;
  if (!getToken (nested, &ruleDots, "dots operand"))
    return 0;
  for (k = 0; k < ruleDots.length && ruleDots.chars[k] != ','; k++);
  if (k == ruleDots.length)
    {
      if (!parseDots (nested, &dotsBefore, &ruleDots))
	return 0;
      dotsAfter.length = dotsBefore.length;
      for (k = 0; k < dotsBefore.length; k++)
	dotsAfter.chars[k] = dotsBefore.chars[k];
      dotsAfter.chars[k] = 0;
    }
  else
    {
      haveDotsAfter = ruleDots.length;
      ruleDots.length = k;
      if (!parseDots (nested, &dotsBefore, &ruleDots))
	return 0;
      otherDots.length = 0;
      k++;
      for (; k < haveDotsAfter; k++)
	otherDots.chars[otherDots.length++] = ruleDots.chars[k];
      if (!parseDots (nested, &dotsAfter, &otherDots))
	return 0;
    }
  for (k = 0; k < dotsBefore.length; k++)
    dotsBefore.chars[k] = getCharFromDots (dotsBefore.chars[k]);
  for (k = 0; k < dotsAfter.length; k++)
    dotsAfter.chars[k] = getCharFromDots (dotsAfter.chars[k]);
  if (!addRule (nested, CTO_NoBreak, &dotsBefore, &dotsAfter, 0, 0))
    return 0;
  table->noBreak = newRuleOffset;
  return 1;
}

static int
compileCharDef (FileInfo * nested,
		TranslationTableOpcode opcode,
		TranslationTableCharacterAttributes attributes)
{
  CharsString ruleChars;
  CharsString ruleDots;
  TranslationTableCharacter *character;
  TranslationTableCharacter *cell;
  TranslationTableCharacter *otherCell;
  TranslationTableCharacterAttributes attr;
  int k;
  if (!getRuleCharsText (nested, &ruleChars))
    return 0;
  if (attributes & (CTC_UpperCase | CTC_LowerCase))
    attributes |= CTC_Letter;
  if (!getRuleDotsPattern (nested, &ruleDots))
    return 0;
  if (ruleChars.length != 1 || ruleDots.length < 1)
    {
      compileError (nested,
		    "Exactly one Unicode character and at least one cell are required.");
      return 0;
    }
  character = addCharOrDots (nested, ruleChars.chars[0], 0);
  character->attributes |= attributes;
  character->uppercase = character->lowercase = character->realchar;
  cell = compile_findCharOrDots (ruleDots.chars[0], 1);
  if (ruleDots.length == 1 && cell)
    cell->attributes |= attributes;
  else
    {
      for (k = 0; k < ruleDots.length; k++)
	{
	  if (!compile_findCharOrDots (ruleDots.chars[k], 1))
	    {
	      attr = attributes;
	      otherCell = addCharOrDots (nested, ruleDots.chars[k], 1);
	      if (ruleDots.length != 1)
		attr = CTC_Space;
	      otherCell->attributes |= attr;
	      otherCell->uppercase = otherCell->lowercase =
		otherCell->realchar;
	    }
	}
    }
  if (!addRule (nested, opcode, &ruleChars, &ruleDots, 0, 0))
    return 0;
  if (ruleDots.length == 1)
    putCharAndDots (nested, ruleChars.chars[0], ruleDots.chars[0]);
  return 1;
}

static int
compileRule (FileInfo * nested)
{
  int ok = 1;
  CharsString token;
  TranslationTableOpcode opcode;
  CharsString ruleChars;
  CharsString ruleDots;
  CharsString cells;
  CharsString scratchPad;
  TranslationTableCharacterAttributes after = 0;
  TranslationTableCharacterAttributes before = 0;
  int k;

  noback = nofor = 0;
doOpcode:
  if (!getToken (nested, &token, NULL))
    return 1;			/*blank line */
  if (token.chars[0] == '#' || token.chars[0] == '<')
    return 1;			/*comment */
  if (nested->lineNumber == 1 && (eqasc2uni ((unsigned char *) "ISO",
					     token.chars, 3) ||
				  eqasc2uni ((unsigned char *) "UTF-8",
					     token.chars, 5)))
    {
      compileHyphenation (nested, &token);
      return 1;
    }
  opcode = getOpcode (nested, &token);
  switch (opcode)
    {				/*Carry out operations */
    case CTO_None:
      break;
    case CTO_IncludeFile:
      {
	CharsString includedFile;
	if (getToken (nested, &token, "include file name"))
	  if (parseChars (nested, &includedFile, &token))
	    if (!includeFile (nested, &includedFile))
	      ok = 0;
	break;
      }
    case CTO_Locale:
      break;
    case CTO_Undefined:
      ok =
	compileBrailleIndicator (nested, "undefined character opcode",
				 CTO_Undefined, &table->undefined);
      break;
    case CTO_CapitalSign:
      ok =
	compileBrailleIndicator (nested, "capital sign", CTO_CapitalRule,
				 &table->capitalSign);
      break;
    case CTO_BeginCapitalSign:
      ok =
	compileBrailleIndicator (nested, "begin capital sign",
				 CTO_BeginCapitalRule,
				 &table->beginCapitalSign);
      break;
    case CTO_LenBegcaps:
      ok = table->lenBeginCaps = compileNumber (nested);
      break;
    case CTO_EndCapitalSign:
      ok =
	compileBrailleIndicator (nested, "end capitals sign",
				 CTO_EndCapitalRule, &table->endCapitalSign);
      break;
    case CTO_FirstWordCaps:
      ok =
	compileBrailleIndicator (nested, "first word capital sign",
				 CTO_FirstWordCapsRule,
				 &table->firstWordCaps);
      break;
    case CTO_LastWordCapsBefore:
      ok =
	compileBrailleIndicator (nested, "capital sign before last word",
				 CTO_LastWordCapsBeforeRule,
				 &table->lastWordCapsBefore);
      break;
    case CTO_LastWordCapsAfter:
      ok =
	compileBrailleIndicator (nested, "capital sign after last word",
				 CTO_LastWordCapsAfterRule,
				 &table->lastWordCapsAfter);
      break;
    case CTO_LenCapsPhrase:
      ok = table->lenCapsPhrase = compileNumber (nested);
      break;
    case CTO_LetterSign:
      ok =
	compileBrailleIndicator (nested, "letter sign", CTO_LetterRule,
				 &table->letterSign);
      break;
    case CTO_NoLetsignBefore:
      if (getRuleCharsText (nested, &ruleChars))
	{
	  if ((table->noLetsignBeforeCount + ruleChars.length) > LETSIGNSIZE)
	    {
	      compileError (nested, "More than %d characters", LETSIGNSIZE);
	      ok = 0;
	      break;
	    }
	  for (k = 0; k < ruleChars.length; k++)
	    table->noLetsignBefore[table->noLetsignBeforeCount++] =
	      ruleChars.chars[k];
	}
      break;
    case CTO_NoLetsign:
      if (getRuleCharsText (nested, &ruleChars))
	{
	  if ((table->noLetsignCount + ruleChars.length) > LETSIGNSIZE)
	    {
	      compileError (nested, "More than %d characters", LETSIGNSIZE);
	      ok = 0;
	      break;
	    }
	  for (k = 0; k < ruleChars.length; k++)
	    table->noLetsign[table->noLetsignCount++] = ruleChars.chars[k];
	}
      break;
    case CTO_NoLetsignAfter:
      if (getRuleCharsText (nested, &ruleChars))
	{
	  if ((table->noLetsignAfterCount + ruleChars.length) > LETSIGNSIZE)
	    {
	      compileError (nested, "More than %d characters", LETSIGNSIZE);
	      ok = 0;
	      break;
	    }
	  for (k = 0; k < ruleChars.length; k++)
	    table->noLetsignAfter[table->noLetsignAfterCount++] =
	      ruleChars.chars[k];
	}
      break;
    case CTO_NumberSign:
      ok =
	compileBrailleIndicator (nested, "number sign", CTO_NumberRule,
				 &table->numberSign);
      break;
    case CTO_FirstWordItal:
      ok =
	compileBrailleIndicator (nested, "first word italic",
				 CTO_FirstWordItalRule,
				 &table->firstWordItal);
      break;
    case CTO_ItalSign:
    case CTO_LastWordItalBefore:
      ok =
	compileBrailleIndicator (nested, "first word italic before",
				 CTO_LastWordItalBeforeRule,
				 &table->lastWordItalBefore);
      break;
    case CTO_LastWordItalAfter:
      ok =
	compileBrailleIndicator (nested, "last word italic after",
				 CTO_LastWordItalAfterRule,
				 &table->lastWordItalAfter);
      break;
    case CTO_BegItal:
    case CTO_FirstLetterItal:
      ok =
	compileBrailleIndicator (nested, "first letter italic",
				 CTO_FirstLetterItalRule,
				 &table->firstLetterItal);
      break;
    case CTO_EndItal:
    case CTO_LastLetterItal:
      ok =
	compileBrailleIndicator (nested, "last letter italic",
				 CTO_LastLetterItalRule,
				 &table->lastLetterItal);
      break;
    case CTO_SingleLetterItal:
      ok =
	compileBrailleIndicator (nested, "single letter italic",
				 CTO_SingleLetterItalRule,
				 &table->singleLetterItal);
      break;
    case CTO_ItalWord:
      ok =
	compileBrailleIndicator (nested, "italic word", CTO_ItalWordRule,
				 &table->italWord);
      break;
    case CTO_LenItalPhrase:
      ok = table->lenItalPhrase = compileNumber (nested);
      break;
    case CTO_FirstWordBold:
      ok =
	compileBrailleIndicator (nested, "first word bold",
				 CTO_FirstWordBoldRule,
				 &table->firstWordBold);
      break;
    case CTO_BoldSign:
    case CTO_LastWordBoldBefore:
      ok =
	compileBrailleIndicator (nested, "last word bold before",
				 CTO_LastWordBoldBeforeRule,
				 &table->lastWordBoldBefore);
      break;
    case CTO_LastWordBoldAfter:
      ok =
	compileBrailleIndicator (nested, "last word bold after",
				 CTO_LastWordBoldAfterRule,
				 &table->lastWordBoldAfter);
      break;
    case CTO_BegBold:
    case CTO_FirstLetterBold:
      ok =
	compileBrailleIndicator (nested, "first  letter bold",
				 CTO_FirstLetterBoldRule,
				 &table->firstLetterBold);
      break;
    case CTO_EndBold:
    case CTO_LastLetterBold:
      ok =
	compileBrailleIndicator (nested, "last letter bold",
				 CTO_LastLetterBoldRule,
				 &table->lastLetterBold);
      break;
    case CTO_SingleLetterBold:
      ok =
	compileBrailleIndicator (nested, "single  letter bold",
				 CTO_SingleLetterBoldRule,
				 &table->singleLetterBold);
      break;
    case CTO_BoldWord:
      ok =
	compileBrailleIndicator (nested, "bold word", CTO_BoldWordRule,
				 &table->boldWord);
      break;
    case CTO_LenBoldPhrase:
      ok = table->lenBoldPhrase = compileNumber (nested);
      break;
    case CTO_FirstWordUnder:
      ok =
	compileBrailleIndicator (nested, "first word  underline",
				 CTO_FirstWordUnderRule,
				 &table->firstWordUnder);
      break;
    case CTO_UnderSign:
    case CTO_LastWordUnderBefore:
      ok =
	compileBrailleIndicator (nested, "last word underline before",
				 CTO_LastWordUnderBeforeRule,
				 &table->lastWordUnderBefore);
      break;
    case CTO_LastWordUnderAfter:
      ok =
	compileBrailleIndicator (nested, "last  word underline after",
				 CTO_LastWordUnderAfterRule,
				 &table->lastWordUnderAfter);
      break;
    case CTO_BegUnder:
    case CTO_FirstLetterUnder:
      ok =
	compileBrailleIndicator (nested, "first letter underline",
				 CTO_FirstLetterUnderRule,
				 &table->firstLetterUnder);
      break;
    case CTO_EndUnder:
    case CTO_LastLetterUnder:
      ok =
	compileBrailleIndicator (nested, "last letter underline",
				 CTO_LastLetterUnderRule,
				 &table->lastLetterUnder);
      break;
    case CTO_SingleLetterUnder:
      ok =
	compileBrailleIndicator (nested, "single letter underline",
				 CTO_SingleLetterUnderRule,
				 &table->singleLetterUnder);
      break;
    case CTO_UnderWord:
      ok =
	compileBrailleIndicator (nested, "underlined word", CTO_UnderWordRule,
				 &table->underWord);
      break;
    case CTO_LenUnderPhrase:
      ok = table->lenUnderPhrase = compileNumber (nested);
      break;
    case CTO_BegComp:
      ok =
	compileBrailleIndicator (nested, "begin computer braille",
				 CTO_BegCompRule, &table->begComp);
      break;
    case CTO_EndComp:
      ok =
	compileBrailleIndicator (nested, "end computer braslle",
				 CTO_EndCompRule, &table->endComp);
      break;
    case CTO_Syllable:
      table->syllables = 1;
    case CTO_Always:
    case CTO_NoCross:
    case CTO_LargeSign:
    case CTO_WholeWord:
    case CTO_PartWord:
    case CTO_JoinNum:
    case CTO_JoinableWord:
    case CTO_LowWord:
    case CTO_SuffixableWord:
    case CTO_PrefixableWord:
    case CTO_BegWord:
    case CTO_BegMidWord:
    case CTO_MidWord:
    case CTO_MidEndWord:
    case CTO_EndWord:
    case CTO_PrePunc:
    case CTO_PostPunc:
    case CTO_BegNum:
    case CTO_MidNum:
    case CTO_EndNum:
    case CTO_Repeated:
    case CTO_RepWord:
      if (getRuleCharsText (nested, &ruleChars))
	if (getRuleDotsPattern (nested, &ruleDots))
	  if (!addRule (nested, opcode, &ruleChars, &ruleDots, after, before))
	    ok = 0;
      break;
    case CTO_CompDots:
    case CTO_Comp6:
      if (!getRuleCharsText (nested, &ruleChars))
	return 0;
      if (ruleChars.length != 1 || ruleChars.chars[0] > 255)
	{
	  compileError (nested,
			"first operand must be 1 character and < 256");
	  return 0;
	}
      if (!getRuleDotsPattern (nested, &ruleDots))
	return 0;
      if (!addRule (nested, opcode, &ruleChars, &ruleDots, after, before))
	ok = 0;
      table->compdotsPattern[ruleChars.chars[0]] = newRuleOffset;
      break;
    case CTO_ExactDots:
      if (!getRuleCharsText (nested, &ruleChars))
	return 0;
      if (ruleChars.chars[0] != '@')
	{
	  compileError (nested, "The operand must begin with an at sign (@)");
	  return 0;
	}
      for (k = 1; k < ruleChars.length; k++)
	scratchPad.chars[k - 1] = ruleChars.chars[k];
      scratchPad.length = ruleChars.length - 1;
      if (!parseDots (nested, &ruleDots, &scratchPad))
	return 0;
      if (!addRule (nested, opcode, &ruleChars, &ruleDots, before, after))
	ok = 0;
      break;
    case CTO_CapsNoCont:
      ruleChars.length = 1;
      ruleChars.chars[0] = 'a';
      if (!addRule
	  (nested, CTO_CapsNoContRule, &ruleChars, NULL, after, before))
	ok = 0;
      table->capsNoCont = newRuleOffset;
      break;
    case CTO_Replace:
      if (getRuleCharsText (nested, &ruleChars))
	{
	  if (lastToken)
	    ruleDots.length = ruleDots.chars[0] = 0;
	  else
	    {
	      getRuleDotsText (nested, &ruleDots);
	      if (ruleDots.chars[0] == '#')
		ruleDots.length = ruleDots.chars[0] = 0;
	      else if (ruleDots.chars[0] == '\\' && ruleDots.chars[1] == '#')
		memcpy (&ruleDots.chars[0], &ruleDots.chars[1],
			ruleDots.length-- * CHARSIZE);
	    }
	}
      for (k = 0; k < ruleChars.length; k++)
	addCharOrDots (nested, ruleChars.chars[k], 0);
      for (k = 0; k < ruleDots.length; k++)
	addCharOrDots (nested, ruleDots.chars[k], 0);
      if (!addRule (nested, opcode, &ruleChars, &ruleDots, after, before))
	ok = 0;
      break;
    case CTO_Pass2:
      if (table->numPasses < 2)
	table->numPasses = 2;
      goto doPass;
    case CTO_Pass3:
      if (table->numPasses < 3)
	table->numPasses = 3;
      goto doPass;
    case CTO_Pass4:
      if (table->numPasses < 4)
	table->numPasses = 4;
    doPass:
    case CTO_Context:
      if (!compilePassOpcode (nested, opcode))
	ok = 0;
      break;
    case CTO_Correct:
      if (!compilePassOpcode (nested, opcode))
	ok = 0;
      table->corrections = 1;
      break;
    case CTO_Contraction:
    case CTO_NoCont:
    case CTO_CompBrl:
    case CTO_Literal:
      if (getRuleCharsText (nested, &ruleChars))
	if (!addRule (nested, opcode, &ruleChars, NULL, after, before))
	  ok = 0;
      break;
    case CTO_MultInd:
      {
	int lastToken;
	ruleChars.length = 0;
	if (getToken (nested, &token, "multiple braille indicators") &&
	    parseDots (nested, &cells, &token))
	  {
	    while ((lastToken = getToken (nested, &token, "multind opcodes")))
	      {
		opcode = getOpcode (nested, &token);
		if (opcode >= CTO_CapitalSign && opcode < CTO_MultInd)
		  ruleChars.chars[ruleChars.length++] = (widechar) opcode;
		else
		  {
		    compileError (nested, "Not a braille indicator opcode.");
		    ok = 0;
		  }
		if (lastToken == 2)
		  break;
	      }
	  }
	else
	  ok = 0;
	if (!addRule (nested, CTO_MultInd, &ruleChars, &cells, after, before))
	  ok = 0;
	break;
      }

    case CTO_Class:
      {
	CharsString characters;
	const struct CharacterClass *class;
	if (!characterClasses)
	  {
	    if (!allocateCharacterClasses ())
	      ok = 0;
	  }
	if (getToken (nested, &token, "character class name"))
	  {
	    if ((class = findCharacterClass (&token)))
	      {
		compileError (nested, "character class already defined.");
	      }
	    else
	      if ((class =
		   addCharacterClass (nested, &token.chars[0], token.length)))
	      {
		if (getCharacters (nested, &characters))
		  {
		    int index;
		    for (index = 0; index < characters.length; ++index)
		      {
			TranslationTableRule *defRule;
			TranslationTableCharacter *character =
			  definedCharOrDots
			  (nested, characters.chars[index], 0);
			character->attributes |= class->attribute;
			defRule = (TranslationTableRule *)
			  & table->ruleArea[character->definitionRule];
			if (defRule->dotslen == 1)
			  {
			    character = definedCharOrDots
			      (nested,
			       defRule->charsdots[defRule->charslen], 1);
			    character->attributes |= class->attribute;
			  }
		      }
		  }
	      }
	  }
	break;
      }

      {
	TranslationTableCharacterAttributes *attributes;
	const struct CharacterClass *class;
    case CTO_After:
	attributes = &after;
	goto doClass;
    case CTO_Before:
	attributes = &before;
      doClass:

	if (!characterClasses)
	  {
	    if (!allocateCharacterClasses ())
	      ok = 0;
	  }
	if (getCharacterClass (nested, &class))
	  {
	    *attributes |= class->attribute;
	    goto doOpcode;
	  }
	break;
      }
    case CTO_NoBack:
      noback = 1;
      goto doOpcode;
    case CTO_NoFor:
      nofor = 1;
      goto doOpcode;
    case CTO_SwapCc:
    case CTO_SwapCd:
    case CTO_SwapDd:
      if (!compileSwap (nested, opcode))
	ok = 0;
      break;
    case CTO_Hyphen:
    case CTO_DecPoint:
      if (getRuleCharsText (nested, &ruleChars))
	if (getRuleDotsPattern (nested, &ruleDots))
	  {
	    if (ruleChars.length != 1 || ruleDots.length < 1)
	      {
		compileError (nested,
			      "One Unicode character and at least one cell are required.");
		ok = 0;
	      }
	    if (!addRule
		(nested, opcode, &ruleChars, &ruleDots, after, before))
	      ok = 0;
	  }
      break;
    case CTO_Space:
      compileCharDef (nested, opcode, CTC_Space);
      break;
    case CTO_Digit:
      compileCharDef (nested, opcode, CTC_Digit);
      break;
    case CTO_LitDigit:
      compileCharDef (nested, opcode, CTC_LitDigit);
      break;
    case CTO_Punctuation:
      compileCharDef (nested, opcode, CTC_Punctuation);
      break;
    case CTO_Math:
      compileCharDef (nested, opcode, CTC_Math);
      break;
    case CTO_Sign:
      compileCharDef (nested, opcode, CTC_Sign);
      break;
    case CTO_Letter:
      compileCharDef (nested, opcode, CTC_Letter);
      break;
    case CTO_UpperCase:
      compileCharDef (nested, opcode, CTC_UpperCase);
      break;
    case CTO_LowerCase:
      compileCharDef (nested, opcode, CTC_LowerCase);
      break;
    case CTO_NoBreak:
      ok = compileNoBreak (nested);
      break;
    case CTO_Grouping:
      ok = compileGrouping (nested);
      break;
    case CTO_UpLow:
      ok = compileUplow (nested);
      break;
    case CTO_Display:
      if (getRuleCharsText (nested, &ruleChars))
	if (getRuleDotsPattern (nested, &ruleDots))
	  {
	    if (ruleChars.length != 1 || ruleDots.length != 1)
	      {
		compileError (nested,
			      "Exactly one character and one cell are required.");
		ok = 0;
	      }
	    putCharAndDots (nested, ruleChars.chars[0], ruleDots.chars[0]);
	  }
      break;
    default:
      compileError (nested, "unimplemented opcode.");
      break;
    }
  return ok;
}

int EXPORT_CALL
lou_readCharFromFile (const char *fileName, int *mode)
{
/*Read a character from a file, whether big-endian, little-endian or 
* ASCII8*/
  int ch;
  static FileInfo nested;
  if (fileName == NULL)
    return 0;
  if (*mode == 1)
    {
      *mode = 0;
      nested.fileName = fileName;
      nested.encoding = noEncoding;
      nested.status = 0;
      nested.lineNumber = 0;
      if (!(nested.in = fopen (nested.fileName, "r")))
	{
	  logMessage (LOG_ERROR, "Cannot open file '%s'", nested.fileName);
	  *mode = 1;
	  return EOF;
	}
    }
  if (nested.in == NULL)
    {
      *mode = 1;
      return EOF;
    }
  ch = getAChar (&nested);
  if (ch == EOF)
    {
      fclose (nested.in);
      nested.in = NULL;
      *mode = 1;
    }
  return ch;
}

static int
compileString (const char *inString)
{
/* This function can be used to make changes to tables on the fly. */
  int k;
  FileInfo nested;
  if (inString == NULL)
    return 0;
  nested.fileName = inString;
  nested.encoding = noEncoding;
  nested.lineNumber = 1;
  nested.status = 0;
  nested.linepos = 0;
  for (k = 0; inString[k]; k++)
    nested.line[k] = inString[k];
  nested.line[k] = 0;
  return compileRule (&nested);
}

static int
makeDoubleRule (TranslationTableOpcode opcode, TranslationTableOffset
		* singleRule, TranslationTableOffset * doubleRule)
{
  CharsString dots;
  TranslationTableRule *rule;
  if (!*singleRule || *doubleRule)
    return 1;
  rule = (TranslationTableRule *) & table->ruleArea[*singleRule];
  memcpy (dots.chars, &rule->charsdots[0], rule->dotslen * CHARSIZE);
  memcpy (&dots.chars[rule->dotslen], &rule->charsdots[0],
	  rule->dotslen * CHARSIZE);
  dots.length = 2 * rule->dotslen;
  if (!addRule (NULL, opcode, NULL, &dots, 0, 0))
    return 0;
  *doubleRule = newRuleOffset;
  return 1;
}

static int
setDefaults ()
{
  if (!table->lenBeginCaps)
    table->lenBeginCaps = 2;
  makeDoubleRule (CTO_FirstWordItal, &table->lastWordItalBefore,
		  &table->firstWordItal);
  if (!table->lenItalPhrase)
    table->lenItalPhrase = 4;
  makeDoubleRule (CTO_FirstWordBold, &table->lastWordBoldBefore,
		  &table->firstWordBold);
  if (!table->lenBoldPhrase)
    table->lenBoldPhrase = 4;
  makeDoubleRule (CTO_FirstWordUnder, &table->lastWordUnderBefore,
		  &table->firstWordUnder);
  if (!table->lenUnderPhrase)
    table->lenUnderPhrase = 4;
  if (table->numPasses == 0)
    table->numPasses = 1;
  return 1;
}

/* =============== *
 * TABLE RESOLVING *
 * =============== *
 *
 * A table resolver is a function that resolves a `tableList` path against a
 * `base` path, and returns the resolved table(s) as a list of absolute file
 * paths.
 *
 * The function must have the following signature:
 *
 *     char ** (const char * tableList, const char * base)
 *
 * In general, `tableList` is a path in the broad sense. The default
 * implementation accepts only *file* paths. But another implementation could
 * for instance handle URI's. `base` is always a file path however.
 *
 * The idea is to give other programs that use liblouis the ability to define
 * their own table resolver (in C, Java, Python, etc.) when the default
 * resolver is not satisfying. (see also lou_registerTableResolver)
 *
 */

/**
 * Resolve a single (sub)table.
 * 
 * Tries to resolve `table` against `base` if base is an absolute path. If
 * that fails, searches `searchPath`.
 *
 */
static char *
resolveSubtable (const char *table, const char *base, const char *searchPath)
{
  char *tableFile;
  static struct stat info;
  
  if (table == NULL || table[0] == '\0')
    return NULL;
  tableFile = (char *) malloc (MAXSTRING * sizeof(char));
  
  //
  // First try to resolve against base
  //
  if (base)
    {
      int k;
      strcpy (tableFile, base);
      for (k = strlen (tableFile); k >= 0 && tableFile[k] != DIR_SEP; k--)
	;
      tableFile[++k] = '\0';
      strcat (tableFile, table);
      if (stat (tableFile, &info) == 0 && !(info.st_mode & S_IFDIR))
	return tableFile;
    }
  
  //
  // It could be an absolute path, or a path relative to the current working
  // directory
  //
  strcpy (tableFile, table);
  if (stat (tableFile, &info) == 0 && !(info.st_mode & S_IFDIR))
    return tableFile;
  
  //
  // Then search `LOUIS_TABLEPATH`, `dataPath` and `programPath`
  //
  if (searchPath[0] != '\0')
    {
      char *dir;
      int last;
      char *cp;
      char *searchPath_copy = strdup (searchPath + 1);
      for (dir = searchPath_copy; ; dir = cp + 1)
	{
	  for (cp = dir; *cp != '\0' && *cp != ','; cp++)
	    ;
	  last = (*cp == '\0');
	  *cp = '\0';
	  if (dir == cp)
	    dir = ".";
	  sprintf (tableFile, "%s%c%s", dir, DIR_SEP, table);
	  if (stat (tableFile, &info) == 0 && !(info.st_mode & S_IFDIR)) 
	    {
	      free(searchPath_copy);
	      return tableFile;
	    }
	  if (last)
	    break;
	}
      free(searchPath_copy);
    }
  free (tableFile);
  return NULL;
}

/**
 * The default table resolver
 *
 * Tries to resolve tableList against base. The search path is set to
 * `LOUIS_TABLEPATH`, `dataPath` and `programPath` (in that order).
 *
 * @param table A file path, may be absolute or relative. May be a list of
 *              tables separated by comma's. In that case, the first table
 *              is used as the base for the other subtables.
 * @param base A file path or directory path, or NULL.
 * @return The file paths of the resolved subtables, or NULL if the table
 *         could not be resolved.
 *
 */
static char **
defaultTableResolver (const char *tableList, const char *base)
{
  char searchPath[MAXSTRING];
  char **tableFiles;
  char *subTable;
  char *tableList_copy;
  char *cp;
  char *path;
  int last;
  int k;
  
  /* Set up search path */
  cp = searchPath;
  path = getenv ("LOUIS_TABLEPATH");
  if (path != NULL && path[0] != '\0')
    cp += sprintf (cp, ",%s", path);
  path = lou_getDataPath ();
  if (path != NULL && path[0] != '\0')
    cp += sprintf (cp, ",%s%c%s%c%s", path, DIR_SEP, "liblouis", DIR_SEP,
		   "tables");
#ifdef _WIN32
  path = lou_getProgramPath ();
  if (path != NULL && path[0] != '\0')
    cp += sprintf (cp, ",%s%s", path, "\\share\\liblouis\\tables");
#else
  cp += sprintf (cp, ",%s", TABLESDIR);
#endif
  
  /* Count number of subtables in table list */
  k = 0;
  for (cp = (char *)tableList; *cp != '\0'; cp++)
    if (*cp == ',')
      k++;
  tableFiles = (char **) malloc ((k + 2) * sizeof(char *));
  
  /* Resolve subtables */
  k = 0;
  tableList_copy = strdup (tableList);
  for (subTable = tableList_copy; ; subTable = cp + 1)
    {
      for (cp = subTable; *cp != '\0' && *cp != ','; cp++);
      last = (*cp == '\0');
      *cp = '\0';
      if (!(tableFiles[k++] = resolveSubtable (subTable, base, searchPath)))
	{
	  logMessage (LOG_ERROR, "Cannot resolve table '%s'", subTable);
	  free(tableList_copy);
	  free (tableFiles);
	  return NULL;
	}
      if (k == 1)
	base = subTable;
      if (last)
	break;
    }
  free(tableList_copy);
  tableFiles[k] = NULL;
  return tableFiles;
}

static char ** (* tableResolver) (const char *tableList, const char *base) =
  &defaultTableResolver;

static char **
resolveTable (const char *tableList, const char *base)
{
  return (*tableResolver) (tableList, base);
}

/**
 * Register a new table resolver. Overrides the default resolver.
 *
 * @param resolver The new resolver as a function pointer.
 *
 */
void EXPORT_CALL
lou_registerTableResolver (char ** (* resolver) (const char *tableList, const char *base))
{
  tableResolver = resolver;
}

static int fileCount = 0;

/**
 * Compile a single file
 *
 */
static int
compileFile (const char *fileName)
{
  FileInfo nested;
  fileCount++;
  nested.fileName = fileName;
  nested.encoding = noEncoding;
  nested.status = 0;
  nested.lineNumber = 0;
  if ((nested.in = fopen (nested.fileName, "rb")))
    {
      while (getALine (&nested))
	compileRule (&nested);
      fclose (nested.in);
      return 1;
    }
  else
    logMessage (LOG_ERROR, "Cannot open table '%s'", nested.fileName);
  errorCount++;
  return 0;
}

/** 
 * Free a char** array 
 */
static void 
free_tablefiles(char **tables) {
  char **table;
  if (!tables) return;
  for (table = tables; *table; table++)
    free(*table);
  free(tables);
}

/**
 * Implement include opcode
 *
 */
static int
includeFile (FileInfo * nested, CharsString * includedFile)
{
  int k;
  char includeThis[MAXSTRING];
  char **tableFiles;
  int rv;
  for (k = 0; k < includedFile->length; k++)
    includeThis[k] = (char) includedFile->chars[k];
  includeThis[k] = 0;
  tableFiles = resolveTable (includeThis, nested->fileName);
  if (tableFiles == NULL)
    {
      errorCount++;
      return 0;
    }
  if (tableFiles[1] != NULL)
    {
      errorCount++;
      free_tablefiles(tableFiles);
      logMessage (LOG_ERROR, "Table list not supported in include statement: 'include %s'", includeThis);
      return 0;
    }
  rv = compileFile (*tableFiles);
  free_tablefiles(tableFiles);
  return rv;
}

/**
 * Compile source tables into a table in memory
 *
 */
static void *
compileTranslationTable (const char *tableList)
{
  char **tableFiles;
  char **subTable;
  errorCount = warningCount = fileCount = 0;
  table = NULL;
  characterClasses = NULL;
  ruleNames = NULL;
  if (tableList == NULL)
    return NULL;
  if (!opcodeLengths[0])
    {
      TranslationTableOpcode opcode;
      for (opcode = 0; opcode < CTO_None; opcode++)
	opcodeLengths[opcode] = strlen (opcodeNames[opcode]);
    }
  allocateHeader (NULL);
  /* Compile things that are necesary for the proper operation of 
     liblouis or liblouisxml or liblouisutdml */
  compileString ("space \\s 0");
  compileString ("noback sign \\x0000 0");
  compileString ("space \\x00a0 a unbreakable space");
  compileString ("space \\x001b 1b escape");
  compileString ("space \\xffff 123456789abcdef ENDSEGMENT");
  
  /* Compile all subtables in the list */
  if (!(tableFiles = resolveTable (tableList, NULL)))
    {
      errorCount++;
      goto cleanup;
    }
  for (subTable = tableFiles; *subTable; subTable++)
    if (!compileFile (*subTable))
      goto cleanup;
  
/* Clean up after compiling files */
cleanup:
  free_tablefiles(tableFiles);
  if (characterClasses)
    deallocateCharacterClasses ();
  if (ruleNames)
    deallocateRuleNames ();
  if (warningCount)
    logMessage (LOG_WARN, "%d warnings issued", warningCount);
  if (!errorCount)
    {
      setDefaults ();
      table->tableSize = tableSize;
      table->bytesUsed = tableUsed;
    }
  else
    {
      logMessage (LOG_ERROR, "%d errors found.", errorCount);
      if (table)
	free (table);
      table = NULL;
    }
  return (void *) table;
}

static ChainEntry *lastTrans = NULL;
static void *
getTable (const char *tableList)
{
/*Keep track of which tables have already been compiled */
  int tableListLen;
  ChainEntry *currentEntry = NULL;
  ChainEntry *lastEntry = NULL;
  void *newTable;
  if (tableList == NULL || *tableList == 0)
    return NULL;
  errorCount = fileCount = 0;
  tableListLen = strlen (tableList);
  /*See if this is the last table used. */
  if (lastTrans != NULL)
    if (tableListLen == lastTrans->tableListLength && (memcmp
						       (&lastTrans->
							tableList
							[0],
							tableList,
							tableListLen)) == 0)
      return (table = lastTrans->table);
/*See if Table has already been compiled*/
  currentEntry = tableChain;
  while (currentEntry != NULL)
    {
      if (tableListLen == currentEntry->tableListLength && (memcmp
							    (&currentEntry->
							     tableList
							     [0],
							     tableList,
							     tableListLen))
	  == 0)
	{
	  lastTrans = currentEntry;
	  return (table = currentEntry->table);
	}
      lastEntry = currentEntry;
      currentEntry = currentEntry->next;
    }
  if ((newTable = compileTranslationTable (tableList)))
    {
      /*Add a new entry to the table chain. */
      int entrySize = sizeof (ChainEntry) + tableListLen;
      ChainEntry *newEntry = malloc (entrySize);
      if (!newEntry)
	outOfMemory ();
      if (tableChain == NULL)
	tableChain = newEntry;
      else
	lastEntry->next = newEntry;
      newEntry->next = NULL;
      newEntry->table = newTable;
      newEntry->tableListLength = tableListLen;
      memcpy (&newEntry->tableList[0], tableList, tableListLen);
      lastTrans = newEntry;
      return newEntry->table;
    }
  return NULL;
}

char *
getLastTableList ()
{
  if (lastTrans == NULL)
    return NULL;
  strncpy (scratchBuf, lastTrans->tableList, lastTrans->tableListLength);
  scratchBuf[lastTrans->tableListLength] = 0;
  return scratchBuf;
}

void *EXPORT_CALL
lou_getTable (const char *tableList)
{
  void *table = NULL;
  if (tableList == NULL || tableList[0] == 0)
    return NULL;
  errorCount = fileCount = 0;
  table = getTable (tableList);
  if (!table)
    logMessage (LOG_ERROR, "%s could not be found", tableList);
  return table;
}

static unsigned char *destSpacing = NULL;
static int sizeDestSpacing = 0;
static unsigned short *typebuf = NULL;
static int sizeTypebuf = 0;
static widechar *passbuf1 = NULL;
static int sizePassbuf1 = 0;
static widechar *passbuf2 = NULL;
static int sizePassbuf2 = 0;
static int *srcMapping = NULL;
static int *prevSrcMapping = NULL;
static int sizeSrcMapping = 0;
static int sizePrevSrcMapping = 0;
void *
liblouis_allocMem (AllocBuf buffer, int srcmax, int destmax)
{
  if (srcmax < 1024)
    srcmax = 1024;
  if (destmax < 1024)
    destmax = 1024;
  switch (buffer)
    {
    case alloc_typebuf:
      if (destmax > sizeTypebuf)
	{
	  if (typebuf != NULL)
	    free (typebuf);
	  typebuf = malloc ((destmax + 4) * sizeof (unsigned short));
	  if (!typebuf)
	    outOfMemory ();
	  sizeTypebuf = destmax;
	}
      return typebuf;
    case alloc_destSpacing:
      if (destmax > sizeDestSpacing)
	{
	  if (destSpacing != NULL)
	    free (destSpacing);
	  destSpacing = malloc (destmax + 4);
	  if (!destSpacing)
	    outOfMemory ();
	  sizeDestSpacing = destmax;
	}
      return destSpacing;
    case alloc_passbuf1:
      if (destmax > sizePassbuf1)
	{
	  if (passbuf1 != NULL)
	    free (passbuf1);
	  passbuf1 = malloc ((destmax + 4) * CHARSIZE);
	  if (!passbuf1)
	    outOfMemory ();
	  sizePassbuf1 = destmax;
	}
      return passbuf1;
    case alloc_passbuf2:
      if (destmax > sizePassbuf2)
	{
	  if (passbuf2 != NULL)
	    free (passbuf2);
	  passbuf2 = malloc ((destmax + 4) * CHARSIZE);
	  if (!passbuf2)
	    outOfMemory ();
	  sizePassbuf2 = destmax;
	}
      return passbuf2;
    case alloc_srcMapping:
      {
	int mapSize;
	if (srcmax >= destmax)
	  mapSize = srcmax;
	else
	  mapSize = destmax;
	if (mapSize > sizeSrcMapping)
	  {
	    if (srcMapping != NULL)
	      free (srcMapping);
	    srcMapping = malloc ((mapSize + 4) * sizeof (int));
	    if (!srcMapping)
	      outOfMemory ();
	    sizeSrcMapping = mapSize;
	  }
      }
      return srcMapping;
    case alloc_prevSrcMapping:
      {
	int mapSize;
	if (srcmax >= destmax)
	  mapSize = srcmax;
	else
	  mapSize = destmax;
	if (mapSize > sizePrevSrcMapping)
	  {
	    if (prevSrcMapping != NULL)
	      free (prevSrcMapping);
	    prevSrcMapping = malloc ((mapSize + 4) * sizeof (int));
	    if (!prevSrcMapping)
	      outOfMemory ();
	    sizePrevSrcMapping = mapSize;
	  }
      }
      return prevSrcMapping;
    default:
      return NULL;
    }
}

void EXPORT_CALL
lou_free ()
{
  ChainEntry *currentEntry;
  ChainEntry *previousEntry;
  closeLogFile();
  if (tableChain != NULL)
    {
      currentEntry = tableChain;
      while (currentEntry)
	{
	  free (currentEntry->table);
	  previousEntry = currentEntry;
	  currentEntry = currentEntry->next;
	  free (previousEntry);
	}
      tableChain = NULL;
      lastTrans = NULL;
    }
  if (typebuf != NULL)
    free (typebuf);
  typebuf = NULL;
  sizeTypebuf = 0;
  if (destSpacing != NULL)
    free (destSpacing);
  destSpacing = NULL;
  sizeDestSpacing = 0;
  if (passbuf1 != NULL)
    free (passbuf1);
  passbuf1 = NULL;
  sizePassbuf1 = 0;
  if (passbuf2 != NULL)
    free (passbuf2);
  passbuf2 = NULL;
  sizePassbuf2 = 0;
  if (srcMapping != NULL)
    free (srcMapping);
  srcMapping = NULL;
  sizeSrcMapping = 0;
  if (prevSrcMapping != NULL)
    free (prevSrcMapping);
  prevSrcMapping = NULL;
  sizePrevSrcMapping = 0;
  opcodeLengths[0] = 0;
}

char *EXPORT_CALL
lou_version ()
{
  static char *version = PACKAGE_VERSION;
  return version;
}

int EXPORT_CALL
lou_charSize ()
{
  return CHARSIZE;
}

int EXPORT_CALL
lou_compileString (const char *tableList, const char *inString)
{
  if (!lou_getTable (tableList))
    return 0;
  return compileString (inString);
}

/**
 * This procedure provides a target for cals that serve as breakpoints 
 * for gdb.
 */
/*
char *EXPORT_CALL
lou_getTablePaths ()
{
  static char paths[MAXSTRING];
  char *pathList;
  strcpy (paths, tablePath);
  strcat (paths, ",");
  pathList = getenv ("LOUIS_TABLEPATH");
  if (pathList)
    {
      strcat (paths, pathList);
      strcat (paths, ",");
    }
  pathList = getcwd (scratchBuf, MAXSTRING);
  if (pathList)
    {
      strcat (paths, pathList);
      strcat (paths, ",");
    }
  pathList = lou_getDataPath ();
  if (pathList)
    {
      strcat (paths, pathList);
      strcat (paths, ",");
    }
#ifdef _WIN32
  strcpy (paths, lou_getProgramPath ());
  strcat (paths, "\\share\\liblouss\\tables\\");
#else
  strcpy (paths, TABLESDIR);
#endif
  return paths;
}
*/

void
debugHook ()
{
  char *hook = "debug hook";
  printf ("%s\n", hook);
}
