/* liblouis Braille Translation and Back-Translation Library

   Based on the Linux screenreader BRLTTY, copyright (C) 1999-2006 by
   The BRLTTY Team

   Copyright (C) 2004, 2005, 2006, 2009 ViewPlus Technologies, Inc.
   www.viewplus.com and JJB Software, Inc. www.jjb-software.com

   liblouis is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   liblouis is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this program. If not, see
   <http://www.gnu.org/licenses/>.

   Maintained by John J. Boyer john.boyer@abilitiessoft.com
   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "louis.h"

static int
ignoreCaseComp (const char *str1, const char *str2, int length)
{
/* Replaces strncasecmp, which some compilers don't support */
  int k;
  for (k = 0; k < length; k++)
    if ((str1[k] | 32) != (str2[k] | 32))
      break;
  if (k != length)
    return 1;
  return 0;
}

static int
findAction (const char **actions, const char *action)
{
  int actionLength = strlen (action);
  int k;
  for (k = 0; actions[k]; k += 2)
    if (actionLength == strlen (actions[k])
	&& ignoreCaseComp (actions[k], action, actionLength) == 0)
      break;
  if (actions[k] == NULL)
    return -1;
  return atoi (actions[k + 1]);
}

static char *
findColon (char *transSpec)
{
  int k;
  for (k = 0; transSpec[k]; k++)
    if (transSpec[k] == ':')
      {
	transSpec[k] = 0;
	return &transSpec[k + 1];
      }
  return NULL;
}

static const char *translators[] = {
  "korean",
  "1",
  "japanese",
  "2",
  "german",
  "3",
  NULL
};

int
other_translate (const char *trantab, const widechar
		 * inbuf,
		 int *inlen, widechar * outbuf, int *outlen,
		 formtype *typeform, char *spacing, int *outputPos, int
		 *inputPos, int *cursorPos, int mode)
{
  char transSpec[MAXSTRING];
  int action;
  strcpy (transSpec, trantab);
  findColon (transSpec);
  action = findAction (translators, transSpec);
  switch (action)
    {
    case -1:
      logMessage (LOG_ERROR, "There is no translator called '%s'", transSpec);
      return 0;
    case 1:
      return 1;
    case 2:
      return 1;
    case 3:
      return 1;
    default:
      return 0;
    }
  return 0;
}

int
other_backTranslate (const char *trantab, const widechar
		     * inbuf,
		     int *inlen, widechar * outbuf, int *outlen,
		     formtype *typeform, char *spacing, int *outputPos, 
		     int
		     *inputPos, int *cursorPos, int mode)
{
  char transSpec[MAXSTRING];
  int action;
  strcpy (transSpec, trantab);
  findColon (transSpec);
  action = findAction (translators, transSpec);
  switch (action)
    {
    case -1:
      logMessage (LOG_ERROR, "There is no translator called '%s'", transSpec);
      return 0;
    case 1:
      return 1;
    case 2:
      return 1;
    case 3:
      return 1;
    default:
      return 0;
    }
  return 0;
}

int
other_charToDots (const char *trantab, const widechar
		  * inbuf, widechar * outbuf, int length, int mode)
{
  return 0;
}

int
other_dotsToChar (const char *trantab, widechar * inbuf,
		  widechar * outbuf, int length, int mode)
{
  return 0;
}
