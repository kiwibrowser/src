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
#include <getopt.h>
#include "liblouis.h"
#include "louis.h"
#include "progname.h"
#include "version-etc.h"

#define BUFSIZE MAXSTRING - 4

static int forward_flag = 0;
static int backward_flag = 0;

static const struct option longopts[] =
{
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'v' },
  { "forward", no_argument, NULL, 'f' },
  { "backward", no_argument, NULL, 'b' },
  { NULL, 0, NULL, 0 }
};

const char version_etc_copyright[] =
  "Copyright %s %d ViewPlus Technologies, Inc. and JJB Software, Inc.";

#define AUTHORS "John J. Boyer"

static void 
translate_input (int forward_translation, char *table_name)
{
  char charbuf[BUFSIZE];
  char *outputbuf;
  widechar inbuf[BUFSIZE];
  widechar transbuf[BUFSIZE];
  int inlen;
  int translen;
  int k;
  int ch = 0;
  int result;
  while (1)
    {
      translen = BUFSIZE;
      k = 0;
      while ((ch = getchar ()) != '\n' && ch != EOF && k < BUFSIZE)
	charbuf[k++] = ch;
      if (ch == EOF && k == 0)
	break;
      charbuf[k] = 0;
      inlen = extParseChars (charbuf, inbuf);
      if (forward_translation) 
	  result = lou_translateString (table_name, inbuf, &inlen,
				      transbuf, &translen, NULL, NULL, 0);
      else 
	result = lou_backTranslateString (table_name, inbuf, &inlen,
					  transbuf, &translen, NULL, NULL, 0);
      if (!result)
	break;
      outputbuf = showString (transbuf, translen);
      k = strlen (outputbuf) - 1;
      outputbuf[k] = 0;
      printf ("%s\n", &outputbuf[1]);
    }
  lou_free ();
}

static void
print_help (void)
{
  printf ("\
Usage: %s [OPTIONS] TABLE[,TABLE,...]\n", program_name);
  
  fputs ("\
Translate whatever is on standard input and print it on standard\n\
output. It is intended for large-scale testing of the accuracy of\n\
Braille translation and back-translation.\n\n", stdout);

  fputs ("\
  -h, --help          display this help and exit\n\
  -v, --version       display version information and exit\n\
  -f, --forward       forward translation using the given table\n\
  -b, --backward      backward translation using the given table\n\
                      If neither -f nor -b are specified forward translation\n\
                      is assumed\n", stdout);
  printf ("\n");
  printf ("Report bugs to %s.\n", PACKAGE_BUGREPORT);

#ifdef PACKAGE_PACKAGER_BUG_REPORTS
  printf ("Report %s bugs to: %s\n", PACKAGE_PACKAGER, PACKAGE_PACKAGER_BUG_REPORTS);
#endif
#ifdef PACKAGE_URL
  printf ("%s home page: <%s>\n", PACKAGE_NAME, PACKAGE_URL);
#endif
}

int
main (int argc, char **argv)
{
  int optc;
  
  set_program_name (argv[0]);

  while ((optc = getopt_long (argc, argv, "hvfb", longopts, NULL)) != -1)
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
      case 'f':
	forward_flag = 1;
        break;
      case 'b':
	backward_flag = 1;
        break;
      default:
	fprintf (stderr, "Try `%s --help' for more information.\n",
		 program_name);
	exit (EXIT_FAILURE);
        break;
      }

  if (forward_flag && backward_flag)
    {
      fprintf (stderr, "%s: specify either -f or -b but not both\n", 
	       program_name);
      fprintf (stderr, "Try `%s --help' for more information.\n",
               program_name);
      exit (EXIT_FAILURE);
    }

  if (optind != argc - 1)
    {
      /* Print error message and exit.  */
      if (optind < argc - 1)
	fprintf (stderr, "%s: extra operand: %s\n",
		 program_name, argv[optind + 1]);
      else
	fprintf (stderr, "%s: no table specified\n", 
		 program_name);
      fprintf (stderr, "Try `%s --help' for more information.\n",
               program_name);
      exit (EXIT_FAILURE);
    }

  /* assume forward translation by default */
  translate_input (!backward_flag, argv[optind]);
  exit (EXIT_SUCCESS);
}
