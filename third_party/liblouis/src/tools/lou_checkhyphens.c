/* liblouis Braille Translation and Back-Translation Library

   Based on the Linux screenreader BRLTTY, copyright (C) 1999-2006 by
   The BRLTTY Team

   Copyright (C) 2004, 2005, 2006, 2009 
   ViewPlus Technologies, Inc. www.viewplus.com and
   JJB Software, Inc. www.jjb-software.com

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Maintained by John J. Boyer john.boyer@jjb-software.com
   */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "louis.h"
#include <getopt.h>
#include "progname.h"
#include "version-etc.h"

static const struct option longopts[] =
{
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'v' },
  { NULL, 0, NULL, 0 }
};

const char version_etc_copyright[] =
  "Copyright %s %d ViewPlus Technologies, Inc. and JJB Software, Inc.";

#define AUTHORS "John J. Boyer"

static void
print_help (void)
{
  printf ("\
Usage: %s [OPTIONS]\n", program_name);
  
  fputs ("\
Check the accuracy of hyphenation in Braille translation for both\n\
translated and untranslated words.\n\n", stdout);

  fputs ("\
  -h, --help          display this help and exit\n\
  -v, --version       display version information and exit\n", stdout);

  printf ("\n");
  printf ("Report bugs to %s.\n", PACKAGE_BUGREPORT);

#ifdef PACKAGE_PACKAGER_BUG_REPORTS
  printf ("Report %s bugs to: %s\n", PACKAGE_PACKAGER, PACKAGE_PACKAGER_BUG_REPORTS);
#endif
#ifdef PACKAGE_URL
  printf ("%s home page: <%s>\n", PACKAGE_NAME, PACKAGE_URL);
#endif
}

#define BUFSIZE 256

static char inputBuffer[BUFSIZE];
static const TranslationTableHeader *validTable = NULL;
static unsigned int mode;
static char table[BUFSIZE];

static int
getInput (void)
{
  int inputLength;
  inputBuffer[0] = 0;
  fgets (inputBuffer, sizeof (inputBuffer), stdin);
  inputLength = strlen (inputBuffer) - 1;
  if (inputLength < 0)		/*EOF on script */
    exit (0);
  inputBuffer[inputLength] = 0;
  return inputLength;
}

static void
paramLetters (void)
{
  printf ("Press one of the letters in parentheses, then enter.\n");
  printf ("(t)able, tr(a)nslated, (u)ntranslated, (r)un, (h)elp, (q)uit\n");
}

static int
getCommands (void)
{
  paramLetters ();
  do
    {
      printf ("Command: ");
      getInput ();
      switch (inputBuffer[0])
	{
	case 0:
	  break;
	case 't':
	  do
	    {
	      printf ("Enter the name of a table or a list: ");
	      getInput ();
	      strcpy (table, inputBuffer);
	      validTable = lou_getTable (table);
	      if (validTable != NULL && validTable->hyphenStatesArray == 0)
		{
		  printf ("No hyphenation table.\n");
		  validTable = NULL;
		}
	    }
	  while (validTable == NULL);
	  break;
	case 'a':
	  mode = 1;
	  break;
	case 'u':
	  mode = 0;
	  break;
	case 'r':
	  if (validTable == NULL)
	    {
	      printf ("You must enter a valid table name or list.\n");
	      inputBuffer[0] = 0;
	    }
	  break;
	case 'h':
	  printf ("Commands: action\n");
	  printf ("(t)able: Enter a table name or list\n");
	  printf ("(r)un: run the hyphenation test loop\n");
	  printf ("tr(a)nslated: translated input\n");
	  printf ("(u)ntranslated: untranslated input\n");
	  printf ("(h)elp: print this page\n");
	  printf ("(q)uit: leave the program\n");
	  printf ("\n");
	  paramLetters ();
	  break;
	case 'q':
	  exit (0);
	default:
	  printf ("Bad choice.\n");
	  break;
	}
    }
  while (inputBuffer[0] != 'r');
  return 1;
}

int
main (int argc, char **argv)
{
  widechar inbuf[BUFSIZE];
  char hyphens[BUFSIZE];
  int inlen;
  int k;
  int optc;

  set_program_name (argv[0]);

  while ((optc = getopt_long (argc, argv, "hv", longopts, NULL)) != -1)
    switch (optc)
      {
      /* --help and --version exit immediately, per GNU coding standards.  */
      case 'v':
        version_etc (stdout, program_name, PACKAGE_NAME, VERSION, AUTHORS, (char *) NULL);
        exit (EXIT_SUCCESS);
        break;
      case 'h':
        print_help ();
        exit (EXIT_SUCCESS);
        break;
      default:
	fprintf (stderr, "Try `%s --help' for more information.\n",
		 program_name);
	exit (EXIT_FAILURE);
        break;
      }

  if (optind < argc)
    {
      /* Print error message and exit.  */
      fprintf (stderr, "%s: extra operand: %s\n",
	       program_name, argv[optind]);
      fprintf (stderr, "Try `%s --help' for more information.\n",
               program_name);
      exit (EXIT_FAILURE);
    }

  validTable = NULL;
  mode = 0;

  while (1)
    {
      getCommands ();
      printf ("Type something, press enter, and view the results.\n");
      printf ("A blank line returns to command entry.\n");
      while (1)
	{
	  inlen = getInput ();
	  if (inlen == 0)
	    break;
	  for (k = 0; k < inlen; k++)
	    inbuf[k] = inputBuffer[k];
	  if (!lou_hyphenate (table, inbuf, inlen, hyphens, mode))
	    {
	      printf ("Hyphenation error\n");
	      continue;
	    }
	  printf ("Hyphenation mask: %s\n", hyphens);
	  printf ("Hyphenated word: ");
	  for (k = 0; k < inlen; k++)
	    {
	      if (hyphens[k] == '1')
		printf ("-");
	      printf ("%c", inbuf[k]);
	    }
	  printf ("\n");
	}
    }
  lou_free ();
  return 0;
}
