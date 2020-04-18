/* liblouis Braille Translation and Back-Translation Library

   Copyright (C) 2012 Swiss Library for the Blind, Visually Impaired and Print Disabled

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

   */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "louis.h"
#include <getopt.h>
#include "progname.h"
#include "version-etc.h"

static const struct option longopts[] = {
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {NULL, 0, NULL, 0}
};

const char version_etc_copyright[] =
  "Copyright %s %d Swiss Library for the Blind, Visually Impaired and Print Disabled.";

#define AUTHORS "Bert Frees"

static void
print_help(void) {
  printf("\
Usage: %s [OPTIONS] TABLE[,TABLE,...]\n", program_name);
  fputs("\
Examine and debug Braille translation tables. This program allows you\n\
to inspect liblouis translation tables by printing out the list of\n\
applied translation rules for a given input.\n\n", stdout);
  fputs("\
  -h, --help          display this help and exit\n\
  -v, --version       display version information and exit\n\n", stdout);
  printf("Report bugs to %s.\n", PACKAGE_BUGREPORT);
#ifdef PACKAGE_PACKAGER_BUG_REPORTS
  printf("Report %s bugs to: %s\n", PACKAGE_PACKAGER,
	 PACKAGE_PACKAGER_BUG_REPORTS);
#endif
#ifdef PACKAGE_URL
  printf("%s home page: <%s>\n", PACKAGE_NAME, PACKAGE_URL);
#endif
}

#define BUFSIZE 512
#define RULESSIZE 512

static char *
get_input(void) {
  static char input_buffer[BUFSIZE];
  size_t input_length;
  input_buffer[0] = 0;
  fgets(input_buffer, sizeof(input_buffer), stdin);
  input_length = strlen(input_buffer) - 1;
  if (input_length < 0)
    exit(0);
  input_buffer[input_length] = 0;
  return input_buffer;
}

static int
get_wide_input(widechar * buffer) {
  return extParseChars(get_input(), buffer);
}

static char *
print_chars(const widechar * buffer, int length) {
  static char chars[BUFSIZE];
  strcpy(chars, &showString(buffer, length)[1]);
  chars[strlen(chars) - 1] = 0;
  return chars;
}

static char *
print_dots(const widechar * buffer, int length) {
  static char dots[BUFSIZE];
  strcpy(dots, showDots(buffer, length));
  return dots;
}

static char *
print_number(widechar c) {
  static char number[BUFSIZE];
  sprintf(number, "%d", c);
  return number;
}

static char *
print_attributes(unsigned int a) {
  static char attr[BUFSIZE];
  strcpy(attr, showAttributes(a));
  return attr;
}

static void
append_char(char *destination, int *length, char source) {
  destination[(*length)++] = source;
}

static void
append_string(char *destination, int *length, char *source) {
  strcpy(&destination[(*length)], source);
  (*length) += strlen(source);
}

static char *
print_script(const widechar * buffer, int length) {
  static char script[BUFSIZE];
  int i = 0;
  int j = 0;
  while (i < length) {
    switch (buffer[i]) {
    case pass_first:
    case pass_last:
    case pass_not:
    case pass_startReplace:
    case pass_endReplace:
    case pass_search:
    case pass_copy:
    case pass_omit:
      append_char(script, &j, buffer[i++]);
      break;
    case pass_lookback:
      append_char(script, &j, buffer[i++]);
      if (buffer[i] > 1)
	append_string(script, &j, print_number(buffer[i++]));
      break;
    case pass_string:
      append_char(script, &j, buffer[i]);
      append_string(script, &j, print_chars(&buffer[i + 2], buffer[i + 1]));
      append_char(script, &j, buffer[i]);
      i += (2 + buffer[i + 1]);
      break;
    case pass_dots:
      append_char(script, &j, buffer[i++]);
      append_string(script, &j, print_dots(&buffer[i + 1], buffer[i]));
      i += (1 + buffer[i]);
      break;
    case pass_eq:
    case pass_lt:
    case pass_gt:
    case pass_lteq:
    case pass_gteq:
      append_char(script, &j, '#');
      append_string(script, &j, print_number(buffer[i + 1]));
      append_char(script, &j, buffer[i]);
      append_string(script, &j, print_number(buffer[i + 2]));
      i += 3;
      break;
    case pass_hyphen:
    case pass_plus:
      append_char(script, &j, '#');
      append_string(script, &j, print_number(buffer[i + 1]));
      append_char(script, &j, buffer[i]);
      i += 2;
      break;
    case pass_attributes:
      append_char(script, &j, buffer[i]);
      append_string(script, &j,
		    print_attributes(buffer[i + 1] << 16 | buffer[i + 2]));
      i += 3;
      if (buffer[i] == 1 && buffer[i + 1] == 1) {
      } else if (buffer[i] == 1 && buffer[i + 1] == 0xffff)
	append_char(script, &j, pass_until);
      else if (buffer[i] == buffer[i + 1])
	append_string(script, &j, print_number(buffer[i]));
      else {
	append_string(script, &j, print_number(buffer[i]));
	append_char(script, &j, '-');
	append_string(script, &j, print_number(buffer[i + 1]));
      }
      i += 2;
      break;
    case pass_endTest:
      append_char(script, &j, '\t');
      i++;
      break;
    case pass_swap:
    case pass_groupstart:
    case pass_groupend:
    case pass_groupreplace:
      /* TBD */
    default:
      i++;
      break;
    }
  }
  script[j] = 0;
  return script;
}

static void
print_rule(const TranslationTableRule * rule) {
  const char *opcode = findOpcodeName(rule->opcode);
  char *chars;
  char *dots;
  char *script;
  switch (rule->opcode) {
  case CTO_Context:
  case CTO_Correct:
  case CTO_SwapCd:
  case CTO_SwapDd:
  case CTO_Pass2:
  case CTO_Pass3:
  case CTO_Pass4:
    script = print_script(&rule->charsdots[rule->charslen], rule->dotslen);
    printf("%s\t%s\n", opcode, script);
    break;
  default:
    chars = print_chars(rule->charsdots, rule->charslen);
    dots = print_dots(&rule->charsdots[rule->charslen], rule->dotslen);
    printf("%s\t%s\t%s\n", opcode, chars, dots);
    break;
  }
}

static void
main_loop(char *table) {
  widechar inbuf[BUFSIZE];
  widechar outbuf[BUFSIZE];
  int inlen;
  int outlen;
  const TranslationTableRule **rules =
    malloc(512 * sizeof(TranslationTableRule));
  int ruleslen;
  int i;
  while (1) {
    inlen = get_wide_input(inbuf);
    outlen = BUFSIZE;
    ruleslen = RULESSIZE;
    if (!trace_translate(table, inbuf, &inlen, outbuf, &outlen,
			 NULL, NULL, NULL, NULL, NULL, rules, &ruleslen, 0))
      break;
    printf("%s\n", print_chars(outbuf, outlen));
    for (i = 0; i < ruleslen; i++) {
      printf("%d.\t", i + 1);
      print_rule(rules[i]);
    }
  }
}

int
main(int argc, char **argv) {
  int optc;
  char *table;
  set_program_name(argv[0]);
  while ((optc = getopt_long(argc, argv, "hv", longopts, NULL)) != -1) {
    switch (optc) {
    case 'v':
      version_etc(stdout, program_name, PACKAGE_NAME, VERSION, AUTHORS,
		  (char *) NULL);
      exit(EXIT_SUCCESS);
      break;
    case 'h':
      print_help();
      exit(EXIT_SUCCESS);
      break;
    default:
      fprintf(stderr, "Try `%s --help' for more information.\n",
	      program_name);
      exit(EXIT_FAILURE);
      break;
    }
  }
  if (optind != argc - 1) {
    if (optind < argc - 1)
      fprintf(stderr, "%s: extra operand: %s\n", program_name,
	      argv[optind + 1]);
    else
      fprintf(stderr, "%s: no table specified\n", program_name);
    fprintf(stderr, "Try `%s --help' for more information.\n", program_name);
    exit(EXIT_FAILURE);
  }
  table = argv[optind];
  if (!lou_getTable(table)) {
    lou_free();
    exit(EXIT_FAILURE);
  }
  main_loop(table);
  lou_free();
  exit(EXIT_SUCCESS);
}
