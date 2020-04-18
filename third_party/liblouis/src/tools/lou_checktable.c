/* liblouis Braille Translation and Back-Translation Library

   Based on BRLTTY, copyright (C) 1999-2006 by
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
#include "louis.h"
#include <getopt.h>
#include "progname.h"
#include "version-etc.h"

static const struct option longopts[] =
{
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'v' },
  { "quiet", no_argument, NULL, 'q' },
  { NULL, 0, NULL, 0 }
};

const char version_etc_copyright[] =
  "Copyright %s %d ViewPlus Technologies, Inc. and JJB Software, Inc.";

#define AUTHORS "John J. Boyer"

static int quiet_flag = 0;

static void
print_help (void)
{
  printf ("\
Usage: %s [OPTIONS] TABLE[,TABLE,...]\n", program_name);
  
  fputs ("\
Test a Braille translation table. If the table contains errors,\n\
appropriate messages are displayed. If there are no errors the\n\
message \"no errors found.\" is shown unless you specify the --quiet\n\
option.\n", stdout);

  fputs ("\
  -h, --help          display this help and exit\n\
  -v, --version       display version information and exit\n\
  -q, --quiet         do not write to standard error if there are no errors.\n", stdout);

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
  const TranslationTableHeader *table;
  int optc;

  set_program_name (argv[0]);

  while ((optc = getopt_long (argc, argv, "hvq", longopts, NULL)) != -1)
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
      case 'q':
	quiet_flag = 1;
        break;
      default:
	fprintf (stderr, "Try `%s --help' for more information.\n",
		 program_name);
	exit (EXIT_FAILURE);
        break;
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

  if (!(table = lou_getTable (argv[optind])))
    {
      lou_free ();
      exit (EXIT_FAILURE);
    }
  if (quiet_flag == 0)
    fprintf (stderr, "No errors found.\n");
  lou_free ();
  exit (EXIT_SUCCESS);
}

