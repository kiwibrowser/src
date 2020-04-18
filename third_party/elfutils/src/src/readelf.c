/* Print information from ELF file in human-readable form.
   Copyright (C) 1999-2013 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 1999.

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

#include <argp.h>
#include <assert.h>
#include <ctype.h>
#include <dwarf.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <gelf.h>
#include <inttypes.h>
#include <langinfo.h>
#include <libdw.h>
#include <libdwfl.h>
#include <libintl.h>
#include <locale.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <signal.h>

#include <system.h>
#include "../libelf/libelfP.h"
#include "../libelf/common.h"
#include "../libebl/libeblP.h"
#include "../libdw/libdwP.h"
#include "../libdwfl/libdwflP.h"
#include "../libdw/memory-access.h"

#include "../libdw/known-dwarf.h"


/* Name and version of program.  */
static void print_version (FILE *stream, struct argp_state *state);
ARGP_PROGRAM_VERSION_HOOK_DEF = print_version;

/* Bug report address.  */
ARGP_PROGRAM_BUG_ADDRESS_DEF = PACKAGE_BUGREPORT;

/* argp key value for --elf-section, non-ascii.  */
#define ELF_INPUT_SECTION 256

/* Definitions of arguments for argp functions.  */
static const struct argp_option options[] =
{
  { NULL, 0, NULL, 0, N_("ELF input selection:"), 0 },
  { "elf-section", ELF_INPUT_SECTION, "SECTION", OPTION_ARG_OPTIONAL,
    N_("Use the named SECTION (default .gnu_debugdata) as (compressed) ELF "
       "input data"), 0 },
  { NULL, 0, NULL, 0, N_("ELF output selection:"), 0 },
  { "all", 'a', NULL, 0,
    N_("All these plus -p .strtab -p .dynstr -p .comment"), 0 },
  { "dynamic", 'd', NULL, 0, N_("Display the dynamic segment"), 0 },
  { "file-header", 'h', NULL, 0, N_("Display the ELF file header"), 0 },
  { "histogram", 'I', NULL, 0,
    N_("Display histogram of bucket list lengths"), 0 },
  { "program-headers", 'l', NULL, 0, N_("Display the program headers"), 0 },
  { "segments", 'l', NULL, OPTION_ALIAS | OPTION_HIDDEN, NULL, 0 },
  { "relocs", 'r', NULL, 0, N_("Display relocations"), 0 },
  { "section-headers", 'S', NULL, 0, N_("Display the sections' headers"), 0 },
  { "sections", 'S', NULL, OPTION_ALIAS | OPTION_HIDDEN, NULL, 0 },
  { "symbols", 's', NULL, 0, N_("Display the symbol table"), 0 },
  { "version-info", 'V', NULL, 0, N_("Display versioning information"), 0 },
  { "notes", 'n', NULL, 0, N_("Display the ELF notes"), 0 },
  { "arch-specific", 'A', NULL, 0,
    N_("Display architecture specific information, if any"), 0 },
  { "exception", 'e', NULL, 0,
    N_("Display sections for exception handling"), 0 },

  { NULL, 0, NULL, 0, N_("Additional output selection:"), 0 },
  { "debug-dump", 'w', "SECTION", OPTION_ARG_OPTIONAL,
    N_("Display DWARF section content.  SECTION can be one of abbrev, "
       "aranges, decodedaranges, frame, gdb_index, info, loc, line, "
       "decodedline, ranges, pubnames, str, macinfo, macro or exception"), 0 },
  { "hex-dump", 'x', "SECTION", 0,
    N_("Dump the uninterpreted contents of SECTION, by number or name"), 0 },
  { "strings", 'p', "SECTION", OPTION_ARG_OPTIONAL,
    N_("Print string contents of sections"), 0 },
  { "string-dump", 'p', NULL, OPTION_ALIAS | OPTION_HIDDEN, NULL, 0 },
  { "archive-index", 'c', NULL, 0,
    N_("Display the symbol index of an archive"), 0 },

  { NULL, 0, NULL, 0, N_("Output control:"), 0 },
  { "numeric-addresses", 'N', NULL, 0,
    N_("Do not find symbol names for addresses in DWARF data"), 0 },
  { "unresolved-address-offsets", 'U', NULL, 0,
    N_("Display just offsets instead of resolving values to addresses in DWARF data"), 0 },
  { "wide", 'W', NULL, 0,
    N_("Ignored for compatibility (lines always wide)"), 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Short description of program.  */
static const char doc[] = N_("\
Print information from ELF file in human-readable form.");

/* Strings for arguments in help texts.  */
static const char args_doc[] = N_("FILE...");

/* Prototype for option handler.  */
static error_t parse_opt (int key, char *arg, struct argp_state *state);

/* Data structure to communicate with argp functions.  */
static struct argp argp =
{
  options, parse_opt, args_doc, doc, NULL, NULL, NULL
};

/* If non-null, the section from which we should read to (compressed) ELF.  */
static const char *elf_input_section = NULL;

/* Flags set by the option controlling the output.  */

/* True if dynamic segment should be printed.  */
static bool print_dynamic_table;

/* True if the file header should be printed.  */
static bool print_file_header;

/* True if the program headers should be printed.  */
static bool print_program_header;

/* True if relocations should be printed.  */
static bool print_relocations;

/* True if the section headers should be printed.  */
static bool print_section_header;

/* True if the symbol table should be printed.  */
static bool print_symbol_table;

/* True if the version information should be printed.  */
static bool print_version_info;

/* True if section groups should be printed.  */
static bool print_section_groups;

/* True if bucket list length histogram should be printed.  */
static bool print_histogram;

/* True if the architecture specific data should be printed.  */
static bool print_arch;

/* True if note section content should be printed.  */
static bool print_notes;

/* True if SHF_STRINGS section content should be printed.  */
static bool print_string_sections;

/* True if archive index should be printed.  */
static bool print_archive_index;

/* True if any of the control options except print_archive_index is set.  */
static bool any_control_option;

/* True if we should print addresses from DWARF in symbolic form.  */
static bool print_address_names = true;

/* True if we should print raw values instead of relativized addresses.  */
static bool print_unresolved_addresses = false;

/* True if we should print the .debug_aranges section using libdw.  */
static bool decodedaranges = false;

/* True if we should print the .debug_aranges section using libdw.  */
static bool decodedline = false;

/* Select printing of debugging sections.  */
static enum section_e
{
  section_abbrev = 1,		/* .debug_abbrev  */
  section_aranges = 2,		/* .debug_aranges  */
  section_frame = 4,		/* .debug_frame or .eh_frame & al.  */
  section_info = 8,		/* .debug_info, .debug_types  */
  section_types = section_info,
  section_line = 16,		/* .debug_line  */
  section_loc = 32,		/* .debug_loc  */
  section_pubnames = 64,	/* .debug_pubnames  */
  section_str = 128,		/* .debug_str  */
  section_macinfo = 256,	/* .debug_macinfo  */
  section_ranges = 512, 	/* .debug_ranges  */
  section_exception = 1024,	/* .eh_frame & al.  */
  section_gdb_index = 2048,	/* .gdb_index  */
  section_macro = 4096,		/* .debug_macro  */
  section_all = (section_abbrev | section_aranges | section_frame
		 | section_info | section_line | section_loc
		 | section_pubnames | section_str | section_macinfo
		 | section_ranges | section_exception | section_gdb_index
		 | section_macro)
} print_debug_sections, implicit_debug_sections;

/* Select hex dumping of sections.  */
static struct section_argument *dump_data_sections;
static struct section_argument **dump_data_sections_tail = &dump_data_sections;

/* Select string dumping of sections.  */
static struct section_argument *string_sections;
static struct section_argument **string_sections_tail = &string_sections;

struct section_argument
{
  struct section_argument *next;
  const char *arg;
  bool implicit;
};

/* Numbers of sections and program headers in the file.  */
static size_t shnum;
static size_t phnum;


/* Declarations of local functions.  */
static void process_file (int fd, const char *fname, bool only_one);
static void process_elf_file (Dwfl_Module *dwflmod, int fd);
static void print_ehdr (Ebl *ebl, GElf_Ehdr *ehdr);
static void print_shdr (Ebl *ebl, GElf_Ehdr *ehdr);
static void print_phdr (Ebl *ebl, GElf_Ehdr *ehdr);
static void print_scngrp (Ebl *ebl);
static void print_dynamic (Ebl *ebl);
static void print_relocs (Ebl *ebl, GElf_Ehdr *ehdr);
static void handle_relocs_rel (Ebl *ebl, GElf_Ehdr *ehdr, Elf_Scn *scn,
			       GElf_Shdr *shdr);
static void handle_relocs_rela (Ebl *ebl, GElf_Ehdr *ehdr, Elf_Scn *scn,
				GElf_Shdr *shdr);
static void print_symtab (Ebl *ebl, int type);
static void handle_symtab (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr);
static void print_verinfo (Ebl *ebl);
static void handle_verneed (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr);
static void handle_verdef (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr);
static void handle_versym (Ebl *ebl, Elf_Scn *scn,
			   GElf_Shdr *shdr);
static void print_debug (Dwfl_Module *dwflmod, Ebl *ebl, GElf_Ehdr *ehdr);
static void handle_hash (Ebl *ebl);
static void handle_notes (Ebl *ebl, GElf_Ehdr *ehdr);
static void print_liblist (Ebl *ebl);
static void print_attributes (Ebl *ebl, const GElf_Ehdr *ehdr);
static void dump_data (Ebl *ebl);
static void dump_strings (Ebl *ebl);
static void print_strings (Ebl *ebl);
static void dump_archive_index (Elf *, const char *);


int
main (int argc, char *argv[])
{
  /* Set locale.  */
  setlocale (LC_ALL, "");

  /* Initialize the message catalog.  */
  textdomain (PACKAGE_TARNAME);

  /* Parse and process arguments.  */
  int remaining;
  argp_parse (&argp, argc, argv, 0, &remaining, NULL);

  /* Before we start tell the ELF library which version we are using.  */
  elf_version (EV_CURRENT);

  /* Now process all the files given at the command line.  */
  bool only_one = remaining + 1 == argc;
  do
    {
      /* Open the file.  */
      int fd = open (argv[remaining], O_RDONLY);
      if (fd == -1)
	{
	  error (0, errno, gettext ("cannot open input file"));
	  continue;
	}

      process_file (fd, argv[remaining], only_one);

      close (fd);
    }
  while (++remaining < argc);

  return error_message_count != 0;
}


/* Handle program arguments.  */
static error_t
parse_opt (int key, char *arg,
	   struct argp_state *state __attribute__ ((unused)))
{
  void add_dump_section (const char *name, bool implicit)
  {
    struct section_argument *a = xmalloc (sizeof *a);
    a->arg = name;
    a->next = NULL;
    a->implicit = implicit;
    struct section_argument ***tailp
      = key == 'x' ? &dump_data_sections_tail : &string_sections_tail;
    **tailp = a;
    *tailp = &a->next;
  }

  switch (key)
    {
    case 'a':
      print_file_header = true;
      print_program_header = true;
      print_relocations = true;
      print_section_header = true;
      print_symbol_table = true;
      print_version_info = true;
      print_dynamic_table = true;
      print_section_groups = true;
      print_histogram = true;
      print_arch = true;
      print_notes = true;
      implicit_debug_sections |= section_exception;
      add_dump_section (".strtab", true);
      add_dump_section (".dynstr", true);
      add_dump_section (".comment", true);
      any_control_option = true;
      break;
    case 'A':
      print_arch = true;
      any_control_option = true;
      break;
    case 'd':
      print_dynamic_table = true;
      any_control_option = true;
      break;
    case 'e':
      print_debug_sections |= section_exception;
      any_control_option = true;
      break;
    case 'g':
      print_section_groups = true;
      any_control_option = true;
      break;
    case 'h':
      print_file_header = true;
      any_control_option = true;
      break;
    case 'I':
      print_histogram = true;
      any_control_option = true;
      break;
    case 'l':
      print_program_header = true;
      any_control_option = true;
      break;
    case 'n':
      print_notes = true;
      any_control_option = true;
      break;
    case 'r':
      print_relocations = true;
      any_control_option = true;
     break;
    case 'S':
      print_section_header = true;
      any_control_option = true;
      break;
    case 's':
      print_symbol_table = true;
      any_control_option = true;
      break;
    case 'V':
      print_version_info = true;
      any_control_option = true;
      break;
    case 'c':
      print_archive_index = true;
      break;
    case 'w':
      if (arg == NULL)
	print_debug_sections = section_all;
      else if (strcmp (arg, "abbrev") == 0)
	print_debug_sections |= section_abbrev;
      else if (strcmp (arg, "aranges") == 0)
	print_debug_sections |= section_aranges;
      else if (strcmp (arg, "decodedaranges") == 0)
	{
	  print_debug_sections |= section_aranges;
	  decodedaranges = true;
	}
      else if (strcmp (arg, "ranges") == 0)
	{
	  print_debug_sections |= section_ranges;
	  implicit_debug_sections |= section_info;
	}
      else if (strcmp (arg, "frame") == 0 || strcmp (arg, "frames") == 0)
	print_debug_sections |= section_frame;
      else if (strcmp (arg, "info") == 0)
	print_debug_sections |= section_info;
      else if (strcmp (arg, "loc") == 0)
	{
	  print_debug_sections |= section_loc;
	  implicit_debug_sections |= section_info;
	}
      else if (strcmp (arg, "line") == 0)
	print_debug_sections |= section_line;
      else if (strcmp (arg, "decodedline") == 0)
	{
	  print_debug_sections |= section_line;
	  decodedline = true;
	}
      else if (strcmp (arg, "pubnames") == 0)
	print_debug_sections |= section_pubnames;
      else if (strcmp (arg, "str") == 0)
	print_debug_sections |= section_str;
      else if (strcmp (arg, "macinfo") == 0)
	print_debug_sections |= section_macinfo;
      else if (strcmp (arg, "macro") == 0)
	print_debug_sections |= section_macro;
      else if (strcmp (arg, "exception") == 0)
	print_debug_sections |= section_exception;
      else if (strcmp (arg, "gdb_index") == 0)
	print_debug_sections |= section_gdb_index;
      else
	{
	  fprintf (stderr, gettext ("Unknown DWARF debug section `%s'.\n"),
		   arg);
	  argp_help (&argp, stderr, ARGP_HELP_SEE,
		     program_invocation_short_name);
	  exit (1);
	}
      any_control_option = true;
      break;
    case 'p':
      any_control_option = true;
      if (arg == NULL)
	{
	  print_string_sections = true;
	  break;
	}
      /* Fall through.  */
    case 'x':
      add_dump_section (arg, false);
      any_control_option = true;
      break;
    case 'N':
      print_address_names = false;
      break;
    case 'U':
      print_unresolved_addresses = true;
      break;
    case ARGP_KEY_NO_ARGS:
      fputs (gettext ("Missing file name.\n"), stderr);
      goto do_argp_help;
    case ARGP_KEY_FINI:
      if (! any_control_option && ! print_archive_index)
	{
	  fputs (gettext ("No operation specified.\n"), stderr);
	do_argp_help:
	  argp_help (&argp, stderr, ARGP_HELP_SEE,
		     program_invocation_short_name);
	  exit (EXIT_FAILURE);
	}
      break;
    case 'W':			/* Ignored.  */
      break;
    case ELF_INPUT_SECTION:
      if (arg == NULL)
	elf_input_section = ".gnu_debugdata";
      else
	elf_input_section = arg;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


/* Print the version information.  */
static void
print_version (FILE *stream, struct argp_state *state __attribute__ ((unused)))
{
  fprintf (stream, "readelf (%s) %s\n", PACKAGE_NAME, PACKAGE_VERSION);
  fprintf (stream, gettext ("\
Copyright (C) %s Red Hat, Inc.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "2012");
  fprintf (stream, gettext ("Written by %s.\n"), "Ulrich Drepper");
}


/* Create a file descriptor to read the data from the
   elf_input_section given a file descriptor to an ELF file.  */
static int
open_input_section (int fd)
{
  size_t shnums;
  size_t cnt;
  size_t shstrndx;
  Elf *elf = elf_begin (fd, ELF_C_READ_MMAP, NULL);
  if (elf == NULL)
    {
      error (0, 0, gettext ("cannot generate Elf descriptor: %s"),
	     elf_errmsg (-1));
      return -1;
    }

  if (elf_getshdrnum (elf, &shnums) < 0)
    {
      error (0, 0, gettext ("cannot determine number of sections: %s"),
	     elf_errmsg (-1));
    open_error:
      elf_end (elf);
      return -1;
    }

  if (elf_getshdrstrndx (elf, &shstrndx) < 0)
    {
      error (0, 0, gettext ("cannot get section header string table index"));
      goto open_error;
    }

  for (cnt = 0; cnt < shnums; ++cnt)
    {
      Elf_Scn *scn = elf_getscn (elf, cnt);
      if (scn == NULL)
	{
	  error (0, 0, gettext ("cannot get section: %s"),
		 elf_errmsg (-1));
	  goto open_error;
	}

      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
      if (unlikely (shdr == NULL))
	{
	  error (0, 0, gettext ("cannot get section header: %s"),
		 elf_errmsg (-1));
	  goto open_error;
	}

      const char *sname = elf_strptr (elf, shstrndx, shdr->sh_name);
      if (sname == NULL)
	{
	  error (0, 0, gettext ("cannot get section name"));
	  goto open_error;
	}

      if (strcmp (sname, elf_input_section) == 0)
	{
	  Elf_Data *data = elf_rawdata (scn, NULL);
	  if (data == NULL)
	    {
	      error (0, 0, gettext ("cannot get %s content: %s"),
		     sname, elf_errmsg (-1));
	      goto open_error;
	    }

	  /* Create (and immediately unlink) a temporary file to store
	     section data in to create a file descriptor for it.  */
	  const char *tmpdir = getenv ("TMPDIR") ?: P_tmpdir;
	  static const char suffix[] = "/readelfXXXXXX";
	  int tmplen = strlen (tmpdir) + sizeof (suffix);
	  char *tempname = alloca (tmplen);
	  sprintf (tempname, "%s%s", tmpdir, suffix);

	  int sfd = mkstemp (tempname);
	  if (sfd == -1)
	    {
	      error (0, 0, gettext ("cannot create temp file '%s'"),
		     tempname);
	      goto open_error;
	    }
	  unlink (tempname);

	  ssize_t size = data->d_size;
	  if (write_retry (sfd, data->d_buf, size) != size)
	    {
	      error (0, 0, gettext ("cannot write section data"));
	      goto open_error;
	    }

	  if (elf_end (elf) != 0)
	    {
	      error (0, 0, gettext ("error while closing Elf descriptor: %s"),
		     elf_errmsg (-1));
	      return -1;
	    }

	  if (lseek (sfd, 0, SEEK_SET) == -1)
	    {
	      error (0, 0, gettext ("error while rewinding file descriptor"));
	      return -1;
	    }

	  return sfd;
	}
    }

  /* Named section not found.  */
  if (elf_end (elf) != 0)
    error (0, 0, gettext ("error while closing Elf descriptor: %s"),
	   elf_errmsg (-1));
  return -1;
}

/* Check if the file is an archive, and if so dump its index.  */
static void
check_archive_index (int fd, const char *fname, bool only_one)
{
  /* Create an `Elf' descriptor.  */
  Elf *elf = elf_begin (fd, ELF_C_READ_MMAP, NULL);
  if (elf == NULL)
    error (0, 0, gettext ("cannot generate Elf descriptor: %s"),
	   elf_errmsg (-1));
  else
    {
      if (elf_kind (elf) == ELF_K_AR)
	{
	  if (!only_one)
	    printf ("\n%s:\n\n", fname);
	  dump_archive_index (elf, fname);
	}
      else
	error (0, 0,
	       gettext ("'%s' is not an archive, cannot print archive index"),
	       fname);

      /* Now we can close the descriptor.  */
      if (elf_end (elf) != 0)
	error (0, 0, gettext ("error while closing Elf descriptor: %s"),
	       elf_errmsg (-1));
    }
}

/* Trivial callback used for checking if we opened an archive.  */
static int
count_dwflmod (Dwfl_Module *dwflmod __attribute__ ((unused)),
	       void **userdata __attribute__ ((unused)),
	       const char *name __attribute__ ((unused)),
	       Dwarf_Addr base __attribute__ ((unused)),
	       void *arg)
{
  if (*(bool *) arg)
    return DWARF_CB_ABORT;
  *(bool *) arg = true;
  return DWARF_CB_OK;
}

struct process_dwflmod_args
{
  int fd;
  bool only_one;
};

static int
process_dwflmod (Dwfl_Module *dwflmod,
		 void **userdata __attribute__ ((unused)),
		 const char *name __attribute__ ((unused)),
		 Dwarf_Addr base __attribute__ ((unused)),
		 void *arg)
{
  const struct process_dwflmod_args *a = arg;

  /* Print the file name.  */
  if (!a->only_one)
    {
      const char *fname;
      dwfl_module_info (dwflmod, NULL, NULL, NULL, NULL, NULL, &fname, NULL);

      printf ("\n%s:\n\n", fname);
    }

  process_elf_file (dwflmod, a->fd);

  return DWARF_CB_OK;
}

/* Stub libdwfl callback, only the ELF handle already open is ever used.  */
static int
find_no_debuginfo (Dwfl_Module *mod __attribute__ ((unused)),
		   void **userdata __attribute__ ((unused)),
		   const char *modname __attribute__ ((unused)),
		   Dwarf_Addr base __attribute__ ((unused)),
		   const char *file_name __attribute__ ((unused)),
		   const char *debuglink_file __attribute__ ((unused)),
		   GElf_Word debuglink_crc __attribute__ ((unused)),
		   char **debuginfo_file_name __attribute__ ((unused)))
{
  return -1;
}

/* Process one input file.  */
static void
process_file (int fd, const char *fname, bool only_one)
{
  if (print_archive_index)
    check_archive_index (fd, fname, only_one);

  if (!any_control_option)
    return;

  if (elf_input_section != NULL)
    {
      /* Replace fname and fd with section content. */
      char *fnname = alloca (strlen (fname) + strlen (elf_input_section) + 2);
      sprintf (fnname, "%s:%s", fname, elf_input_section);
      fd = open_input_section (fd);
      if (fd == -1)
        {
          error (0, 0, gettext ("No such section '%s' in '%s'"),
		 elf_input_section, fname);
          return;
        }
      fname = fnname;
    }

  /* Duplicate an fd for dwfl_report_offline to swallow.  */
  int dwfl_fd = dup (fd);
  if (unlikely (dwfl_fd < 0))
    error (EXIT_FAILURE, errno, "dup");

  /* Use libdwfl in a trivial way to open the libdw handle for us.
     This takes care of applying relocations to DWARF data in ET_REL files.  */
  static const Dwfl_Callbacks callbacks =
    {
      .section_address = dwfl_offline_section_address,
      .find_debuginfo = find_no_debuginfo
    };
  Dwfl *dwfl = dwfl_begin (&callbacks);
  if (likely (dwfl != NULL))
    /* Let 0 be the logical address of the file (or first in archive).  */
    dwfl->offline_next_address = 0;
  if (dwfl_report_offline (dwfl, fname, fname, dwfl_fd) == NULL)
    {
      struct stat64 st;
      if (fstat64 (dwfl_fd, &st) != 0)
	error (0, errno, gettext ("cannot stat input file"));
      else if (unlikely (st.st_size == 0))
	error (0, 0, gettext ("input file is empty"));
      else
	error (0, 0, gettext ("failed reading '%s': %s"),
	       fname, dwfl_errmsg (-1));
      close (dwfl_fd);		/* Consumed on success, not on failure.  */
    }
  else
    {
      dwfl_report_end (dwfl, NULL, NULL);

      if (only_one)
	{
	  /* Clear ONLY_ONE if we have multiple modules, from an archive.  */
	  bool seen = false;
	  only_one = dwfl_getmodules (dwfl, &count_dwflmod, &seen, 0) == 0;
	}

      /* Process the one or more modules gleaned from this file.  */
      struct process_dwflmod_args a = { .fd = fd, .only_one = only_one };
      dwfl_getmodules (dwfl, &process_dwflmod, &a, 0);
    }
  dwfl_end (dwfl);

  /* Need to close the replaced fd if we created it.  Caller takes
     care of original.  */
  if (elf_input_section != NULL)
    close (fd);
}


/* Process one ELF file.  */
static void
process_elf_file (Dwfl_Module *dwflmod, int fd)
{
  GElf_Addr dwflbias;
  Elf *elf = dwfl_module_getelf (dwflmod, &dwflbias);

  GElf_Ehdr ehdr_mem;
  GElf_Ehdr *ehdr = gelf_getehdr (elf, &ehdr_mem);

  if (ehdr == NULL)
    {
    elf_error:
      error (0, 0, gettext ("cannot read ELF header: %s"), elf_errmsg (-1));
      return;
    }

  Ebl *ebl = ebl_openbackend (elf);
  if (unlikely (ebl == NULL))
    {
    ebl_error:
      error (0, errno, gettext ("cannot create EBL handle"));
      return;
    }

  /* Determine the number of sections.  */
  if (unlikely (elf_getshdrnum (ebl->elf, &shnum) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot determine number of sections: %s"),
	   elf_errmsg (-1));

  /* Determine the number of phdrs.  */
  if (unlikely (elf_getphdrnum (ebl->elf, &phnum) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot determine number of program headers: %s"),
	   elf_errmsg (-1));

  /* For an ET_REL file, libdwfl has adjusted the in-core shdrs
     and may have applied relocation to some sections.
     So we need to get a fresh Elf handle on the file to display those.  */
  bool print_unrelocated = (print_section_header
			    || print_relocations
			    || dump_data_sections != NULL
			    || print_notes);

  Elf *pure_elf = NULL;
  Ebl *pure_ebl = ebl;
  if (ehdr->e_type == ET_REL && print_unrelocated)
    {
      /* Read the file afresh.  */
      off64_t aroff = elf_getaroff (elf);
      pure_elf = elf_begin (fd, ELF_C_READ_MMAP, NULL);
      if (aroff > 0)
	{
	  /* Archive member.  */
	  (void) elf_rand (pure_elf, aroff);
	  Elf *armem = elf_begin (-1, ELF_C_READ_MMAP, pure_elf);
	  elf_end (pure_elf);
	  pure_elf = armem;
	}
      if (pure_elf == NULL)
	goto elf_error;
      pure_ebl = ebl_openbackend (pure_elf);
      if (pure_ebl == NULL)
	goto ebl_error;
    }

  if (print_file_header)
    print_ehdr (ebl, ehdr);
  if (print_section_header)
    print_shdr (pure_ebl, ehdr);
  if (print_program_header)
    print_phdr (ebl, ehdr);
  if (print_section_groups)
    print_scngrp (ebl);
  if (print_dynamic_table)
    print_dynamic (ebl);
  if (print_relocations)
    print_relocs (pure_ebl, ehdr);
  if (print_histogram)
    handle_hash (ebl);
  if (print_symbol_table)
    print_symtab (ebl, SHT_DYNSYM);
  if (print_version_info)
    print_verinfo (ebl);
  if (print_symbol_table)
    print_symtab (ebl, SHT_SYMTAB);
  if (print_arch)
    print_liblist (ebl);
  if (print_arch)
    print_attributes (ebl, ehdr);
  if (dump_data_sections != NULL)
    dump_data (pure_ebl);
  if (string_sections != NULL)
    dump_strings (ebl);
  if ((print_debug_sections | implicit_debug_sections) != 0)
    print_debug (dwflmod, ebl, ehdr);
  if (print_notes)
    handle_notes (pure_ebl, ehdr);
  if (print_string_sections)
    print_strings (ebl);

  ebl_closebackend (ebl);

  if (pure_ebl != ebl)
    {
      ebl_closebackend (pure_ebl);
      elf_end (pure_elf);
    }
}


/* Print file type.  */
static void
print_file_type (unsigned short int e_type)
{
  if (likely (e_type <= ET_CORE))
    {
      static const char *const knowntypes[] =
      {
	N_("NONE (None)"),
	N_("REL (Relocatable file)"),
	N_("EXEC (Executable file)"),
	N_("DYN (Shared object file)"),
	N_("CORE (Core file)")
      };
      puts (gettext (knowntypes[e_type]));
    }
  else if (e_type >= ET_LOOS && e_type <= ET_HIOS)
    printf (gettext ("OS Specific: (%x)\n"),  e_type);
  else if (e_type >= ET_LOPROC /* && e_type <= ET_HIPROC always true */)
    printf (gettext ("Processor Specific: (%x)\n"),  e_type);
  else
    puts ("???");
}


/* Print ELF header.  */
static void
print_ehdr (Ebl *ebl, GElf_Ehdr *ehdr)
{
  fputs_unlocked (gettext ("ELF Header:\n  Magic:  "), stdout);
  for (size_t cnt = 0; cnt < EI_NIDENT; ++cnt)
    printf (" %02hhx", ehdr->e_ident[cnt]);

  printf (gettext ("\n  Class:                             %s\n"),
	  ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? "ELF32"
	  : ehdr->e_ident[EI_CLASS] == ELFCLASS64 ? "ELF64"
	  : "\?\?\?");

  printf (gettext ("  Data:                              %s\n"),
	  ehdr->e_ident[EI_DATA] == ELFDATA2LSB
	  ? "2's complement, little endian"
	  : ehdr->e_ident[EI_DATA] == ELFDATA2MSB
	  ? "2's complement, big endian" : "\?\?\?");

  printf (gettext ("  Ident Version:                     %hhd %s\n"),
	  ehdr->e_ident[EI_VERSION],
	  ehdr->e_ident[EI_VERSION] == EV_CURRENT ? gettext ("(current)")
	  : "(\?\?\?)");

  char buf[512];
  printf (gettext ("  OS/ABI:                            %s\n"),
	  ebl_osabi_name (ebl, ehdr->e_ident[EI_OSABI], buf, sizeof (buf)));

  printf (gettext ("  ABI Version:                       %hhd\n"),
	  ehdr->e_ident[EI_ABIVERSION]);

  fputs_unlocked (gettext ("  Type:                              "), stdout);
  print_file_type (ehdr->e_type);

  printf (gettext ("  Machine:                           %s\n"), ebl->name);

  printf (gettext ("  Version:                           %d %s\n"),
	  ehdr->e_version,
	  ehdr->e_version  == EV_CURRENT ? gettext ("(current)") : "(\?\?\?)");

  printf (gettext ("  Entry point address:               %#" PRIx64 "\n"),
	  ehdr->e_entry);

  printf (gettext ("  Start of program headers:          %" PRId64 " %s\n"),
	  ehdr->e_phoff, gettext ("(bytes into file)"));

  printf (gettext ("  Start of section headers:          %" PRId64 " %s\n"),
	  ehdr->e_shoff, gettext ("(bytes into file)"));

  printf (gettext ("  Flags:                             %s\n"),
	  ebl_machine_flag_name (ebl, ehdr->e_flags, buf, sizeof (buf)));

  printf (gettext ("  Size of this header:               %" PRId16 " %s\n"),
	  ehdr->e_ehsize, gettext ("(bytes)"));

  printf (gettext ("  Size of program header entries:    %" PRId16 " %s\n"),
	  ehdr->e_phentsize, gettext ("(bytes)"));

  printf (gettext ("  Number of program headers entries: %" PRId16),
	  ehdr->e_phnum);
  if (ehdr->e_phnum == PN_XNUM)
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (elf_getscn (ebl->elf, 0), &shdr_mem);
      if (shdr != NULL)
	printf (gettext (" (%" PRIu32 " in [0].sh_info)"),
		(uint32_t) shdr->sh_info);
      else
	fputs_unlocked (gettext (" ([0] not available)"), stdout);
    }
  fputc_unlocked ('\n', stdout);

  printf (gettext ("  Size of section header entries:    %" PRId16 " %s\n"),
	  ehdr->e_shentsize, gettext ("(bytes)"));

  printf (gettext ("  Number of section headers entries: %" PRId16),
	  ehdr->e_shnum);
  if (ehdr->e_shnum == 0)
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (elf_getscn (ebl->elf, 0), &shdr_mem);
      if (shdr != NULL)
	printf (gettext (" (%" PRIu32 " in [0].sh_size)"),
		(uint32_t) shdr->sh_size);
      else
	fputs_unlocked (gettext (" ([0] not available)"), stdout);
    }
  fputc_unlocked ('\n', stdout);

  if (unlikely (ehdr->e_shstrndx == SHN_XINDEX))
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (elf_getscn (ebl->elf, 0), &shdr_mem);
      if (shdr != NULL)
	/* We managed to get the zeroth section.  */
	snprintf (buf, sizeof (buf), gettext (" (%" PRIu32 " in [0].sh_link)"),
		  (uint32_t) shdr->sh_link);
      else
	{
	  strncpy (buf, gettext (" ([0] not available)"), sizeof (buf));
	  buf[sizeof (buf) - 1] = '\0';
	}

      printf (gettext ("  Section header string table index: XINDEX%s\n\n"),
	      buf);
    }
  else
    printf (gettext ("  Section header string table index: %" PRId16 "\n\n"),
	    ehdr->e_shstrndx);
}


static const char *
get_visibility_type (int value)
{
  switch (value)
    {
    case STV_DEFAULT:
      return "DEFAULT";
    case STV_INTERNAL:
      return "INTERNAL";
    case STV_HIDDEN:
      return "HIDDEN";
    case STV_PROTECTED:
      return "PROTECTED";
    default:
      return "???";
    }
}


/* Print the section headers.  */
static void
print_shdr (Ebl *ebl, GElf_Ehdr *ehdr)
{
  size_t cnt;
  size_t shstrndx;

  if (! print_file_header)
    printf (gettext ("\
There are %d section headers, starting at offset %#" PRIx64 ":\n\
\n"),
	    ehdr->e_shnum, ehdr->e_shoff);

  /* Get the section header string table index.  */
  if (unlikely (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  puts (gettext ("Section Headers:"));

  if (ehdr->e_ident[EI_CLASS] == ELFCLASS32)
    puts (gettext ("[Nr] Name                 Type         Addr     Off    Size   ES Flags Lk Inf Al"));
  else
    puts (gettext ("[Nr] Name                 Type         Addr             Off      Size     ES Flags Lk Inf Al"));

  for (cnt = 0; cnt < shnum; ++cnt)
    {
      Elf_Scn *scn = elf_getscn (ebl->elf, cnt);

      if (unlikely (scn == NULL))
	error (EXIT_FAILURE, 0, gettext ("cannot get section: %s"),
	       elf_errmsg (-1));

      /* Get the section header.  */
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
      if (unlikely (shdr == NULL))
	error (EXIT_FAILURE, 0, gettext ("cannot get section header: %s"),
	       elf_errmsg (-1));

      char flagbuf[20];
      char *cp = flagbuf;
      if (shdr->sh_flags & SHF_WRITE)
	*cp++ = 'W';
      if (shdr->sh_flags & SHF_ALLOC)
	*cp++ = 'A';
      if (shdr->sh_flags & SHF_EXECINSTR)
	*cp++ = 'X';
      if (shdr->sh_flags & SHF_MERGE)
	*cp++ = 'M';
      if (shdr->sh_flags & SHF_STRINGS)
	*cp++ = 'S';
      if (shdr->sh_flags & SHF_INFO_LINK)
	*cp++ = 'I';
      if (shdr->sh_flags & SHF_LINK_ORDER)
	*cp++ = 'L';
      if (shdr->sh_flags & SHF_OS_NONCONFORMING)
	*cp++ = 'N';
      if (shdr->sh_flags & SHF_GROUP)
	*cp++ = 'G';
      if (shdr->sh_flags & SHF_TLS)
	*cp++ = 'T';
      if (shdr->sh_flags & SHF_ORDERED)
	*cp++ = 'O';
      if (shdr->sh_flags & SHF_EXCLUDE)
	*cp++ = 'E';
      *cp = '\0';

      char buf[128];
      printf ("[%2zu] %-20s %-12s %0*" PRIx64 " %0*" PRIx64 " %0*" PRIx64
	      " %2" PRId64 " %-5s %2" PRId32 " %3" PRId32
	      " %2" PRId64 "\n",
	      cnt,
	      elf_strptr (ebl->elf, shstrndx, shdr->sh_name)
	      ?: "<corrupt>",
	      ebl_section_type_name (ebl, shdr->sh_type, buf, sizeof (buf)),
	      ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 8 : 16, shdr->sh_addr,
	      ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 6 : 8, shdr->sh_offset,
	      ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 6 : 8, shdr->sh_size,
	      shdr->sh_entsize, flagbuf, shdr->sh_link, shdr->sh_info,
	      shdr->sh_addralign);
    }

  fputc_unlocked ('\n', stdout);
}


/* Print the program header.  */
static void
print_phdr (Ebl *ebl, GElf_Ehdr *ehdr)
{
  if (ehdr->e_phnum == 0)
    /* No program header, this is OK in relocatable objects.  */
    return;

  puts (gettext ("Program Headers:"));
  if (ehdr->e_ident[EI_CLASS] == ELFCLASS32)
    puts (gettext ("\
  Type           Offset   VirtAddr   PhysAddr   FileSiz  MemSiz   Flg Align"));
  else
    puts (gettext ("\
  Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align"));

  /* Process all program headers.  */
  bool has_relro = false;
  GElf_Addr relro_from = 0;
  GElf_Addr relro_to = 0;
  for (size_t cnt = 0; cnt < phnum; ++cnt)
    {
      char buf[128];
      GElf_Phdr mem;
      GElf_Phdr *phdr = gelf_getphdr (ebl->elf, cnt, &mem);

      /* If for some reason the header cannot be returned show this.  */
      if (unlikely (phdr == NULL))
	{
	  puts ("  ???");
	  continue;
	}

      printf ("  %-14s 0x%06" PRIx64 " 0x%0*" PRIx64 " 0x%0*" PRIx64
	      " 0x%06" PRIx64 " 0x%06" PRIx64 " %c%c%c 0x%" PRIx64 "\n",
	      ebl_segment_type_name (ebl, phdr->p_type, buf, sizeof (buf)),
	      phdr->p_offset,
	      ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 8 : 16, phdr->p_vaddr,
	      ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 8 : 16, phdr->p_paddr,
	      phdr->p_filesz,
	      phdr->p_memsz,
	      phdr->p_flags & PF_R ? 'R' : ' ',
	      phdr->p_flags & PF_W ? 'W' : ' ',
	      phdr->p_flags & PF_X ? 'E' : ' ',
	      phdr->p_align);

      if (phdr->p_type == PT_INTERP)
	{
	  /* We can show the user the name of the interpreter.  */
	  size_t maxsize;
	  char *filedata = elf_rawfile (ebl->elf, &maxsize);

	  if (filedata != NULL && phdr->p_offset < maxsize)
	    printf (gettext ("\t[Requesting program interpreter: %s]\n"),
		    filedata + phdr->p_offset);
	}
      else if (phdr->p_type == PT_GNU_RELRO)
	{
	  has_relro = true;
	  relro_from = phdr->p_vaddr;
	  relro_to = relro_from + phdr->p_memsz;
	}
    }

  if (ehdr->e_shnum == 0)
    /* No sections in the file.  Punt.  */
    return;

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (unlikely (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  puts (gettext ("\n Section to Segment mapping:\n  Segment Sections..."));

  for (size_t cnt = 0; cnt < phnum; ++cnt)
    {
      /* Print the segment number.  */
      printf ("   %2.2zu     ", cnt);

      GElf_Phdr phdr_mem;
      GElf_Phdr *phdr = gelf_getphdr (ebl->elf, cnt, &phdr_mem);
      /* This must not happen.  */
      if (unlikely (phdr == NULL))
	error (EXIT_FAILURE, 0, gettext ("cannot get program header: %s"),
	       elf_errmsg (-1));

      /* Iterate over the sections.  */
      bool in_relro = false;
      bool in_ro = false;
      for (size_t inner = 1; inner < shnum; ++inner)
	{
	  Elf_Scn *scn = elf_getscn (ebl->elf, inner);
	  /* This should not happen.  */
	  if (unlikely (scn == NULL))
	    error (EXIT_FAILURE, 0, gettext ("cannot get section: %s"),
		   elf_errmsg (-1));

	  /* Get the section header.  */
	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	  if (unlikely (shdr == NULL))
	    error (EXIT_FAILURE, 0,
		   gettext ("cannot get section header: %s"),
		   elf_errmsg (-1));

	  if (shdr->sh_size > 0
	      /* Compare allocated sections by VMA, unallocated
		 sections by file offset.  */
	      && (shdr->sh_flags & SHF_ALLOC
		  ? (shdr->sh_addr >= phdr->p_vaddr
		     && (shdr->sh_addr + shdr->sh_size
			 <= phdr->p_vaddr + phdr->p_memsz))
		  : (shdr->sh_offset >= phdr->p_offset
		     && (shdr->sh_offset + shdr->sh_size
			 <= phdr->p_offset + phdr->p_filesz))))
	    {
	      if (has_relro && !in_relro
		  && shdr->sh_addr >= relro_from
		  && shdr->sh_addr + shdr->sh_size <= relro_to)
		{
		  fputs_unlocked (" [RELRO:", stdout);
		  in_relro = true;
		}
	      else if (has_relro && in_relro && shdr->sh_addr >= relro_to)
		{
		  fputs_unlocked ("]", stdout);
		  in_relro =  false;
		}
	      else if (has_relro && in_relro
		       && shdr->sh_addr + shdr->sh_size > relro_to)
		fputs_unlocked ("] <RELRO:", stdout);
	      else if (phdr->p_type == PT_LOAD && (phdr->p_flags & PF_W) == 0)
		{
		  if (!in_ro)
		    {
		      fputs_unlocked (" [RO:", stdout);
		      in_ro = true;
		    }
		}
	      else
		{
		  /* Determine the segment this section is part of.  */
		  size_t cnt2;
		  GElf_Phdr *phdr2 = NULL;
		  for (cnt2 = 0; cnt2 < phnum; ++cnt2)
		    {
		      GElf_Phdr phdr2_mem;
		      phdr2 = gelf_getphdr (ebl->elf, cnt2, &phdr2_mem);

		      if (phdr2 != NULL && phdr2->p_type == PT_LOAD
			  && shdr->sh_addr >= phdr2->p_vaddr
			  && (shdr->sh_addr + shdr->sh_size
			      <= phdr2->p_vaddr + phdr2->p_memsz))
			break;
		    }

		  if (cnt2 < phnum)
		    {
		      if ((phdr2->p_flags & PF_W) == 0 && !in_ro)
			{
			  fputs_unlocked (" [RO:", stdout);
			  in_ro = true;
			}
		      else if ((phdr2->p_flags & PF_W) != 0 && in_ro)
			{
			  fputs_unlocked ("]", stdout);
			  in_ro = false;
			}
		    }
		}

	      printf (" %s",
		      elf_strptr (ebl->elf, shstrndx, shdr->sh_name));

	      /* Signal that this sectin is only partially covered.  */
	      if (has_relro && in_relro
		       && shdr->sh_addr + shdr->sh_size > relro_to)
		{
		  fputs_unlocked (">", stdout);
		  in_relro =  false;
		}
	    }
	}
      if (in_relro || in_ro)
	fputs_unlocked ("]", stdout);

      /* Finish the line.  */
      fputc_unlocked ('\n', stdout);
    }
}


static const char *
section_name (Ebl *ebl, GElf_Ehdr *ehdr, GElf_Shdr *shdr)
{
  return elf_strptr (ebl->elf, ehdr->e_shstrndx, shdr->sh_name) ?: "???";
}


static void
handle_scngrp (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr)
{
  /* Get the data of the section.  */
  Elf_Data *data = elf_getdata (scn, NULL);

  Elf_Scn *symscn = elf_getscn (ebl->elf, shdr->sh_link);
  GElf_Shdr symshdr_mem;
  GElf_Shdr *symshdr = gelf_getshdr (symscn, &symshdr_mem);
  Elf_Data *symdata = elf_getdata (symscn, NULL);

  if (data == NULL || data->d_size < sizeof (Elf32_Word) || symshdr == NULL
      || symdata == NULL)
    return;

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (unlikely (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  Elf32_Word *grpref = (Elf32_Word *) data->d_buf;

  GElf_Sym sym_mem;
  printf ((grpref[0] & GRP_COMDAT)
	  ? ngettext ("\
\nCOMDAT section group [%2zu] '%s' with signature '%s' contains %zu entry:\n",
		      "\
\nCOMDAT section group [%2zu] '%s' with signature '%s' contains %zu entries:\n",
		      data->d_size / sizeof (Elf32_Word) - 1)
	  : ngettext ("\
\nSection group [%2zu] '%s' with signature '%s' contains %zu entry:\n", "\
\nSection group [%2zu] '%s' with signature '%s' contains %zu entries:\n",
		      data->d_size / sizeof (Elf32_Word) - 1),
	  elf_ndxscn (scn),
	  elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
	  elf_strptr (ebl->elf, symshdr->sh_link,
		      gelf_getsym (symdata, shdr->sh_info, &sym_mem)->st_name)
	  ?: gettext ("<INVALID SYMBOL>"),
	  data->d_size / sizeof (Elf32_Word) - 1);

  for (size_t cnt = 1; cnt < data->d_size / sizeof (Elf32_Word); ++cnt)
    {
      GElf_Shdr grpshdr_mem;
      GElf_Shdr *grpshdr = gelf_getshdr (elf_getscn (ebl->elf, grpref[cnt]),
					 &grpshdr_mem);

      const char *str;
      printf ("  [%2u] %s\n",
	      grpref[cnt],
	      grpshdr != NULL
	      && (str = elf_strptr (ebl->elf, shstrndx, grpshdr->sh_name))
	      ? str : gettext ("<INVALID SECTION>"));
    }
}


static void
print_scngrp (Ebl *ebl)
{
  /* Find all relocation sections and handle them.  */
  Elf_Scn *scn = NULL;

  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
       /* Handle the section if it is a symbol table.  */
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (shdr != NULL && shdr->sh_type == SHT_GROUP)
	handle_scngrp (ebl, scn, shdr);
    }
}


static const struct flags
{
  int mask;
  const char *str;
} dt_flags[] =
  {
    { DF_ORIGIN, "ORIGIN" },
    { DF_SYMBOLIC, "SYMBOLIC" },
    { DF_TEXTREL, "TEXTREL" },
    { DF_BIND_NOW, "BIND_NOW" },
    { DF_STATIC_TLS, "STATIC_TLS" }
  };
static const int ndt_flags = sizeof (dt_flags) / sizeof (dt_flags[0]);

static const struct flags dt_flags_1[] =
  {
    { DF_1_NOW, "NOW" },
    { DF_1_GLOBAL, "GLOBAL" },
    { DF_1_GROUP, "GROUP" },
    { DF_1_NODELETE, "NODELETE" },
    { DF_1_LOADFLTR, "LOADFLTR" },
    { DF_1_INITFIRST, "INITFIRST" },
    { DF_1_NOOPEN, "NOOPEN" },
    { DF_1_ORIGIN, "ORIGIN" },
    { DF_1_DIRECT, "DIRECT" },
    { DF_1_TRANS, "TRANS" },
    { DF_1_INTERPOSE, "INTERPOSE" },
    { DF_1_NODEFLIB, "NODEFLIB" },
    { DF_1_NODUMP, "NODUMP" },
    { DF_1_CONFALT, "CONFALT" },
    { DF_1_ENDFILTEE, "ENDFILTEE" },
    { DF_1_DISPRELDNE, "DISPRELDNE" },
    { DF_1_DISPRELPND, "DISPRELPND" },
  };
static const int ndt_flags_1 = sizeof (dt_flags_1) / sizeof (dt_flags_1[0]);

static const struct flags dt_feature_1[] =
  {
    { DTF_1_PARINIT, "PARINIT" },
    { DTF_1_CONFEXP, "CONFEXP" }
  };
static const int ndt_feature_1 = (sizeof (dt_feature_1)
				  / sizeof (dt_feature_1[0]));

static const struct flags dt_posflag_1[] =
  {
    { DF_P1_LAZYLOAD, "LAZYLOAD" },
    { DF_P1_GROUPPERM, "GROUPPERM" }
  };
static const int ndt_posflag_1 = (sizeof (dt_posflag_1)
				  / sizeof (dt_posflag_1[0]));


static void
print_flags (int class, GElf_Xword d_val, const struct flags *flags,
		int nflags)
{
  bool first = true;
  int cnt;

  for (cnt = 0; cnt < nflags; ++cnt)
    if (d_val & flags[cnt].mask)
      {
	if (!first)
	  putchar_unlocked (' ');
	fputs_unlocked (flags[cnt].str, stdout);
	d_val &= ~flags[cnt].mask;
	first = false;
      }

  if (d_val != 0)
    {
      if (!first)
	putchar_unlocked (' ');
      printf ("%#0*" PRIx64, class == ELFCLASS32 ? 10 : 18, d_val);
    }

  putchar_unlocked ('\n');
}


static void
print_dt_flags (int class, GElf_Xword d_val)
{
  print_flags (class, d_val, dt_flags, ndt_flags);
}


static void
print_dt_flags_1 (int class, GElf_Xword d_val)
{
  print_flags (class, d_val, dt_flags_1, ndt_flags_1);
}


static void
print_dt_feature_1 (int class, GElf_Xword d_val)
{
  print_flags (class, d_val, dt_feature_1, ndt_feature_1);
}


static void
print_dt_posflag_1 (int class, GElf_Xword d_val)
{
  print_flags (class, d_val, dt_posflag_1, ndt_posflag_1);
}


static void
handle_dynamic (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr)
{
  int class = gelf_getclass (ebl->elf);
  GElf_Shdr glink;
  Elf_Data *data;
  size_t cnt;
  size_t shstrndx;

  /* Get the data of the section.  */
  data = elf_getdata (scn, NULL);
  if (data == NULL)
    return;

  /* Get the section header string table index.  */
  if (unlikely (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  printf (ngettext ("\
\nDynamic segment contains %lu entry:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    "\
\nDynamic segment contains %lu entries:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    shdr->sh_size / shdr->sh_entsize),
	  (unsigned long int) (shdr->sh_size / shdr->sh_entsize),
	  class == ELFCLASS32 ? 10 : 18, shdr->sh_addr,
	  shdr->sh_offset,
	  (int) shdr->sh_link,
	  elf_strptr (ebl->elf, shstrndx,
		      gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link),
				    &glink)->sh_name));
  fputs_unlocked (gettext ("  Type              Value\n"), stdout);

  for (cnt = 0; cnt < shdr->sh_size / shdr->sh_entsize; ++cnt)
    {
      GElf_Dyn dynmem;
      GElf_Dyn *dyn = gelf_getdyn (data, cnt, &dynmem);
      if (dyn == NULL)
	break;

      char buf[64];
      printf ("  %-17s ",
	      ebl_dynamic_tag_name (ebl, dyn->d_tag, buf, sizeof (buf)));

      switch (dyn->d_tag)
	{
	case DT_NULL:
	case DT_DEBUG:
	case DT_BIND_NOW:
	case DT_TEXTREL:
	  /* No further output.  */
	  fputc_unlocked ('\n', stdout);
	  break;

	case DT_NEEDED:
	  printf (gettext ("Shared library: [%s]\n"),
		  elf_strptr (ebl->elf, shdr->sh_link, dyn->d_un.d_val));
	  break;

	case DT_SONAME:
	  printf (gettext ("Library soname: [%s]\n"),
		  elf_strptr (ebl->elf, shdr->sh_link, dyn->d_un.d_val));
	  break;

	case DT_RPATH:
	  printf (gettext ("Library rpath: [%s]\n"),
		  elf_strptr (ebl->elf, shdr->sh_link, dyn->d_un.d_val));
	  break;

	case DT_RUNPATH:
	  printf (gettext ("Library runpath: [%s]\n"),
		  elf_strptr (ebl->elf, shdr->sh_link, dyn->d_un.d_val));
	  break;

	case DT_PLTRELSZ:
	case DT_RELASZ:
	case DT_STRSZ:
	case DT_RELSZ:
	case DT_RELAENT:
	case DT_SYMENT:
	case DT_RELENT:
	case DT_PLTPADSZ:
	case DT_MOVEENT:
	case DT_MOVESZ:
	case DT_INIT_ARRAYSZ:
	case DT_FINI_ARRAYSZ:
	case DT_SYMINSZ:
	case DT_SYMINENT:
	case DT_GNU_CONFLICTSZ:
	case DT_GNU_LIBLISTSZ:
	  printf (gettext ("%" PRId64 " (bytes)\n"), dyn->d_un.d_val);
	  break;

	case DT_VERDEFNUM:
	case DT_VERNEEDNUM:
	case DT_RELACOUNT:
	case DT_RELCOUNT:
	  printf ("%" PRId64 "\n", dyn->d_un.d_val);
	  break;

	case DT_PLTREL:;
	  const char *tagname = ebl_dynamic_tag_name (ebl, dyn->d_un.d_val,
						      NULL, 0);
	  puts (tagname ?: "???");
	  break;

	case DT_FLAGS:
	  print_dt_flags (class, dyn->d_un.d_val);
	  break;

	case DT_FLAGS_1:
	  print_dt_flags_1 (class, dyn->d_un.d_val);
	  break;

	case DT_FEATURE_1:
	  print_dt_feature_1 (class, dyn->d_un.d_val);
	  break;

	case DT_POSFLAG_1:
	  print_dt_posflag_1 (class, dyn->d_un.d_val);
	  break;

	default:
	  printf ("%#0*" PRIx64 "\n",
		  class == ELFCLASS32 ? 10 : 18, dyn->d_un.d_val);
	  break;
	}
    }
}


/* Print the dynamic segment.  */
static void
print_dynamic (Ebl *ebl)
{
  for (size_t i = 0; i < phnum; ++i)
    {
      GElf_Phdr phdr_mem;
      GElf_Phdr *phdr = gelf_getphdr (ebl->elf, i, &phdr_mem);

      if (phdr != NULL && phdr->p_type == PT_DYNAMIC)
	{
	  Elf_Scn *scn = gelf_offscn (ebl->elf, phdr->p_offset);
	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	  if (shdr != NULL && shdr->sh_type == SHT_DYNAMIC)
	    handle_dynamic (ebl, scn, shdr);
	  break;
	}
    }
}


/* Print relocations.  */
static void
print_relocs (Ebl *ebl, GElf_Ehdr *ehdr)
{
  /* Find all relocation sections and handle them.  */
  Elf_Scn *scn = NULL;

  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
       /* Handle the section if it is a symbol table.  */
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (likely (shdr != NULL))
	{
	  if (shdr->sh_type == SHT_REL)
	    handle_relocs_rel (ebl, ehdr, scn, shdr);
	  else if (shdr->sh_type == SHT_RELA)
	    handle_relocs_rela (ebl, ehdr, scn, shdr);
	}
    }
}


/* Handle a relocation section.  */
static void
handle_relocs_rel (Ebl *ebl, GElf_Ehdr *ehdr, Elf_Scn *scn, GElf_Shdr *shdr)
{
  int class = gelf_getclass (ebl->elf);
  int nentries = shdr->sh_size / shdr->sh_entsize;

  /* Get the data of the section.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    return;

  /* Get the symbol table information.  */
  Elf_Scn *symscn = elf_getscn (ebl->elf, shdr->sh_link);
  GElf_Shdr symshdr_mem;
  GElf_Shdr *symshdr = gelf_getshdr (symscn, &symshdr_mem);
  Elf_Data *symdata = elf_getdata (symscn, NULL);

  /* Get the section header of the section the relocations are for.  */
  GElf_Shdr destshdr_mem;
  GElf_Shdr *destshdr = gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_info),
				      &destshdr_mem);

  if (unlikely (symshdr == NULL || symdata == NULL || destshdr == NULL))
    {
      printf (gettext ("\nInvalid symbol table at offset %#0" PRIx64 "\n"),
	      shdr->sh_offset);
      return;
    }

  /* Search for the optional extended section index table.  */
  Elf_Data *xndxdata = NULL;
  int xndxscnidx = elf_scnshndx (scn);
  if (unlikely (xndxscnidx > 0))
    xndxdata = elf_getdata (elf_getscn (ebl->elf, xndxscnidx), NULL);

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (unlikely (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  if (shdr->sh_info != 0)
    printf (ngettext ("\
\nRelocation section [%2zu] '%s' for section [%2u] '%s' at offset %#0" PRIx64 " contains %d entry:\n",
		    "\
\nRelocation section [%2zu] '%s' for section [%2u] '%s' at offset %#0" PRIx64 " contains %d entries:\n",
		      nentries),
	    elf_ndxscn (scn),
	    elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
	    (unsigned int) shdr->sh_info,
	    elf_strptr (ebl->elf, shstrndx, destshdr->sh_name),
	    shdr->sh_offset,
	    nentries);
  else
    /* The .rel.dyn section does not refer to a specific section but
       instead of section index zero.  Do not try to print a section
       name.  */
    printf (ngettext ("\
\nRelocation section [%2u] '%s' at offset %#0" PRIx64 " contains %d entry:\n",
		    "\
\nRelocation section [%2u] '%s' at offset %#0" PRIx64 " contains %d entries:\n",
		      nentries),
	    (unsigned int) elf_ndxscn (scn),
	    elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
	    shdr->sh_offset,
	    nentries);
  fputs_unlocked (class == ELFCLASS32
		  ? gettext ("\
  Offset      Type                 Value       Name\n")
		  : gettext ("\
  Offset              Type                 Value               Name\n"),
	 stdout);

  int is_statically_linked = 0;
  for (int cnt = 0; cnt < nentries; ++cnt)
    {
      GElf_Rel relmem;
      GElf_Rel *rel = gelf_getrel (data, cnt, &relmem);
      if (likely (rel != NULL))
	{
	  char buf[128];
	  GElf_Sym symmem;
	  Elf32_Word xndx;
	  GElf_Sym *sym = gelf_getsymshndx (symdata, xndxdata,
					    GELF_R_SYM (rel->r_info),
					    &symmem, &xndx);
	  if (unlikely (sym == NULL))
	    {
	      /* As a special case we have to handle relocations in static
		 executables.  This only happens for IRELATIVE relocations
		 (so far).  There is no symbol table.  */
	      if (is_statically_linked == 0)
		{
		  /* Find the program header and look for a PT_INTERP entry. */
		  is_statically_linked = -1;
		  if (ehdr->e_type == ET_EXEC)
		    {
		      is_statically_linked = 1;

		      for (size_t inner = 0; inner < phnum; ++inner)
			{
			  GElf_Phdr phdr_mem;
			  GElf_Phdr *phdr = gelf_getphdr (ebl->elf, inner,
							  &phdr_mem);
			  if (phdr != NULL && phdr->p_type == PT_INTERP)
			    {
			      is_statically_linked = -1;
			      break;
			    }
			}
		    }
		}

	      if (is_statically_linked > 0 && shdr->sh_link == 0)
		printf ("\
  %#0*" PRIx64 "  %-20s %*s  %s\n",
			class == ELFCLASS32 ? 10 : 18, rel->r_offset,
			ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
			/* Avoid the leading R_ which isn't carrying any
			   information.  */
			? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					       buf, sizeof (buf)) + 2
			: gettext ("<INVALID RELOC>"),
			class == ELFCLASS32 ? 10 : 18, "",
			elf_strptr (ebl->elf, shstrndx, destshdr->sh_name));
	      else
		printf ("  %#0*" PRIx64 "  %-20s <%s %ld>\n",
			class == ELFCLASS32 ? 10 : 18, rel->r_offset,
			ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
			/* Avoid the leading R_ which isn't carrying any
			   information.  */
			? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					       buf, sizeof (buf)) + 2
			: gettext ("<INVALID RELOC>"),
			gettext ("INVALID SYMBOL"),
			(long int) GELF_R_SYM (rel->r_info));
	    }
	  else if (GELF_ST_TYPE (sym->st_info) != STT_SECTION)
	    printf ("  %#0*" PRIx64 "  %-20s %#0*" PRIx64 "  %s\n",
		    class == ELFCLASS32 ? 10 : 18, rel->r_offset,
		    likely (ebl_reloc_type_check (ebl,
						  GELF_R_TYPE (rel->r_info)))
		    /* Avoid the leading R_ which isn't carrying any
		       information.  */
		    ? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					   buf, sizeof (buf)) + 2
		    : gettext ("<INVALID RELOC>"),
		    class == ELFCLASS32 ? 10 : 18, sym->st_value,
		    elf_strptr (ebl->elf, symshdr->sh_link, sym->st_name));
	  else
	    {
	      destshdr = gelf_getshdr (elf_getscn (ebl->elf,
						   sym->st_shndx == SHN_XINDEX
						   ? xndx : sym->st_shndx),
				       &destshdr_mem);

	      if (unlikely (destshdr == NULL))
		printf ("  %#0*" PRIx64 "  %-20s <%s %ld>\n",
			class == ELFCLASS32 ? 10 : 18, rel->r_offset,
			ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
			/* Avoid the leading R_ which isn't carrying any
			   information.  */
			? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					       buf, sizeof (buf)) + 2
			: gettext ("<INVALID RELOC>"),
			gettext ("INVALID SECTION"),
			(long int) (sym->st_shndx == SHN_XINDEX
				    ? xndx : sym->st_shndx));
	      else
		printf ("  %#0*" PRIx64 "  %-20s %#0*" PRIx64 "  %s\n",
			class == ELFCLASS32 ? 10 : 18, rel->r_offset,
			ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
			/* Avoid the leading R_ which isn't carrying any
			   information.  */
			? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					       buf, sizeof (buf)) + 2
			: gettext ("<INVALID RELOC>"),
			class == ELFCLASS32 ? 10 : 18, sym->st_value,
			elf_strptr (ebl->elf, shstrndx, destshdr->sh_name));
	    }
	}
    }
}


/* Handle a relocation section.  */
static void
handle_relocs_rela (Ebl *ebl, GElf_Ehdr *ehdr, Elf_Scn *scn, GElf_Shdr *shdr)
{
  int class = gelf_getclass (ebl->elf);
  int nentries = shdr->sh_size / shdr->sh_entsize;

  /* Get the data of the section.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    return;

  /* Get the symbol table information.  */
  Elf_Scn *symscn = elf_getscn (ebl->elf, shdr->sh_link);
  GElf_Shdr symshdr_mem;
  GElf_Shdr *symshdr = gelf_getshdr (symscn, &symshdr_mem);
  Elf_Data *symdata = elf_getdata (symscn, NULL);

  /* Get the section header of the section the relocations are for.  */
  GElf_Shdr destshdr_mem;
  GElf_Shdr *destshdr = gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_info),
				      &destshdr_mem);

  if (unlikely (symshdr == NULL || symdata == NULL || destshdr == NULL))
    {
      printf (gettext ("\nInvalid symbol table at offset %#0" PRIx64 "\n"),
	      shdr->sh_offset);
      return;
    }

  /* Search for the optional extended section index table.  */
  Elf_Data *xndxdata = NULL;
  int xndxscnidx = elf_scnshndx (scn);
  if (unlikely (xndxscnidx > 0))
    xndxdata = elf_getdata (elf_getscn (ebl->elf, xndxscnidx), NULL);

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (unlikely (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  printf (ngettext ("\
\nRelocation section [%2zu] '%s' for section [%2u] '%s' at offset %#0" PRIx64 " contains %d entry:\n",
		    "\
\nRelocation section [%2zu] '%s' for section [%2u] '%s' at offset %#0" PRIx64 " contains %d entries:\n",
		    nentries),
	  elf_ndxscn (scn),
	  elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
	  (unsigned int) shdr->sh_info,
	  elf_strptr (ebl->elf, shstrndx, destshdr->sh_name),
	  shdr->sh_offset,
	  nentries);
  fputs_unlocked (class == ELFCLASS32
		  ? gettext ("\
  Offset      Type            Value       Addend Name\n")
		  : gettext ("\
  Offset              Type            Value               Addend Name\n"),
		  stdout);

  int is_statically_linked = 0;
  for (int cnt = 0; cnt < nentries; ++cnt)
    {
      GElf_Rela relmem;
      GElf_Rela *rel = gelf_getrela (data, cnt, &relmem);
      if (likely (rel != NULL))
	{
	  char buf[64];
	  GElf_Sym symmem;
	  Elf32_Word xndx;
	  GElf_Sym *sym = gelf_getsymshndx (symdata, xndxdata,
					    GELF_R_SYM (rel->r_info),
					    &symmem, &xndx);

	  if (unlikely (sym == NULL))
	    {
	      /* As a special case we have to handle relocations in static
		 executables.  This only happens for IRELATIVE relocations
		 (so far).  There is no symbol table.  */
	      if (is_statically_linked == 0)
		{
		  /* Find the program header and look for a PT_INTERP entry. */
		  is_statically_linked = -1;
		  if (ehdr->e_type == ET_EXEC)
		    {
		      is_statically_linked = 1;

		      for (size_t inner = 0; inner < phnum; ++inner)
			{
			  GElf_Phdr phdr_mem;
			  GElf_Phdr *phdr = gelf_getphdr (ebl->elf, inner,
							  &phdr_mem);
			  if (phdr != NULL && phdr->p_type == PT_INTERP)
			    {
			      is_statically_linked = -1;
			      break;
			    }
			}
		    }
		}

	      if (is_statically_linked > 0 && shdr->sh_link == 0)
		printf ("\
  %#0*" PRIx64 "  %-15s %*s  %#6" PRIx64 " %s\n",
			class == ELFCLASS32 ? 10 : 18, rel->r_offset,
			ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
			/* Avoid the leading R_ which isn't carrying any
			   information.  */
			? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					       buf, sizeof (buf)) + 2
			: gettext ("<INVALID RELOC>"),
			class == ELFCLASS32 ? 10 : 18, "",
			rel->r_addend,
			elf_strptr (ebl->elf, shstrndx, destshdr->sh_name));
	      else
		printf ("  %#0*" PRIx64 "  %-15s <%s %ld>\n",
			class == ELFCLASS32 ? 10 : 18, rel->r_offset,
			ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
			/* Avoid the leading R_ which isn't carrying any
			   information.  */
			? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					       buf, sizeof (buf)) + 2
			: gettext ("<INVALID RELOC>"),
			gettext ("INVALID SYMBOL"),
			(long int) GELF_R_SYM (rel->r_info));
	    }
	  else if (GELF_ST_TYPE (sym->st_info) != STT_SECTION)
	    printf ("\
  %#0*" PRIx64 "  %-15s %#0*" PRIx64 "  %+6" PRId64 " %s\n",
		    class == ELFCLASS32 ? 10 : 18, rel->r_offset,
		    likely (ebl_reloc_type_check (ebl,
						  GELF_R_TYPE (rel->r_info)))
		    /* Avoid the leading R_ which isn't carrying any
		       information.  */
		    ? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					   buf, sizeof (buf)) + 2
		    : gettext ("<INVALID RELOC>"),
		    class == ELFCLASS32 ? 10 : 18, sym->st_value,
		    rel->r_addend,
		    elf_strptr (ebl->elf, symshdr->sh_link, sym->st_name));
	  else
	    {
	      destshdr = gelf_getshdr (elf_getscn (ebl->elf,
						   sym->st_shndx == SHN_XINDEX
						   ? xndx : sym->st_shndx),
				       &destshdr_mem);

	      if (unlikely (shdr == NULL))
		printf ("  %#0*" PRIx64 "  %-15s <%s %ld>\n",
			class == ELFCLASS32 ? 10 : 18, rel->r_offset,
			ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
			/* Avoid the leading R_ which isn't carrying any
			   information.  */
			? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					       buf, sizeof (buf)) + 2
			: gettext ("<INVALID RELOC>"),
			gettext ("INVALID SECTION"),
			(long int) (sym->st_shndx == SHN_XINDEX
				    ? xndx : sym->st_shndx));
	      else
		printf ("\
  %#0*" PRIx64 "  %-15s %#0*" PRIx64 "  %+6" PRId64 " %s\n",
			class == ELFCLASS32 ? 10 : 18, rel->r_offset,
			ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
			/* Avoid the leading R_ which isn't carrying any
			   information.  */
			? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					       buf, sizeof (buf)) + 2
			: gettext ("<INVALID RELOC>"),
			class == ELFCLASS32 ? 10 : 18, sym->st_value,
			rel->r_addend,
			elf_strptr (ebl->elf, shstrndx, destshdr->sh_name));
	    }
	}
    }
}


/* Print the program header.  */
static void
print_symtab (Ebl *ebl, int type)
{
  /* Find the symbol table(s).  For this we have to search through the
     section table.  */
  Elf_Scn *scn = NULL;

  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
      /* Handle the section if it is a symbol table.  */
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (shdr != NULL && shdr->sh_type == (GElf_Word) type)
	handle_symtab (ebl, scn, shdr);
    }
}


static void
handle_symtab (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr)
{
  Elf_Data *versym_data = NULL;
  Elf_Data *verneed_data = NULL;
  Elf_Data *verdef_data = NULL;
  Elf_Data *xndx_data = NULL;
  int class = gelf_getclass (ebl->elf);
  Elf32_Word verneed_stridx = 0;
  Elf32_Word verdef_stridx = 0;

  /* Get the data of the section.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    return;

  /* Find out whether we have other sections we might need.  */
  Elf_Scn *runscn = NULL;
  while ((runscn = elf_nextscn (ebl->elf, runscn)) != NULL)
    {
      GElf_Shdr runshdr_mem;
      GElf_Shdr *runshdr = gelf_getshdr (runscn, &runshdr_mem);

      if (likely (runshdr != NULL))
	{
	  if (runshdr->sh_type == SHT_GNU_versym
	      && runshdr->sh_link == elf_ndxscn (scn))
	    /* Bingo, found the version information.  Now get the data.  */
	    versym_data = elf_getdata (runscn, NULL);
	  else if (runshdr->sh_type == SHT_GNU_verneed)
	    {
	      /* This is the information about the needed versions.  */
	      verneed_data = elf_getdata (runscn, NULL);
	      verneed_stridx = runshdr->sh_link;
	    }
	  else if (runshdr->sh_type == SHT_GNU_verdef)
	    {
	      /* This is the information about the defined versions.  */
	      verdef_data = elf_getdata (runscn, NULL);
	      verdef_stridx = runshdr->sh_link;
	    }
	  else if (runshdr->sh_type == SHT_SYMTAB_SHNDX
	      && runshdr->sh_link == elf_ndxscn (scn))
	    /* Extended section index.  */
	    xndx_data = elf_getdata (runscn, NULL);
	}
    }

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (unlikely (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  /* Now we can compute the number of entries in the section.  */
  unsigned int nsyms = data->d_size / (class == ELFCLASS32
				       ? sizeof (Elf32_Sym)
				       : sizeof (Elf64_Sym));

  printf (ngettext ("\nSymbol table [%2u] '%s' contains %u entry:\n",
		    "\nSymbol table [%2u] '%s' contains %u entries:\n",
		    nsyms),
	  (unsigned int) elf_ndxscn (scn),
	  elf_strptr (ebl->elf, shstrndx, shdr->sh_name), nsyms);
  GElf_Shdr glink;
  printf (ngettext (" %lu local symbol  String table: [%2u] '%s'\n",
		    " %lu local symbols  String table: [%2u] '%s'\n",
		    shdr->sh_info),
	  (unsigned long int) shdr->sh_info,
	  (unsigned int) shdr->sh_link,
	  elf_strptr (ebl->elf, shstrndx,
		      gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link),
				    &glink)->sh_name));

  fputs_unlocked (class == ELFCLASS32
		  ? gettext ("\
  Num:    Value   Size Type    Bind   Vis          Ndx Name\n")
		  : gettext ("\
  Num:            Value   Size Type    Bind   Vis          Ndx Name\n"),
		  stdout);

  for (unsigned int cnt = 0; cnt < nsyms; ++cnt)
    {
      char typebuf[64];
      char bindbuf[64];
      char scnbuf[64];
      Elf32_Word xndx;
      GElf_Sym sym_mem;
      GElf_Sym *sym = gelf_getsymshndx (data, xndx_data, cnt, &sym_mem, &xndx);

      if (unlikely (sym == NULL))
	continue;

      /* Determine the real section index.  */
      if (likely (sym->st_shndx != SHN_XINDEX))
	xndx = sym->st_shndx;

      printf (gettext ("\
%5u: %0*" PRIx64 " %6" PRId64 " %-7s %-6s %-9s %6s %s"),
	      cnt,
	      class == ELFCLASS32 ? 8 : 16,
	      sym->st_value,
	      sym->st_size,
	      ebl_symbol_type_name (ebl, GELF_ST_TYPE (sym->st_info),
				    typebuf, sizeof (typebuf)),
	      ebl_symbol_binding_name (ebl, GELF_ST_BIND (sym->st_info),
				       bindbuf, sizeof (bindbuf)),
	      get_visibility_type (GELF_ST_VISIBILITY (sym->st_other)),
	      ebl_section_name (ebl, sym->st_shndx, xndx, scnbuf,
				sizeof (scnbuf), NULL, shnum),
	      elf_strptr (ebl->elf, shdr->sh_link, sym->st_name));

      if (versym_data != NULL)
	{
	  /* Get the version information.  */
	  GElf_Versym versym_mem;
	  GElf_Versym *versym = gelf_getversym (versym_data, cnt, &versym_mem);

	  if (versym != NULL && ((*versym & 0x8000) != 0 || *versym > 1))
	    {
	      bool is_nobits = false;
	      bool check_def = xndx != SHN_UNDEF;

	      if (xndx < SHN_LORESERVE || sym->st_shndx == SHN_XINDEX)
		{
		  GElf_Shdr symshdr_mem;
		  GElf_Shdr *symshdr =
		    gelf_getshdr (elf_getscn (ebl->elf, xndx), &symshdr_mem);

		  is_nobits = (symshdr != NULL
			       && symshdr->sh_type == SHT_NOBITS);
		}

	      if (is_nobits || ! check_def)
		{
		  /* We must test both.  */
		  GElf_Vernaux vernaux_mem;
		  GElf_Vernaux *vernaux = NULL;
		  size_t vn_offset = 0;

		  GElf_Verneed verneed_mem;
		  GElf_Verneed *verneed = gelf_getverneed (verneed_data, 0,
							   &verneed_mem);
		  while (verneed != NULL)
		    {
		      size_t vna_offset = vn_offset;

		      vernaux = gelf_getvernaux (verneed_data,
						 vna_offset += verneed->vn_aux,
						 &vernaux_mem);
		      while (vernaux != NULL
			     && vernaux->vna_other != *versym
			     && vernaux->vna_next != 0)
			{
			  /* Update the offset.  */
			  vna_offset += vernaux->vna_next;

			  vernaux = (vernaux->vna_next == 0
				     ? NULL
				     : gelf_getvernaux (verneed_data,
							vna_offset,
							&vernaux_mem));
			}

		      /* Check whether we found the version.  */
		      if (vernaux != NULL && vernaux->vna_other == *versym)
			/* Found it.  */
			break;

		      vn_offset += verneed->vn_next;
		      verneed = (verneed->vn_next == 0
				 ? NULL
				 : gelf_getverneed (verneed_data, vn_offset,
						    &verneed_mem));
		    }

		  if (vernaux != NULL && vernaux->vna_other == *versym)
		    {
		      printf ("@%s (%u)",
			      elf_strptr (ebl->elf, verneed_stridx,
					  vernaux->vna_name),
			      (unsigned int) vernaux->vna_other);
		      check_def = 0;
		    }
		  else if (unlikely (! is_nobits))
		    error (0, 0, gettext ("bad dynamic symbol"));
		  else
		    check_def = 1;
		}

	      if (check_def && *versym != 0x8001)
		{
		  /* We must test both.  */
		  size_t vd_offset = 0;

		  GElf_Verdef verdef_mem;
		  GElf_Verdef *verdef = gelf_getverdef (verdef_data, 0,
							&verdef_mem);
		  while (verdef != NULL)
		    {
		      if (verdef->vd_ndx == (*versym & 0x7fff))
			/* Found the definition.  */
			break;

		      vd_offset += verdef->vd_next;
		      verdef = (verdef->vd_next == 0
				? NULL
				: gelf_getverdef (verdef_data, vd_offset,
						  &verdef_mem));
		    }

		  if (verdef != NULL)
		    {
		      GElf_Verdaux verdaux_mem;
		      GElf_Verdaux *verdaux
			= gelf_getverdaux (verdef_data,
					   vd_offset + verdef->vd_aux,
					   &verdaux_mem);

		      if (verdaux != NULL)
			printf ((*versym & 0x8000) ? "@%s" : "@@%s",
				elf_strptr (ebl->elf, verdef_stridx,
					    verdaux->vda_name));
		    }
		}
	    }
	}

      putchar_unlocked ('\n');
    }
}


/* Print version information.  */
static void
print_verinfo (Ebl *ebl)
{
  /* Find the version information sections.  For this we have to
     search through the section table.  */
  Elf_Scn *scn = NULL;

  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
      /* Handle the section if it is part of the versioning handling.  */
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (likely (shdr != NULL))
	{
	  if (shdr->sh_type == SHT_GNU_verneed)
	    handle_verneed (ebl, scn, shdr);
	  else if (shdr->sh_type == SHT_GNU_verdef)
	    handle_verdef (ebl, scn, shdr);
	  else if (shdr->sh_type == SHT_GNU_versym)
	    handle_versym (ebl, scn, shdr);
	}
    }
}


static const char *
get_ver_flags (unsigned int flags)
{
  static char buf[32];
  char *endp;

  if (flags == 0)
    return gettext ("none");

  if (flags & VER_FLG_BASE)
    endp = stpcpy (buf, "BASE ");
  else
    endp = buf;

  if (flags & VER_FLG_WEAK)
    {
      if (endp != buf)
	endp = stpcpy (endp, "| ");

      endp = stpcpy (endp, "WEAK ");
    }

  if (unlikely (flags & ~(VER_FLG_BASE | VER_FLG_WEAK)))
    {
      strncpy (endp, gettext ("| <unknown>"), buf + sizeof (buf) - endp);
      buf[sizeof (buf) - 1] = '\0';
    }

  return buf;
}


static void
handle_verneed (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr)
{
  int class = gelf_getclass (ebl->elf);

  /* Get the data of the section.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    return;

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (unlikely (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  GElf_Shdr glink;
  printf (ngettext ("\
\nVersion needs section [%2u] '%s' contains %d entry:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    "\
\nVersion needs section [%2u] '%s' contains %d entries:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    shdr->sh_info),
	  (unsigned int) elf_ndxscn (scn),
	  elf_strptr (ebl->elf, shstrndx, shdr->sh_name), shdr->sh_info,
	  class == ELFCLASS32 ? 10 : 18, shdr->sh_addr,
	  shdr->sh_offset,
	  (unsigned int) shdr->sh_link,
	  elf_strptr (ebl->elf, shstrndx,
		      gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link),
				    &glink)->sh_name));

  unsigned int offset = 0;
  for (int cnt = shdr->sh_info; --cnt >= 0; )
    {
      /* Get the data at the next offset.  */
      GElf_Verneed needmem;
      GElf_Verneed *need = gelf_getverneed (data, offset, &needmem);
      if (unlikely (need == NULL))
	break;

      printf (gettext ("  %#06x: Version: %hu  File: %s  Cnt: %hu\n"),
	      offset, (unsigned short int) need->vn_version,
	      elf_strptr (ebl->elf, shdr->sh_link, need->vn_file),
	      (unsigned short int) need->vn_cnt);

      unsigned int auxoffset = offset + need->vn_aux;
      for (int cnt2 = need->vn_cnt; --cnt2 >= 0; )
	{
	  GElf_Vernaux auxmem;
	  GElf_Vernaux *aux = gelf_getvernaux (data, auxoffset, &auxmem);
	  if (unlikely (aux == NULL))
	    break;

	  printf (gettext ("  %#06x: Name: %s  Flags: %s  Version: %hu\n"),
		  auxoffset,
		  elf_strptr (ebl->elf, shdr->sh_link, aux->vna_name),
		  get_ver_flags (aux->vna_flags),
		  (unsigned short int) aux->vna_other);

	  auxoffset += aux->vna_next;
	}

      /* Find the next offset.  */
      offset += need->vn_next;
    }
}


static void
handle_verdef (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr)
{
  /* Get the data of the section.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    return;

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (unlikely (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  int class = gelf_getclass (ebl->elf);
  GElf_Shdr glink;
  printf (ngettext ("\
\nVersion definition section [%2u] '%s' contains %d entry:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    "\
\nVersion definition section [%2u] '%s' contains %d entries:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    shdr->sh_info),
	  (unsigned int) elf_ndxscn (scn),
	  elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
	  shdr->sh_info,
	  class == ELFCLASS32 ? 10 : 18, shdr->sh_addr,
	  shdr->sh_offset,
	  (unsigned int) shdr->sh_link,
	  elf_strptr (ebl->elf, shstrndx,
		      gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link),
				    &glink)->sh_name));

  unsigned int offset = 0;
  for (int cnt = shdr->sh_info; --cnt >= 0; )
    {
      /* Get the data at the next offset.  */
      GElf_Verdef defmem;
      GElf_Verdef *def = gelf_getverdef (data, offset, &defmem);
      if (unlikely (def == NULL))
	break;

      unsigned int auxoffset = offset + def->vd_aux;
      GElf_Verdaux auxmem;
      GElf_Verdaux *aux = gelf_getverdaux (data, auxoffset, &auxmem);
      if (unlikely (aux == NULL))
	break;

      printf (gettext ("\
  %#06x: Version: %hd  Flags: %s  Index: %hd  Cnt: %hd  Name: %s\n"),
	      offset, def->vd_version,
	      get_ver_flags (def->vd_flags),
	      def->vd_ndx,
	      def->vd_cnt,
	      elf_strptr (ebl->elf, shdr->sh_link, aux->vda_name));

      auxoffset += aux->vda_next;
      for (int cnt2 = 1; cnt2 < def->vd_cnt; ++cnt2)
	{
	  aux = gelf_getverdaux (data, auxoffset, &auxmem);
	  if (unlikely (aux == NULL))
	    break;

	  printf (gettext ("  %#06x: Parent %d: %s\n"),
		  auxoffset, cnt2,
		  elf_strptr (ebl->elf, shdr->sh_link, aux->vda_name));

	  auxoffset += aux->vda_next;
	}

      /* Find the next offset.  */
      offset += def->vd_next;
    }
}


static void
handle_versym (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr)
{
  int class = gelf_getclass (ebl->elf);
  const char **vername;
  const char **filename;

  /* Get the data of the section.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    return;

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (unlikely (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  /* We have to find the version definition section and extract the
     version names.  */
  Elf_Scn *defscn = NULL;
  Elf_Scn *needscn = NULL;

  Elf_Scn *verscn = NULL;
  while ((verscn = elf_nextscn (ebl->elf, verscn)) != NULL)
    {
      GElf_Shdr vershdr_mem;
      GElf_Shdr *vershdr = gelf_getshdr (verscn, &vershdr_mem);

      if (likely (vershdr != NULL))
	{
	  if (vershdr->sh_type == SHT_GNU_verdef)
	    defscn = verscn;
	  else if (vershdr->sh_type == SHT_GNU_verneed)
	    needscn = verscn;
	}
    }

  size_t nvername;
  if (defscn != NULL || needscn != NULL)
    {
      /* We have a version information (better should have).  Now get
	 the version names.  First find the maximum version number.  */
      nvername = 0;
      if (defscn != NULL)
	{
	  /* Run through the version definitions and find the highest
	     index.  */
	  unsigned int offset = 0;
	  Elf_Data *defdata;
	  GElf_Shdr defshdrmem;
	  GElf_Shdr *defshdr;

	  defdata = elf_getdata (defscn, NULL);
	  if (unlikely (defdata == NULL))
	    return;

	  defshdr = gelf_getshdr (defscn, &defshdrmem);
	  if (unlikely (defshdr == NULL))
	    return;

	  for (unsigned int cnt = 0; cnt < defshdr->sh_info; ++cnt)
	    {
	      GElf_Verdef defmem;
	      GElf_Verdef *def;

	      /* Get the data at the next offset.  */
	      def = gelf_getverdef (defdata, offset, &defmem);
	      if (unlikely (def == NULL))
		break;

	      nvername = MAX (nvername, (size_t) (def->vd_ndx & 0x7fff));

	      offset += def->vd_next;
	    }
	}
      if (needscn != NULL)
	{
	  unsigned int offset = 0;
	  Elf_Data *needdata;
	  GElf_Shdr needshdrmem;
	  GElf_Shdr *needshdr;

	  needdata = elf_getdata (needscn, NULL);
	  if (unlikely (needdata == NULL))
	    return;

	  needshdr = gelf_getshdr (needscn, &needshdrmem);
	  if (unlikely (needshdr == NULL))
	    return;

	  for (unsigned int cnt = 0; cnt < needshdr->sh_info; ++cnt)
	    {
	      GElf_Verneed needmem;
	      GElf_Verneed *need;
	      unsigned int auxoffset;
	      int cnt2;

	      /* Get the data at the next offset.  */
	      need = gelf_getverneed (needdata, offset, &needmem);
	      if (unlikely (need == NULL))
		break;

	      /* Run through the auxiliary entries.  */
	      auxoffset = offset + need->vn_aux;
	      for (cnt2 = need->vn_cnt; --cnt2 >= 0; )
		{
		  GElf_Vernaux auxmem;
		  GElf_Vernaux *aux;

		  aux = gelf_getvernaux (needdata, auxoffset, &auxmem);
		  if (unlikely (aux == NULL))
		    break;

		  nvername = MAX (nvername,
				  (size_t) (aux->vna_other & 0x7fff));

		  auxoffset += aux->vna_next;
		}

	      offset += need->vn_next;
	    }
	}

      /* This is the number of versions we know about.  */
      ++nvername;

      /* Allocate the array.  */
      vername = (const char **) alloca (nvername * sizeof (const char *));
      filename = (const char **) alloca (nvername * sizeof (const char *));

      /* Run through the data structures again and collect the strings.  */
      if (defscn != NULL)
	{
	  /* Run through the version definitions and find the highest
	     index.  */
	  unsigned int offset = 0;
	  Elf_Data *defdata;
	  GElf_Shdr defshdrmem;
	  GElf_Shdr *defshdr;

	  defdata = elf_getdata (defscn, NULL);
	  if (unlikely (defdata == NULL))
	    return;

	  defshdr = gelf_getshdr (defscn, &defshdrmem);
	  if (unlikely (defshdr == NULL))
	    return;

	  for (unsigned int cnt = 0; cnt < defshdr->sh_info; ++cnt)
	    {

	      /* Get the data at the next offset.  */
	      GElf_Verdef defmem;
	      GElf_Verdef *def = gelf_getverdef (defdata, offset, &defmem);
	      GElf_Verdaux auxmem;
	      GElf_Verdaux *aux = gelf_getverdaux (defdata,
						   offset + def->vd_aux,
						   &auxmem);
	      if (unlikely (def == NULL || aux == NULL))
		break;

	      vername[def->vd_ndx & 0x7fff]
		= elf_strptr (ebl->elf, defshdr->sh_link, aux->vda_name);
	      filename[def->vd_ndx & 0x7fff] = NULL;

	      offset += def->vd_next;
	    }
	}
      if (needscn != NULL)
	{
	  unsigned int offset = 0;

	  Elf_Data *needdata = elf_getdata (needscn, NULL);
	  GElf_Shdr needshdrmem;
	  GElf_Shdr *needshdr = gelf_getshdr (needscn, &needshdrmem);
	  if (unlikely (needdata == NULL || needshdr == NULL))
	    return;

	  for (unsigned int cnt = 0; cnt < needshdr->sh_info; ++cnt)
	    {
	      /* Get the data at the next offset.  */
	      GElf_Verneed needmem;
	      GElf_Verneed *need = gelf_getverneed (needdata, offset,
						    &needmem);
	      if (unlikely (need == NULL))
		break;

	      /* Run through the auxiliary entries.  */
	      unsigned int auxoffset = offset + need->vn_aux;
	      for (int cnt2 = need->vn_cnt; --cnt2 >= 0; )
		{
		  GElf_Vernaux auxmem;
		  GElf_Vernaux *aux = gelf_getvernaux (needdata, auxoffset,
						       &auxmem);
		  if (unlikely (aux == NULL))
		    break;

		  vername[aux->vna_other & 0x7fff]
		    = elf_strptr (ebl->elf, needshdr->sh_link, aux->vna_name);
		  filename[aux->vna_other & 0x7fff]
		    = elf_strptr (ebl->elf, needshdr->sh_link, need->vn_file);

		  auxoffset += aux->vna_next;
		}

	      offset += need->vn_next;
	    }
	}
    }
  else
    {
      vername = NULL;
      nvername = 1;
      filename = NULL;
    }

  /* Print the header.  */
  GElf_Shdr glink;
  printf (ngettext ("\
\nVersion symbols section [%2u] '%s' contains %d entry:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'",
		    "\
\nVersion symbols section [%2u] '%s' contains %d entries:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'",
		    shdr->sh_size / shdr->sh_entsize),
	  (unsigned int) elf_ndxscn (scn),
	  elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
	  (int) (shdr->sh_size / shdr->sh_entsize),
	  class == ELFCLASS32 ? 10 : 18, shdr->sh_addr,
	  shdr->sh_offset,
	  (unsigned int) shdr->sh_link,
	  elf_strptr (ebl->elf, shstrndx,
		      gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link),
				    &glink)->sh_name));

  /* Now we can finally look at the actual contents of this section.  */
  for (unsigned int cnt = 0; cnt < shdr->sh_size / shdr->sh_entsize; ++cnt)
    {
      if (cnt % 2 == 0)
	printf ("\n %4d:", cnt);

      GElf_Versym symmem;
      GElf_Versym *sym = gelf_getversym (data, cnt, &symmem);
      if (sym == NULL)
	break;

      switch (*sym)
	{
	  ssize_t n;
	case 0:
	  fputs_unlocked (gettext ("   0 *local*                     "),
			  stdout);
	  break;

	case 1:
	  fputs_unlocked (gettext ("   1 *global*                    "),
			  stdout);
	  break;

	default:
	  n = printf ("%4d%c%s",
		      *sym & 0x7fff, *sym & 0x8000 ? 'h' : ' ',
		      (unsigned int) (*sym & 0x7fff) < nvername
		      ? vername[*sym & 0x7fff] : "???");
	  if ((unsigned int) (*sym & 0x7fff) < nvername
	      && filename[*sym & 0x7fff] != NULL)
	    n += printf ("(%s)", filename[*sym & 0x7fff]);
	  printf ("%*s", MAX (0, 33 - (int) n), " ");
	  break;
	}
    }
  putchar_unlocked ('\n');
}


static void
print_hash_info (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr, size_t shstrndx,
		 uint_fast32_t maxlength, Elf32_Word nbucket,
		 uint_fast32_t nsyms, uint32_t *lengths, const char *extrastr)
{
  uint32_t *counts = (uint32_t *) xcalloc (maxlength + 1, sizeof (uint32_t));

  for (Elf32_Word cnt = 0; cnt < nbucket; ++cnt)
    ++counts[lengths[cnt]];

  GElf_Shdr glink;
  printf (ngettext ("\
\nHistogram for bucket list length in section [%2u] '%s' (total of %d bucket):\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    "\
\nHistogram for bucket list length in section [%2u] '%s' (total of %d buckets):\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    nbucket),
	  (unsigned int) elf_ndxscn (scn),
	  elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
	  (int) nbucket,
	  gelf_getclass (ebl->elf) == ELFCLASS32 ? 10 : 18,
	  shdr->sh_addr,
	  shdr->sh_offset,
	  (unsigned int) shdr->sh_link,
	  elf_strptr (ebl->elf, shstrndx,
		      gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link),
				    &glink)->sh_name));

  if (extrastr != NULL)
    fputs (extrastr, stdout);

  if (likely (nbucket > 0))
    {
      uint64_t success = 0;

      /* xgettext:no-c-format */
      fputs_unlocked (gettext ("\
 Length  Number  % of total  Coverage\n"), stdout);
      printf (gettext ("      0  %6" PRIu32 "      %5.1f%%\n"),
	      counts[0], (counts[0] * 100.0) / nbucket);

      uint64_t nzero_counts = 0;
      for (Elf32_Word cnt = 1; cnt <= maxlength; ++cnt)
	{
	  nzero_counts += counts[cnt] * cnt;
	  printf (gettext ("\
%7d  %6" PRIu32 "      %5.1f%%    %5.1f%%\n"),
		  (int) cnt, counts[cnt], (counts[cnt] * 100.0) / nbucket,
		  (nzero_counts * 100.0) / nsyms);
	}

      Elf32_Word acc = 0;
      for (Elf32_Word cnt = 1; cnt <= maxlength; ++cnt)
	{
	  acc += cnt;
	  success += counts[cnt] * acc;
	}

      printf (gettext ("\
 Average number of tests:   successful lookup: %f\n\
			  unsuccessful lookup: %f\n"),
	      (double) success / (double) nzero_counts,
	      (double) nzero_counts / (double) nbucket);
    }

  free (counts);
}


/* This function handles the traditional System V-style hash table format.  */
static void
handle_sysv_hash (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr, size_t shstrndx)
{
  Elf_Data *data = elf_getdata (scn, NULL);
  if (unlikely (data == NULL))
    {
      error (0, 0, gettext ("cannot get data for section %d: %s"),
	     (int) elf_ndxscn (scn), elf_errmsg (-1));
      return;
    }

  Elf32_Word nbucket = ((Elf32_Word *) data->d_buf)[0];
  Elf32_Word nchain = ((Elf32_Word *) data->d_buf)[1];
  Elf32_Word *bucket = &((Elf32_Word *) data->d_buf)[2];
  Elf32_Word *chain = &((Elf32_Word *) data->d_buf)[2 + nbucket];

  uint32_t *lengths = (uint32_t *) xcalloc (nbucket, sizeof (uint32_t));

  uint_fast32_t maxlength = 0;
  uint_fast32_t nsyms = 0;
  for (Elf32_Word cnt = 0; cnt < nbucket; ++cnt)
    {
      Elf32_Word inner = bucket[cnt];
      while (inner > 0 && inner < nchain)
	{
	  ++nsyms;
	  if (maxlength < ++lengths[cnt])
	    ++maxlength;

	  inner = chain[inner];
	}
    }

  print_hash_info (ebl, scn, shdr, shstrndx, maxlength, nbucket, nsyms,
		   lengths, NULL);

  free (lengths);
}


/* This function handles the incorrect, System V-style hash table
   format some 64-bit architectures use.  */
static void
handle_sysv_hash64 (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr, size_t shstrndx)
{
  Elf_Data *data = elf_getdata (scn, NULL);
  if (unlikely (data == NULL))
    {
      error (0, 0, gettext ("cannot get data for section %d: %s"),
	     (int) elf_ndxscn (scn), elf_errmsg (-1));
      return;
    }

  Elf64_Xword nbucket = ((Elf64_Xword *) data->d_buf)[0];
  Elf64_Xword nchain = ((Elf64_Xword *) data->d_buf)[1];
  Elf64_Xword *bucket = &((Elf64_Xword *) data->d_buf)[2];
  Elf64_Xword *chain = &((Elf64_Xword *) data->d_buf)[2 + nbucket];

  uint32_t *lengths = (uint32_t *) xcalloc (nbucket, sizeof (uint32_t));

  uint_fast32_t maxlength = 0;
  uint_fast32_t nsyms = 0;
  for (Elf64_Xword cnt = 0; cnt < nbucket; ++cnt)
    {
      Elf64_Xword inner = bucket[cnt];
      while (inner > 0 && inner < nchain)
	{
	  ++nsyms;
	  if (maxlength < ++lengths[cnt])
	    ++maxlength;

	  inner = chain[inner];
	}
    }

  print_hash_info (ebl, scn, shdr, shstrndx, maxlength, nbucket, nsyms,
		   lengths, NULL);

  free (lengths);
}


/* This function handles the GNU-style hash table format.  */
static void
handle_gnu_hash (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr, size_t shstrndx)
{
  Elf_Data *data = elf_getdata (scn, NULL);
  if (unlikely (data == NULL))
    {
      error (0, 0, gettext ("cannot get data for section %d: %s"),
	     (int) elf_ndxscn (scn), elf_errmsg (-1));
      return;
    }

  Elf32_Word nbucket = ((Elf32_Word *) data->d_buf)[0];
  Elf32_Word symbias = ((Elf32_Word *) data->d_buf)[1];

  /* Next comes the size of the bitmap.  It's measured in words for
     the architecture.  It's 32 bits for 32 bit archs, and 64 bits for
     64 bit archs.  */
  Elf32_Word bitmask_words = ((Elf32_Word *) data->d_buf)[2];
  if (gelf_getclass (ebl->elf) == ELFCLASS64)
    bitmask_words *= 2;

  Elf32_Word shift = ((Elf32_Word *) data->d_buf)[3];

  uint32_t *lengths = (uint32_t *) xcalloc (nbucket, sizeof (uint32_t));

  Elf32_Word *bitmask = &((Elf32_Word *) data->d_buf)[4];
  Elf32_Word *bucket = &((Elf32_Word *) data->d_buf)[4 + bitmask_words];
  Elf32_Word *chain = &((Elf32_Word *) data->d_buf)[4 + bitmask_words
						    + nbucket];

  /* Compute distribution of chain lengths.  */
  uint_fast32_t maxlength = 0;
  uint_fast32_t nsyms = 0;
  for (Elf32_Word cnt = 0; cnt < nbucket; ++cnt)
    if (bucket[cnt] != 0)
      {
	Elf32_Word inner = bucket[cnt] - symbias;
	do
	  {
	    ++nsyms;
	    if (maxlength < ++lengths[cnt])
	      ++maxlength;
	  }
	while ((chain[inner++] & 1) == 0);
      }

  /* Count bits in bitmask.  */
  uint_fast32_t nbits = 0;
  for (Elf32_Word cnt = 0; cnt < bitmask_words; ++cnt)
    {
      uint_fast32_t word = bitmask[cnt];

      word = (word & 0x55555555) + ((word >> 1) & 0x55555555);
      word = (word & 0x33333333) + ((word >> 2) & 0x33333333);
      word = (word & 0x0f0f0f0f) + ((word >> 4) & 0x0f0f0f0f);
      word = (word & 0x00ff00ff) + ((word >> 8) & 0x00ff00ff);
      nbits += (word & 0x0000ffff) + ((word >> 16) & 0x0000ffff);
    }

  char *str;
  if (unlikely (asprintf (&str, gettext ("\
 Symbol Bias: %u\n\
 Bitmask Size: %zu bytes  %" PRIuFAST32 "%% bits set  2nd hash shift: %u\n"),
			  (unsigned int) symbias,
			  bitmask_words * sizeof (Elf32_Word),
			  ((nbits * 100 + 50)
			   / (uint_fast32_t) (bitmask_words
					      * sizeof (Elf32_Word) * 8)),
			  (unsigned int) shift) == -1))
    error (EXIT_FAILURE, 0, gettext ("memory exhausted"));

  print_hash_info (ebl, scn, shdr, shstrndx, maxlength, nbucket, nsyms,
		   lengths, str);

  free (str);
  free (lengths);
}


/* Find the symbol table(s).  For this we have to search through the
   section table.  */
static void
handle_hash (Ebl *ebl)
{
  /* Get the section header string table index.  */
  size_t shstrndx;
  if (unlikely (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  Elf_Scn *scn = NULL;
  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
      /* Handle the section if it is a symbol table.  */
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (likely (shdr != NULL))
	{
	  if (shdr->sh_type == SHT_HASH)
	    {
	      if (ebl_sysvhash_entrysize (ebl) == sizeof (Elf64_Xword))
		handle_sysv_hash64 (ebl, scn, shdr, shstrndx);
	      else
		handle_sysv_hash (ebl, scn, shdr, shstrndx);
	    }
	  else if (shdr->sh_type == SHT_GNU_HASH)
	    handle_gnu_hash (ebl, scn, shdr, shstrndx);
	}
    }
}


static void
print_liblist (Ebl *ebl)
{
  /* Find the library list sections.  For this we have to search
     through the section table.  */
  Elf_Scn *scn = NULL;

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (unlikely (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (shdr != NULL && shdr->sh_type == SHT_GNU_LIBLIST)
	{
	  int nentries = shdr->sh_size / shdr->sh_entsize;
	  printf (ngettext ("\
\nLibrary list section [%2zu] '%s' at offset %#0" PRIx64 " contains %d entry:\n",
			    "\
\nLibrary list section [%2zu] '%s' at offset %#0" PRIx64 " contains %d entries:\n",
			    nentries),
		  elf_ndxscn (scn),
		  elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
		  shdr->sh_offset,
		  nentries);

	  Elf_Data *data = elf_getdata (scn, NULL);
	  if (data == NULL)
	    return;

	  puts (gettext ("\
       Library                       Time Stamp          Checksum Version Flags"));

	  for (int cnt = 0; cnt < nentries; ++cnt)
	    {
	      GElf_Lib lib_mem;
	      GElf_Lib *lib = gelf_getlib (data, cnt, &lib_mem);
	      if (unlikely (lib == NULL))
		continue;

	      time_t t = (time_t) lib->l_time_stamp;
	      struct tm *tm = gmtime (&t);
	      if (unlikely (tm == NULL))
		continue;

	      printf ("  [%2d] %-29s %04u-%02u-%02uT%02u:%02u:%02u %08x %-7u %u\n",
		      cnt, elf_strptr (ebl->elf, shdr->sh_link, lib->l_name),
		      tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		      tm->tm_hour, tm->tm_min, tm->tm_sec,
		      (unsigned int) lib->l_checksum,
		      (unsigned int) lib->l_version,
		      (unsigned int) lib->l_flags);
	    }
	}
    }
}

static void
print_attributes (Ebl *ebl, const GElf_Ehdr *ehdr)
{
  /* Find the object attributes sections.  For this we have to search
     through the section table.  */
  Elf_Scn *scn = NULL;

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (unlikely (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (shdr == NULL || (shdr->sh_type != SHT_GNU_ATTRIBUTES
			   && (shdr->sh_type != SHT_ARM_ATTRIBUTES
			       || ehdr->e_machine != EM_ARM)))
	continue;

      printf (gettext ("\
\nObject attributes section [%2zu] '%s' of %" PRIu64
		       " bytes at offset %#0" PRIx64 ":\n"),
	      elf_ndxscn (scn),
	      elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
	      shdr->sh_size, shdr->sh_offset);

      Elf_Data *data = elf_rawdata (scn, NULL);
      if (data == NULL)
	return;

      const unsigned char *p = data->d_buf;

      if (unlikely (*p++ != 'A'))
	return;

      fputs_unlocked (gettext ("  Owner          Size\n"), stdout);

      inline size_t left (void)
      {
	return (const unsigned char *) data->d_buf + data->d_size - p;
      }

      while (left () >= 4)
	{
	  uint32_t len;
	  memcpy (&len, p, sizeof len);

	  if (MY_ELFDATA != ehdr->e_ident[EI_DATA])
	    CONVERT (len);

	  if (unlikely (len > left ()))
	    break;

	  const unsigned char *name = p + sizeof len;
	  p += len;

	  unsigned const char *q = memchr (name, '\0', len);
	  if (unlikely (q == NULL))
	    continue;
	  ++q;

	  printf (gettext ("  %-13s  %4" PRIu32 "\n"), name, len);

	  if (shdr->sh_type != SHT_GNU_ATTRIBUTES
	      || (q - name == sizeof "gnu"
		  && !memcmp (name, "gnu", sizeof "gnu")))
	    while (q < p)
	      {
		const unsigned char *const sub = q;

		unsigned int subsection_tag;
		get_uleb128 (subsection_tag, q);
		if (unlikely (q >= p))
		  break;

		uint32_t subsection_len;
		if (unlikely (p - sub < (ptrdiff_t) sizeof subsection_len))
		  break;

		memcpy (&subsection_len, q, sizeof subsection_len);

		if (MY_ELFDATA != ehdr->e_ident[EI_DATA])
		  CONVERT (subsection_len);

		if (unlikely (p - sub < (ptrdiff_t) subsection_len))
		  break;

		const unsigned char *r = q + sizeof subsection_len;
		q = sub + subsection_len;

		switch (subsection_tag)
		  {
		  default:
		    printf (gettext ("    %-4u %12" PRIu32 "\n"),
			    subsection_tag, subsection_len);
		    break;

		  case 1:	/* Tag_File */
		    printf (gettext ("    File: %11" PRIu32 "\n"),
			    subsection_len);

		    while (r < q)
		      {
			unsigned int tag;
			get_uleb128 (tag, r);
			if (unlikely (r >= q))
			  break;

			uint64_t value = 0;
			const char *string = NULL;
			if (tag == 32 || (tag & 1) == 0)
			  {
			    get_uleb128 (value, r);
			    if (r > q)
			      break;
			  }
			if (tag == 32 || (tag & 1) != 0)
			  {
			    r = memchr (r, '\0', q - r);
			    if (r == NULL)
			      break;
			    ++r;
			  }

			const char *tag_name = NULL;
			const char *value_name = NULL;
			ebl_check_object_attribute (ebl, (const char *) name,
						    tag, value,
						    &tag_name, &value_name);

			if (tag_name != NULL)
			  {
			    if (tag == 32)
			      printf (gettext ("      %s: %" PRId64 ", %s\n"),
				      tag_name, value, string);
			    else if (string == NULL && value_name == NULL)
			      printf (gettext ("      %s: %" PRId64 "\n"),
				      tag_name, value);
			    else
			      printf (gettext ("      %s: %s\n"),
				      tag_name, string ?: value_name);
			  }
			else
			  {
			    assert (tag != 32);
			    if (string == NULL)
			      printf (gettext ("      %u: %" PRId64 "\n"),
				      tag, value);
			    else
			      printf (gettext ("      %u: %s\n"),
				      tag, string);
			  }
		      }
		  }
	      }
	}
    }
}


static char *
format_dwarf_addr (Dwfl_Module *dwflmod,
		   int address_size, Dwarf_Addr address, Dwarf_Addr raw)
{
  /* See if there is a name we can give for this address.  */
  GElf_Sym sym;
  GElf_Off off = 0;
  const char *name = (print_address_names && ! print_unresolved_addresses)
    ? dwfl_module_addrinfo (dwflmod, address, &off, &sym, NULL, NULL, NULL)
    : NULL;

  const char *scn;
  if (print_unresolved_addresses)
    {
      address = raw;
      scn = NULL;
    }
  else
    {
      /* Relativize the address.  */
      int n = dwfl_module_relocations (dwflmod);
      int i = n < 1 ? -1 : dwfl_module_relocate_address (dwflmod, &address);

      /* In an ET_REL file there is a section name to refer to.  */
      scn = (i < 0 ? NULL
	     : dwfl_module_relocation_info (dwflmod, i, NULL));
    }

  char *result;
  if ((name != NULL
       ? (off != 0
	  ? (scn != NULL
	     ? (address_size == 0
		? asprintf (&result,
			    gettext ("%s+%#" PRIx64 " <%s+%#" PRIx64 ">"),
			    scn, address, name, off)
		: asprintf (&result,
			    gettext ("%s+%#0*" PRIx64 " <%s+%#" PRIx64 ">"),
			    scn, 2 + address_size * 2, address,
			    name, off))
	     : (address_size == 0
		? asprintf (&result,
			    gettext ("%#" PRIx64 " <%s+%#" PRIx64 ">"),
			    address, name, off)
		: asprintf (&result,
			    gettext ("%#0*" PRIx64 " <%s+%#" PRIx64 ">"),
			    2 + address_size * 2, address,
			    name, off)))
	  : (scn != NULL
	     ? (address_size == 0
		? asprintf (&result,
			    gettext ("%s+%#" PRIx64 " <%s>"),
			    scn, address, name)
		: asprintf (&result,
			    gettext ("%s+%#0*" PRIx64 " <%s>"),
			    scn, 2 + address_size * 2, address, name))
	     : (address_size == 0
		? asprintf (&result,
			    gettext ("%#" PRIx64 " <%s>"),
			    address, name)
		: asprintf (&result,
			    gettext ("%#0*" PRIx64 " <%s>"),
			    2 + address_size * 2, address, name))))
       : (scn != NULL
	  ? (address_size == 0
	     ? asprintf (&result,
			 gettext ("%s+%#" PRIx64),
			 scn, address)
	     : asprintf (&result,
			 gettext ("%s+%#0*" PRIx64),
			 scn, 2 + address_size * 2, address))
	  : (address_size == 0
	     ? asprintf (&result,
			 "%#" PRIx64,
			 address)
	     : asprintf (&result,
			 "%#0*" PRIx64,
			 2 + address_size * 2, address)))) < 0)
    error (EXIT_FAILURE, 0, _("memory exhausted"));

  return result;
}

static const char *
dwarf_tag_string (unsigned int tag)
{
  switch (tag)
    {
#define ONE_KNOWN_DW_TAG(NAME, CODE) case CODE: return #NAME;
      ALL_KNOWN_DW_TAG
#undef ONE_KNOWN_DW_TAG
    default:
      return NULL;
    }
}


static const char *
dwarf_attr_string (unsigned int attrnum)
{
  switch (attrnum)
    {
#define ONE_KNOWN_DW_AT(NAME, CODE) case CODE: return #NAME;
      ALL_KNOWN_DW_AT
#undef ONE_KNOWN_DW_AT
    default:
      return NULL;
    }
}


static const char *
dwarf_form_string (unsigned int form)
{
  switch (form)
    {
#define ONE_KNOWN_DW_FORM_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_FORM (NAME, CODE)
#define ONE_KNOWN_DW_FORM(NAME, CODE) case CODE: return #NAME;
      ALL_KNOWN_DW_FORM
#undef ONE_KNOWN_DW_FORM
#undef ONE_KNOWN_DW_FORM_DESC
    default:
      return NULL;
    }
}


static const char *
dwarf_lang_string (unsigned int lang)
{
  switch (lang)
    {
#define ONE_KNOWN_DW_LANG_DESC(NAME, CODE, DESC) case CODE: return #NAME;
      ALL_KNOWN_DW_LANG
#undef ONE_KNOWN_DW_LANG_DESC
    default:
      return NULL;
    }
}


static const char *
dwarf_inline_string (unsigned int code)
{
  static const char *const known[] =
    {
#define ONE_KNOWN_DW_INL(NAME, CODE) [CODE] = #NAME,
      ALL_KNOWN_DW_INL
#undef ONE_KNOWN_DW_INL
    };

  if (likely (code < sizeof (known) / sizeof (known[0])))
    return known[code];

  return NULL;
}


static const char *
dwarf_encoding_string (unsigned int code)
{
  static const char *const known[] =
    {
#define ONE_KNOWN_DW_ATE(NAME, CODE) [CODE] = #NAME,
      ALL_KNOWN_DW_ATE
#undef ONE_KNOWN_DW_ATE
    };

  if (likely (code < sizeof (known) / sizeof (known[0])))
    return known[code];

  return NULL;
}


static const char *
dwarf_access_string (unsigned int code)
{
  static const char *const known[] =
    {
#define ONE_KNOWN_DW_ACCESS(NAME, CODE) [CODE] = #NAME,
      ALL_KNOWN_DW_ACCESS
#undef ONE_KNOWN_DW_ACCESS
    };

  if (likely (code < sizeof (known) / sizeof (known[0])))
    return known[code];

  return NULL;
}


static const char *
dwarf_visibility_string (unsigned int code)
{
  static const char *const known[] =
    {
#define ONE_KNOWN_DW_VIS(NAME, CODE) [CODE] = #NAME,
      ALL_KNOWN_DW_VIS
#undef ONE_KNOWN_DW_VIS
    };

  if (likely (code < sizeof (known) / sizeof (known[0])))
    return known[code];

  return NULL;
}


static const char *
dwarf_virtuality_string (unsigned int code)
{
  static const char *const known[] =
    {
#define ONE_KNOWN_DW_VIRTUALITY(NAME, CODE) [CODE] = #NAME,
      ALL_KNOWN_DW_VIRTUALITY
#undef ONE_KNOWN_DW_VIRTUALITY
    };

  if (likely (code < sizeof (known) / sizeof (known[0])))
    return known[code];

  return NULL;
}


static const char *
dwarf_identifier_case_string (unsigned int code)
{
  static const char *const known[] =
    {
#define ONE_KNOWN_DW_ID(NAME, CODE) [CODE] = #NAME,
      ALL_KNOWN_DW_ID
#undef ONE_KNOWN_DW_ID
    };

  if (likely (code < sizeof (known) / sizeof (known[0])))
    return known[code];

  return NULL;
}


static const char *
dwarf_calling_convention_string (unsigned int code)
{
  static const char *const known[] =
    {
#define ONE_KNOWN_DW_CC(NAME, CODE) [CODE] = #NAME,
      ALL_KNOWN_DW_CC
#undef ONE_KNOWN_DW_CC
    };

  if (likely (code < sizeof (known) / sizeof (known[0])))
    return known[code];

  return NULL;
}


static const char *
dwarf_ordering_string (unsigned int code)
{
  static const char *const known[] =
    {
#define ONE_KNOWN_DW_ORD(NAME, CODE) [CODE] = #NAME,
      ALL_KNOWN_DW_ORD
#undef ONE_KNOWN_DW_ORD
    };

  if (likely (code < sizeof (known) / sizeof (known[0])))
    return known[code];

  return NULL;
}


static const char *
dwarf_discr_list_string (unsigned int code)
{
  static const char *const known[] =
    {
#define ONE_KNOWN_DW_DSC(NAME, CODE) [CODE] = #NAME,
      ALL_KNOWN_DW_DSC
#undef ONE_KNOWN_DW_DSC
    };

  if (likely (code < sizeof (known) / sizeof (known[0])))
    return known[code];

  return NULL;
}


static const char *
dwarf_locexpr_opcode_string (unsigned int code)
{
  static const char *const known[] =
    {
      /* Normally we can't affort building huge table of 64K entries,
	 most of them zero, just because there are a couple defined
	 values at the far end.  In case of opcodes, it's OK.  */
#define ONE_KNOWN_DW_OP_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_OP (NAME, CODE)
#define ONE_KNOWN_DW_OP(NAME, CODE) [CODE] = #NAME,
      ALL_KNOWN_DW_OP
#undef ONE_KNOWN_DW_OP
#undef ONE_KNOWN_DW_OP_DESC
    };

  if (likely (code < sizeof (known) / sizeof (known[0])))
    return known[code];

  return NULL;
}


/* Used by all dwarf_foo_name functions.  */
static const char *
string_or_unknown (const char *known, unsigned int code,
                   unsigned int lo_user, unsigned int hi_user,
		   bool print_unknown_num)
{
  static char unknown_buf[20];

  if (likely (known != NULL))
    return known;

  if (lo_user != 0 && code >= lo_user && code <= hi_user)
    {
      snprintf (unknown_buf, sizeof unknown_buf, "lo_user+%#x",
		code - lo_user);
      return unknown_buf;
    }

  if (print_unknown_num)
    {
      snprintf (unknown_buf, sizeof unknown_buf, "??? (%#x)", code);
      return unknown_buf;
    }

  return "???";
}


static const char *
dwarf_tag_name (unsigned int tag)
{
  const char *ret = dwarf_tag_string (tag);
  return string_or_unknown (ret, tag, DW_TAG_lo_user, DW_TAG_hi_user, true);
}

static const char *
dwarf_attr_name (unsigned int attr)
{
  const char *ret = dwarf_attr_string (attr);
  return string_or_unknown (ret, attr, DW_AT_lo_user, DW_AT_hi_user, true);
}


static const char *
dwarf_form_name (unsigned int form)
{
  const char *ret = dwarf_form_string (form);
  return string_or_unknown (ret, form, 0, 0, true);
}


static const char *
dwarf_lang_name (unsigned int lang)
{
  const char *ret = dwarf_lang_string (lang);
  return string_or_unknown (ret, lang, DW_LANG_lo_user, DW_LANG_hi_user, false);
}


static const char *
dwarf_inline_name (unsigned int code)
{
  const char *ret = dwarf_inline_string (code);
  return string_or_unknown (ret, code, 0, 0, false);
}


static const char *
dwarf_encoding_name (unsigned int code)
{
  const char *ret = dwarf_encoding_string (code);
  return string_or_unknown (ret, code, DW_ATE_lo_user, DW_ATE_hi_user, false);
}


static const char *
dwarf_access_name (unsigned int code)
{
  const char *ret = dwarf_access_string (code);
  return string_or_unknown (ret, code, 0, 0, false);
}


static const char *
dwarf_visibility_name (unsigned int code)
{
  const char *ret = dwarf_visibility_string (code);
  return string_or_unknown (ret, code, 0, 0, false);
}


static const char *
dwarf_virtuality_name (unsigned int code)
{
  const char *ret = dwarf_virtuality_string (code);
  return string_or_unknown (ret, code, 0, 0, false);
}


static const char *
dwarf_identifier_case_name (unsigned int code)
{
  const char *ret = dwarf_identifier_case_string (code);
  return string_or_unknown (ret, code, 0, 0, false);
}


static const char *
dwarf_calling_convention_name (unsigned int code)
{
  const char *ret = dwarf_calling_convention_string (code);
  return string_or_unknown (ret, code, DW_CC_lo_user, DW_CC_hi_user, false);
}


static const char *
dwarf_ordering_name (unsigned int code)
{
  const char *ret = dwarf_ordering_string (code);
  return string_or_unknown (ret, code, 0, 0, false);
}


static const char *
dwarf_discr_list_name (unsigned int code)
{
  const char *ret = dwarf_discr_list_string (code);
  return string_or_unknown (ret, code, 0, 0, false);
}


static void
print_block (size_t n, const void *block)
{
  if (n == 0)
    puts (_("empty block"));
  else
    {
      printf (_("%zu byte block:"), n);
      const unsigned char *data = block;
      do
	printf (" %02x", *data++);
      while (--n > 0);
      putchar ('\n');
    }
}

static void
print_ops (Dwfl_Module *dwflmod, Dwarf *dbg, int indent, int indentrest,
	   unsigned int vers, unsigned int addrsize, unsigned int offset_size,
	   struct Dwarf_CU *cu, Dwarf_Word len, const unsigned char *data)
{
  const unsigned int ref_size = vers < 3 ? addrsize : offset_size;

  if (len == 0)
    {
      printf ("%*s(empty)\n", indent, "");
      return;
    }

#define NEED(n)		if (len < (Dwarf_Word) (n)) goto invalid
#define CONSUME(n)	NEED (n); else len -= (n)

  Dwarf_Word offset = 0;
  while (len-- > 0)
    {
      uint_fast8_t op = *data++;

      const char *op_name = dwarf_locexpr_opcode_string (op);
      if (unlikely (op_name == NULL))
	{
	  static char buf[20];
	  if (op >= DW_OP_lo_user)
	    snprintf (buf, sizeof buf, "lo_user+%#x", op - DW_OP_lo_user);
	  else
	    snprintf (buf, sizeof buf, "??? (%#x)", op);
	  op_name = buf;
	}

      switch (op)
	{
	case DW_OP_addr:;
	  /* Address operand.  */
	  Dwarf_Word addr;
	  NEED (addrsize);
	  if (addrsize == 4)
	    addr = read_4ubyte_unaligned (dbg, data);
	  else
	    {
	      assert (addrsize == 8);
	      addr = read_8ubyte_unaligned (dbg, data);
	    }
	  data += addrsize;
	  CONSUME (addrsize);

	  char *a = format_dwarf_addr (dwflmod, 0, addr, addr);
	  printf ("%*s[%4" PRIuMAX "] %s %s\n",
		  indent, "", (uintmax_t) offset, op_name, a);
	  free (a);

	  offset += 1 + addrsize;
	  break;

	case DW_OP_call_ref:
	  /* Offset operand.  */
	  NEED (ref_size);
	  if (ref_size == 4)
	    addr = read_4ubyte_unaligned (dbg, data);
	  else
	    {
	      assert (ref_size == 8);
	      addr = read_8ubyte_unaligned (dbg, data);
	    }
	  data += ref_size;
	  CONSUME (ref_size);

	  printf ("%*s[%4" PRIuMAX "] %s %#" PRIxMAX "\n",
		  indent, "", (uintmax_t) offset,
		  op_name, (uintmax_t) addr);
	  offset += 1 + ref_size;
	  break;

	case DW_OP_deref_size:
	case DW_OP_xderef_size:
	case DW_OP_pick:
	case DW_OP_const1u:
	  // XXX value might be modified by relocation
	  NEED (1);
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIu8 "\n",
		  indent, "", (uintmax_t) offset,
		  op_name, *((uint8_t *) data));
	  ++data;
	  --len;
	  offset += 2;
	  break;

	case DW_OP_const2u:
	  NEED (2);
	  // XXX value might be modified by relocation
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIu16 "\n",
		  indent, "", (uintmax_t) offset,
		  op_name, read_2ubyte_unaligned (dbg, data));
	  CONSUME (2);
	  data += 2;
	  offset += 3;
	  break;

	case DW_OP_const4u:
	  NEED (4);
	  // XXX value might be modified by relocation
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIu32 "\n",
		  indent, "", (uintmax_t) offset,
		  op_name, read_4ubyte_unaligned (dbg, data));
	  CONSUME (4);
	  data += 4;
	  offset += 5;
	  break;

	case DW_OP_const8u:
	  NEED (8);
	  // XXX value might be modified by relocation
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIu64 "\n",
		  indent, "", (uintmax_t) offset,
		  op_name, (uint64_t) read_8ubyte_unaligned (dbg, data));
	  CONSUME (8);
	  data += 8;
	  offset += 9;
	  break;

	case DW_OP_const1s:
	  NEED (1);
	  // XXX value might be modified by relocation
	  printf ("%*s[%4" PRIuMAX "] %s %" PRId8 "\n",
		  indent, "", (uintmax_t) offset,
		  op_name, *((int8_t *) data));
	  ++data;
	  --len;
	  offset += 2;
	  break;

	case DW_OP_const2s:
	  NEED (2);
	  // XXX value might be modified by relocation
	  printf ("%*s[%4" PRIuMAX "] %s %" PRId16 "\n",
		  indent, "", (uintmax_t) offset,
		  op_name, read_2sbyte_unaligned (dbg, data));
	  CONSUME (2);
	  data += 2;
	  offset += 3;
	  break;

	case DW_OP_const4s:
	  NEED (4);
	  // XXX value might be modified by relocation
	  printf ("%*s[%4" PRIuMAX "] %s %" PRId32 "\n",
		  indent, "", (uintmax_t) offset,
		  op_name, read_4sbyte_unaligned (dbg, data));
	  CONSUME (4);
	  data += 4;
	  offset += 5;
	  break;

	case DW_OP_const8s:
	  NEED (8);
	  // XXX value might be modified by relocation
	  printf ("%*s[%4" PRIuMAX "] %s %" PRId64 "\n",
		  indent, "", (uintmax_t) offset,
		  op_name, read_8sbyte_unaligned (dbg, data));
	  CONSUME (8);
	  data += 8;
	  offset += 9;
	  break;

	case DW_OP_piece:
	case DW_OP_regx:
	case DW_OP_plus_uconst:
	case DW_OP_constu:;
	  const unsigned char *start = data;
	  uint64_t uleb;
	  NEED (1);
	  get_uleb128 (uleb, data); /* XXX check overrun */
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIu64 "\n",
		  indent, "", (uintmax_t) offset, op_name, uleb);
	  CONSUME (data - start);
	  offset += 1 + (data - start);
	  break;

	case DW_OP_bit_piece:
	  start = data;
	  uint64_t uleb2;
	  NEED (2);
	  get_uleb128 (uleb, data); /* XXX check overrun */
	  get_uleb128 (uleb2, data); /* XXX check overrun */
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIu64 ", %" PRIu64 "\n",
		  indent, "", (uintmax_t) offset, op_name, uleb, uleb2);
	  CONSUME (data - start);
	  offset += 1 + (data - start);
	  break;

	case DW_OP_fbreg:
	case DW_OP_breg0 ... DW_OP_breg31:
	case DW_OP_consts:
	  start = data;
	  int64_t sleb;
	  NEED (1);
	  get_sleb128 (sleb, data); /* XXX check overrun */
	  printf ("%*s[%4" PRIuMAX "] %s %" PRId64 "\n",
		  indent, "", (uintmax_t) offset, op_name, sleb);
	  CONSUME (data - start);
	  offset += 1 + (data - start);
	  break;

	case DW_OP_bregx:
	  start = data;
	  NEED (2);
	  get_uleb128 (uleb, data); /* XXX check overrun */
	  get_sleb128 (sleb, data); /* XXX check overrun */
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIu64 " %" PRId64 "\n",
		  indent, "", (uintmax_t) offset, op_name, uleb, sleb);
	  CONSUME (data - start);
	  offset += 1 + (data - start);
	  break;

	case DW_OP_call2:
	  NEED (2);
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIu16 "\n",
		  indent, "", (uintmax_t) offset, op_name,
		  read_2ubyte_unaligned (dbg, data));
	  CONSUME (2);
	  offset += 3;
	  break;

	case DW_OP_call4:
	  NEED (4);
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIu32 "\n",
		  indent, "", (uintmax_t) offset, op_name,
		  read_4ubyte_unaligned (dbg, data));
	  CONSUME (4);
	  offset += 5;
	  break;

	case DW_OP_skip:
	case DW_OP_bra:
	  NEED (2);
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIuMAX "\n",
		  indent, "", (uintmax_t) offset, op_name,
		  (uintmax_t) (offset + read_2sbyte_unaligned (dbg, data) + 3));
	  CONSUME (2);
	  data += 2;
	  offset += 3;
	  break;

	case DW_OP_implicit_value:
	  start = data;
	  NEED (1);
	  get_uleb128 (uleb, data); /* XXX check overrun */
	  printf ("%*s[%4" PRIuMAX "] %s: ",
		  indent, "", (uintmax_t) offset, op_name);
	  NEED (uleb);
	  print_block (uleb, data);
	  data += uleb;
	  CONSUME (data - start);
	  offset += 1 + (data - start);
	  break;

	case DW_OP_GNU_implicit_pointer:
	  /* DIE offset operand.  */
	  start = data;
	  NEED (ref_size + 1);
	  if (ref_size == 4)
	    addr = read_4ubyte_unaligned (dbg, data);
	  else
	    {
	      assert (ref_size == 8);
	      addr = read_8ubyte_unaligned (dbg, data);
	    }
	  data += ref_size;
	  /* Byte offset operand.  */
	  get_sleb128 (sleb, data); /* XXX check overrun */

	  printf ("%*s[%4" PRIuMAX "] %s [%6" PRIxMAX "] %+" PRId64 "\n",
		  indent, "", (intmax_t) offset,
		  op_name, (uintmax_t) addr, sleb);
	  CONSUME (data - start);
	  offset += 1 + (data - start);
	  break;

	case DW_OP_GNU_entry_value:
	  /* Size plus expression block.  */
	  start = data;
	  NEED (1);
	  get_uleb128 (uleb, data); /* XXX check overrun */
	  printf ("%*s[%4" PRIuMAX "] %s:\n",
		  indent, "", (uintmax_t) offset, op_name);
	  NEED (uleb);
	  print_ops (dwflmod, dbg, indent + 6, indent + 6, vers,
		     addrsize, offset_size, cu, uleb, data);
	  data += uleb;
	  CONSUME (data - start);
	  offset += 1 + (data - start);
	  break;

	case DW_OP_GNU_const_type:
	  /* uleb128 CU relative DW_TAG_base_type DIE offset, 1-byte
	     unsigned size plus block.  */
	  start = data;
	  NEED (2);
	  get_uleb128 (uleb, data); /* XXX check overrun */
	  if (! print_unresolved_addresses && cu != NULL)
	    uleb += cu->start;
	  uint8_t usize = *(uint8_t *) data++;
	  NEED (usize);
	  printf ("%*s[%4" PRIuMAX "] %s [%6" PRIxMAX "] ",
		  indent, "", (uintmax_t) offset, op_name, uleb);
	  print_block (usize, data);
	  data += usize;
	  CONSUME (data - start);
	  offset += 1 + (data - start);
	  break;

	case DW_OP_GNU_regval_type:
	  /* uleb128 register number, uleb128 CU relative
	     DW_TAG_base_type DIE offset.  */
	  start = data;
	  NEED (2);
	  get_uleb128 (uleb, data); /* XXX check overrun */
	  get_uleb128 (uleb2, data); /* XXX check overrun */
	  if (! print_unresolved_addresses && cu != NULL)
	    uleb2 += cu->start;
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIu64 " [%6" PRIx64 "]\n",
		  indent, "", (uintmax_t) offset, op_name, uleb, uleb2);
	  CONSUME (data - start);
	  offset += 1 + (data - start);
	  break;

	case DW_OP_GNU_deref_type:
	  /* 1-byte unsigned size of value, uleb128 CU relative
	     DW_TAG_base_type DIE offset.  */
	  start = data;
	  NEED (2);
	  usize = *(uint8_t *) data++;
	  get_uleb128 (uleb, data); /* XXX check overrun */
	  if (! print_unresolved_addresses && cu != NULL)
	    uleb += cu->start;
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIu8 " [%6" PRIxMAX "]\n",
		  indent, "", (uintmax_t) offset,
		  op_name, usize, uleb);
	  CONSUME (data - start);
	  offset += 1 + (data - start);
	  break;

	case DW_OP_GNU_convert:
	case DW_OP_GNU_reinterpret:
	  /* uleb128 CU relative offset to DW_TAG_base_type, or zero
	     for conversion to untyped.  */
	  start = data;
	  NEED (1);
	  get_uleb128 (uleb, data); /* XXX check overrun */
	  if (uleb != 0 && ! print_unresolved_addresses && cu != NULL)
	    uleb += cu->start;
	  printf ("%*s[%4" PRIuMAX "] %s [%6" PRIxMAX "]\n",
		  indent, "", (uintmax_t) offset, op_name, uleb);
	  CONSUME (data - start);
	  offset += 1 + (data - start);
	  break;

	case DW_OP_GNU_parameter_ref:
	  /* 4 byte CU relative reference to the abstract optimized away
	     DW_TAG_formal_parameter.  */
	  NEED (4);
	  uintmax_t param_off = (uintmax_t) read_4ubyte_unaligned (dbg, data);
	  if (! print_unresolved_addresses && cu != NULL)
	    param_off += cu->start;
	  printf ("%*s[%4" PRIuMAX "] %s [%6" PRIxMAX "]\n",
		  indent, "", (uintmax_t) offset, op_name, param_off);
	  CONSUME (4);
	  data += 4;
	  offset += 5;
	  break;

	default:
	  /* No Operand.  */
	  printf ("%*s[%4" PRIuMAX "] %s\n",
		  indent, "", (uintmax_t) offset, op_name);
	  ++offset;
	  break;
	}

      indent = indentrest;
      continue;

    invalid:
      printf (gettext ("%*s[%4" PRIuMAX "] %s  <TRUNCATED>\n"),
	      indent, "", (uintmax_t) offset, op_name);
      break;
    }
}


struct listptr
{
  Dwarf_Off offset:(64 - 3);
  bool addr64:1;
  bool dwarf64:1;
  bool warned:1;
  struct Dwarf_CU *cu;
};

#define listptr_offset_size(p)	((p)->dwarf64 ? 8 : 4)
#define listptr_address_size(p)	((p)->addr64 ? 8 : 4)

static Dwarf_Addr
listptr_base (struct listptr *p)
{
  Dwarf_Addr base;
  Dwarf_Die cu = CUDIE (p->cu);
  /* Find the base address of the compilation unit.  It will normally
     be specified by DW_AT_low_pc.  In DWARF-3 draft 4, the base
     address could be overridden by DW_AT_entry_pc.  It's been
     removed, but GCC emits DW_AT_entry_pc and not DW_AT_lowpc for
     compilation units with discontinuous ranges.  */
  if (unlikely (dwarf_lowpc (&cu, &base) != 0))
    {
      Dwarf_Attribute attr_mem;
      if (dwarf_formaddr (dwarf_attr (&cu, DW_AT_entry_pc, &attr_mem),
			  &base) != 0)
	base = 0;
    }
  return base;
}

static int
compare_listptr (const void *a, const void *b, void *arg)
{
  const char *name = arg;
  struct listptr *p1 = (void *) a;
  struct listptr *p2 = (void *) b;

  if (p1->offset < p2->offset)
    return -1;
  if (p1->offset > p2->offset)
    return 1;

  if (!p1->warned && !p2->warned)
    {
      if (p1->addr64 != p2->addr64)
	{
	  p1->warned = p2->warned = true;
	  error (0, 0,
		 gettext ("%s %#" PRIx64 " used with different address sizes"),
		 name, (uint64_t) p1->offset);
	}
      if (p1->dwarf64 != p2->dwarf64)
	{
	  p1->warned = p2->warned = true;
	  error (0, 0,
		 gettext ("%s %#" PRIx64 " used with different offset sizes"),
		 name, (uint64_t) p1->offset);
	}
      if (listptr_base (p1) != listptr_base (p2))
	{
	  p1->warned = p2->warned = true;
	  error (0, 0,
		 gettext ("%s %#" PRIx64 " used with different base addresses"),
		 name, (uint64_t) p1->offset);
	}
    }

  return 0;
}

struct listptr_table
{
  size_t n;
  size_t alloc;
  struct listptr *table;
};

static struct listptr_table known_loclistptr;
static struct listptr_table known_rangelistptr;

static void
reset_listptr (struct listptr_table *table)
{
  free (table->table);
  table->table = NULL;
  table->n = table->alloc = 0;
}

static void
notice_listptr (enum section_e section, struct listptr_table *table,
		uint_fast8_t address_size, uint_fast8_t offset_size,
		struct Dwarf_CU *cu, Dwarf_Off offset)
{
  if (print_debug_sections & section)
    {
      if (table->n == table->alloc)
	{
	  if (table->alloc == 0)
	    table->alloc = 128;
	  else
	    table->alloc *= 2;
	  table->table = xrealloc (table->table,
				   table->alloc * sizeof table->table[0]);
	}

      struct listptr *p = &table->table[table->n++];

      *p = (struct listptr)
	{
	  .addr64 = address_size == 8,
	  .dwarf64 = offset_size == 8,
	  .offset = offset,
	  .cu = cu
	};
      assert (p->offset == offset);
    }
}

static void
sort_listptr (struct listptr_table *table, const char *name)
{
  if (table->n > 0)
    qsort_r (table->table, table->n, sizeof table->table[0],
	     &compare_listptr, (void *) name);
}

static bool
skip_listptr_hole (struct listptr_table *table, size_t *idxp,
		   uint_fast8_t *address_sizep, uint_fast8_t *offset_sizep,
		   Dwarf_Addr *base, struct Dwarf_CU **cu, ptrdiff_t offset,
		   unsigned char **readp, unsigned char *endp)
{
  if (table->n == 0)
    return false;

  while (*idxp < table->n && table->table[*idxp].offset < (Dwarf_Off) offset)
    ++*idxp;

  struct listptr *p = &table->table[*idxp];

  if (*idxp == table->n
      || p->offset >= (Dwarf_Off) (endp - *readp + offset))
    {
      *readp = endp;
      printf (gettext (" [%6tx]  <UNUSED GARBAGE IN REST OF SECTION>\n"),
	      offset);
      return true;
    }

  if (p->offset != (Dwarf_Off) offset)
    {
      *readp += p->offset - offset;
      printf (gettext (" [%6tx]  <UNUSED GARBAGE> ... %" PRIu64 " bytes ...\n"),
	      offset, (Dwarf_Off) p->offset - offset);
      return true;
    }

  if (address_sizep != NULL)
    *address_sizep = listptr_address_size (p);
  if (offset_sizep != NULL)
    *offset_sizep = listptr_offset_size (p);
  if (base != NULL)
    *base = listptr_base (p);
  if (cu != NULL)
    *cu = p->cu;

  return false;
}


static void
print_debug_abbrev_section (Dwfl_Module *dwflmod __attribute__ ((unused)),
			    Ebl *ebl, GElf_Ehdr *ehdr,
			    Elf_Scn *scn, GElf_Shdr *shdr, Dwarf *dbg)
{
  const size_t sh_size = (dbg->sectiondata[IDX_debug_abbrev] ?
			  dbg->sectiondata[IDX_debug_abbrev]->d_size : 0);

  printf (gettext ("\nDWARF section [%2zu] '%s' at offset %#" PRIx64 ":\n"
		   " [ Code]\n"),
	  elf_ndxscn (scn), section_name (ebl, ehdr, shdr),
	  (uint64_t) shdr->sh_offset);

  Dwarf_Off offset = 0;
  while (offset < sh_size)
    {
      printf (gettext ("\nAbbreviation section at offset %" PRIu64 ":\n"),
	      offset);

      while (1)
	{
	  size_t length;
	  Dwarf_Abbrev abbrev;

	  int res = dwarf_offabbrev (dbg, offset, &length, &abbrev);
	  if (res != 0)
	    {
	      if (unlikely (res < 0))
		{
		  printf (gettext ("\
 *** error while reading abbreviation: %s\n"),
			  dwarf_errmsg (-1));
		  return;
		}

	      /* This is the NUL byte at the end of the section.  */
	      ++offset;
	      break;
	    }

	  /* We know these calls can never fail.  */
	  unsigned int code = dwarf_getabbrevcode (&abbrev);
	  unsigned int tag = dwarf_getabbrevtag (&abbrev);
	  int has_children = dwarf_abbrevhaschildren (&abbrev);

	  printf (gettext (" [%5u] offset: %" PRId64
			   ", children: %s, tag: %s\n"),
		  code, (int64_t) offset,
		  has_children ? gettext ("yes") : gettext ("no"),
		  dwarf_tag_name (tag));

	  size_t cnt = 0;
	  unsigned int name;
	  unsigned int form;
	  Dwarf_Off enoffset;
	  while (dwarf_getabbrevattr (&abbrev, cnt,
				      &name, &form, &enoffset) == 0)
	    {
	      printf ("          attr: %s, form: %s, offset: %#" PRIx64 "\n",
		      dwarf_attr_name (name), dwarf_form_name (form),
		      (uint64_t) enoffset);

	      ++cnt;
	    }

	  offset += length;
	}
    }
}


/* Print content of DWARF .debug_aranges section.  We fortunately do
   not have to know a bit about the structure of the section, libdwarf
   takes care of it.  */
static void
print_decoded_aranges_section (Ebl *ebl, GElf_Ehdr *ehdr, Elf_Scn *scn,
			       GElf_Shdr *shdr, Dwarf *dbg)
{
  Dwarf_Aranges *aranges;
  size_t cnt;
  if (unlikely (dwarf_getaranges (dbg, &aranges, &cnt) != 0))
    {
      error (0, 0, gettext ("cannot get .debug_aranges content: %s"),
	     dwarf_errmsg (-1));
      return;
    }

  printf (ngettext ("\
\nDWARF section [%2zu] '%s' at offset %#" PRIx64 " contains %zu entry:\n",
		    "\
\nDWARF section [%2zu] '%s' at offset %#" PRIx64 " contains %zu entries:\n",
		    cnt),
	  elf_ndxscn (scn), section_name (ebl, ehdr, shdr),
	  (uint64_t) shdr->sh_offset, cnt);

  /* Compute floor(log16(cnt)).  */
  size_t tmp = cnt;
  int digits = 1;
  while (tmp >= 16)
    {
      ++digits;
      tmp >>= 4;
    }

  for (size_t n = 0; n < cnt; ++n)
    {
      Dwarf_Arange *runp = dwarf_onearange (aranges, n);
      if (unlikely (runp == NULL))
	{
	  printf ("cannot get arange %zu: %s\n", n, dwarf_errmsg (-1));
	  return;
	}

      Dwarf_Addr start;
      Dwarf_Word length;
      Dwarf_Off offset;

      if (unlikely (dwarf_getarangeinfo (runp, &start, &length, &offset) != 0))
	printf (gettext (" [%*zu] ???\n"), digits, n);
      else
	printf (gettext (" [%*zu] start: %0#*" PRIx64
			 ", length: %5" PRIu64 ", CU DIE offset: %6"
			 PRId64 "\n"),
		digits, n, ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 10 : 18,
		(uint64_t) start, (uint64_t) length, (int64_t) offset);
    }
}


/* Print content of DWARF .debug_aranges section.  */
static void
print_debug_aranges_section (Dwfl_Module *dwflmod __attribute__ ((unused)),
			     Ebl *ebl, GElf_Ehdr *ehdr, Elf_Scn *scn,
			     GElf_Shdr *shdr, Dwarf *dbg)
{
  if (decodedaranges)
    {
      print_decoded_aranges_section (ebl, ehdr, scn, shdr, dbg);
      return;
    }

  Elf_Data *data = elf_rawdata (scn, NULL);

  if (unlikely (data == NULL))
    {
      error (0, 0, gettext ("cannot get .debug_aranges content: %s"),
	     elf_errmsg (-1));
      return;
    }

  printf (gettext ("\
\nDWARF section [%2zu] '%s' at offset %#" PRIx64 ":\n"),
	  elf_ndxscn (scn), section_name (ebl, ehdr, shdr),
	  (uint64_t) shdr->sh_offset);

  const unsigned char *readp = data->d_buf;
  const unsigned char *readendp = readp + data->d_size;

  while (readp < readendp)
    {
      const unsigned char *hdrstart = readp;
      size_t start_offset = hdrstart - (const unsigned char *) data->d_buf;

      printf (gettext ("\nTable at offset %Zu:\n"), start_offset);
      if (readp + 4 > readendp)
	{
	invalid_data:
	  error (0, 0, gettext ("invalid data in section [%zu] '%s'"),
		 elf_ndxscn (scn), section_name (ebl, ehdr, shdr));
	  return;
	}

      Dwarf_Word length = read_4ubyte_unaligned_inc (dbg, readp);
      unsigned int length_bytes = 4;
      if (length == DWARF3_LENGTH_64_BIT)
	{
	  if (readp + 8 > readendp)
	    goto invalid_data;
	  length = read_8ubyte_unaligned_inc (dbg, readp);
	  length_bytes = 8;
	}

      const unsigned char *nexthdr = readp + length;
      printf (gettext ("\n Length:        %6" PRIu64 "\n"),
	      (uint64_t) length);

      if (nexthdr > readendp)
	goto invalid_data;

      if (length == 0)
	continue;

      if (readp + 2 > readendp)
	goto invalid_data;
      uint_fast16_t version = read_2ubyte_unaligned_inc (dbg, readp);
      printf (gettext (" DWARF version: %6" PRIuFAST16 "\n"),
	      version);
      if (version != 2)
	{
	  error (0, 0, gettext ("unsupported aranges version"));
	  goto next_table;
	}

      Dwarf_Word offset;
      if (readp + length_bytes > readendp)
	goto invalid_data;
      if (length_bytes == 8)
	offset = read_8ubyte_unaligned_inc (dbg, readp);
      else
	offset = read_4ubyte_unaligned_inc (dbg, readp);
      printf (gettext (" CU offset:     %6" PRIx64 "\n"),
	      (uint64_t) offset);

      if (readp + 1 > readendp)
	goto invalid_data;
      unsigned int address_size = *readp++;
      printf (gettext (" Address size:  %6" PRIu64 "\n"),
	      (uint64_t) address_size);
      if (address_size != 4 && address_size != 8)
	{
	  error (0, 0, gettext ("unsupported address size"));
	  goto next_table;
	}

      unsigned int segment_size = *readp++;
      printf (gettext (" Segment size:  %6" PRIu64 "\n\n"),
	      (uint64_t) segment_size);
      if (segment_size != 0 && segment_size != 4 && segment_size != 8)
	{
	  error (0, 0, gettext ("unsupported segment size"));
	  goto next_table;
	}

      /* Round the address to the next multiple of 2*address_size.  */
      readp += ((2 * address_size - ((readp - hdrstart) % (2 * address_size)))
		% (2 * address_size));

      while (readp < nexthdr)
	{
	  Dwarf_Word range_address;
	  Dwarf_Word range_length;
	  Dwarf_Word segment = 0;
	  if (readp + 2 * address_size + segment_size > readendp)
	    goto invalid_data;
	  if (address_size == 4)
	    {
	      range_address = read_4ubyte_unaligned_inc (dbg, readp);
	      range_length = read_4ubyte_unaligned_inc (dbg, readp);
	    }
	  else
	    {
	      range_address = read_8ubyte_unaligned_inc (dbg, readp);
	      range_length = read_8ubyte_unaligned_inc (dbg, readp);
	    }

	  if (segment_size == 4)
	    segment = read_4ubyte_unaligned_inc (dbg, readp);
	  else if (segment_size == 8)
	    segment = read_8ubyte_unaligned_inc (dbg, readp);

	  if (range_address == 0 && range_length == 0 && segment == 0)
	    break;

	  char *b = format_dwarf_addr (dwflmod, address_size, range_address,
				       range_address);
	  char *e = format_dwarf_addr (dwflmod, address_size,
				       range_address + range_length - 1,
				       range_length);
	  if (segment_size != 0)
	    printf (gettext ("   %s..%s (%" PRIx64 ")\n"), b, e,
		    (uint64_t) segment);
	  else
	    printf (gettext ("   %s..%s\n"), b, e);
	  free (b);
	  free (e);
	}

    next_table:
      if (readp != nexthdr)
	{
	  size_t padding = nexthdr - readp;
	  printf (gettext ("   %Zu padding bytes\n"), padding);
	  readp = nexthdr;
	}
    }
}


/* Print content of DWARF .debug_ranges section.  */
static void
print_debug_ranges_section (Dwfl_Module *dwflmod,
			    Ebl *ebl, GElf_Ehdr *ehdr,
			    Elf_Scn *scn, GElf_Shdr *shdr,
			    Dwarf *dbg)
{
  Elf_Data *data = elf_rawdata (scn, NULL);

  if (unlikely (data == NULL))
    {
      error (0, 0, gettext ("cannot get .debug_ranges content: %s"),
	     elf_errmsg (-1));
      return;
    }

  printf (gettext ("\
\nDWARF section [%2zu] '%s' at offset %#" PRIx64 ":\n"),
	  elf_ndxscn (scn), section_name (ebl, ehdr, shdr),
	  (uint64_t) shdr->sh_offset);

  sort_listptr (&known_rangelistptr, "rangelistptr");
  size_t listptr_idx = 0;

  uint_fast8_t address_size = ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 4 : 8;

  bool first = true;
  Dwarf_Addr base = 0;
  unsigned char *const endp = (unsigned char *) data->d_buf + data->d_size;
  unsigned char *readp = data->d_buf;
  while (readp < endp)
    {
      ptrdiff_t offset = readp - (unsigned char *) data->d_buf;

      if (first && skip_listptr_hole (&known_rangelistptr, &listptr_idx,
				      &address_size, NULL, &base, NULL,
				      offset, &readp, endp))
	continue;

      if (unlikely (data->d_size - offset < (size_t) address_size * 2))
	{
	  printf (gettext (" [%6tx]  <INVALID DATA>\n"), offset);
	  break;
	}

      Dwarf_Addr begin;
      Dwarf_Addr end;
      if (address_size == 8)
	{
	  begin = read_8ubyte_unaligned_inc (dbg, readp);
	  end = read_8ubyte_unaligned_inc (dbg, readp);
	}
      else
	{
	  begin = read_4ubyte_unaligned_inc (dbg, readp);
	  end = read_4ubyte_unaligned_inc (dbg, readp);
	  if (begin == (Dwarf_Addr) (uint32_t) -1)
	    begin = (Dwarf_Addr) -1l;
	}

      if (begin == (Dwarf_Addr) -1l) /* Base address entry.  */
	{
	  char *b = format_dwarf_addr (dwflmod, address_size, end, end);
	  printf (gettext (" [%6tx]  base address %s\n"), offset, b);
	  free (b);
	  base = end;
	}
      else if (begin == 0 && end == 0) /* End of list entry.  */
	{
	  if (first)
	    printf (gettext (" [%6tx]  empty list\n"), offset);
	  first = true;
	}
      else
	{
	  char *b = format_dwarf_addr (dwflmod, address_size, base + begin,
				       begin);
	  char *e = format_dwarf_addr (dwflmod, address_size, base + end,
				       end);
	  /* We have an address range entry.  */
	  if (first)		/* First address range entry in a list.  */
	    printf (gettext (" [%6tx]  %s..%s\n"), offset, b, e);
	  else
	    printf (gettext ("           %s..%s\n"), b, e);
	  free (b);
	  free (e);

	  first = false;
	}
    }
}

#define REGNAMESZ 16
static const char *
register_info (Ebl *ebl, unsigned int regno, const Ebl_Register_Location *loc,
	       char name[REGNAMESZ], int *bits, int *type)
{
  const char *set;
  const char *pfx;
  int ignore;
  ssize_t n = ebl_register_info (ebl, regno, name, REGNAMESZ, &pfx, &set,
				 bits ?: &ignore, type ?: &ignore);
  if (n <= 0)
    {
      if (loc != NULL)
	snprintf (name, REGNAMESZ, "reg%u", loc->regno);
      else
	snprintf (name, REGNAMESZ, "??? 0x%x", regno);
      if (bits != NULL)
	*bits = loc != NULL ? loc->bits : 0;
      if (type != NULL)
	*type = DW_ATE_unsigned;
      set = "??? unrecognized";
    }
  else
    {
      if (bits != NULL && *bits <= 0)
	*bits = loc != NULL ? loc->bits : 0;
      if (type != NULL && *type == DW_ATE_void)
	*type = DW_ATE_unsigned;

    }
  return set;
}

static void
print_cfa_program (const unsigned char *readp, const unsigned char *const endp,
		   Dwarf_Word vma_base, unsigned int code_align,
		   int data_align,
		   unsigned int version, unsigned int ptr_size,
		   Dwfl_Module *dwflmod, Ebl *ebl, Dwarf *dbg)
{
  char regnamebuf[REGNAMESZ];
  const char *regname (unsigned int regno)
  {
    register_info (ebl, regno, NULL, regnamebuf, NULL, NULL);
    return regnamebuf;
  }

  puts ("\n   Program:");
  Dwarf_Word pc = vma_base;
  while (readp < endp)
    {
      unsigned int opcode = *readp++;

      if (opcode < DW_CFA_advance_loc)
	/* Extended opcode.  */
	switch (opcode)
	  {
	    uint64_t op1;
	    int64_t sop1;
	    uint64_t op2;
	    int64_t sop2;

	  case DW_CFA_nop:
	    puts ("     nop");
	    break;
	  case DW_CFA_set_loc:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    op1 += vma_base;
	    printf ("     set_loc %" PRIu64 "\n", op1 * code_align);
	    break;
	  case DW_CFA_advance_loc1:
	    printf ("     advance_loc1 %u to %#" PRIx64 "\n",
		    *readp, pc += *readp * code_align);
	    ++readp;
	    break;
	  case DW_CFA_advance_loc2:
	    op1 = read_2ubyte_unaligned_inc (dbg, readp);
	    printf ("     advance_loc2 %" PRIu64 " to %#" PRIx64 "\n",
		    op1, pc += op1 * code_align);
	    break;
	  case DW_CFA_advance_loc4:
	    op1 = read_4ubyte_unaligned_inc (dbg, readp);
	    printf ("     advance_loc4 %" PRIu64 " to %#" PRIx64 "\n",
		    op1, pc += op1 * code_align);
	    break;
	  case DW_CFA_offset_extended:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    get_uleb128 (op2, readp);
	    printf ("     offset_extended r%" PRIu64 " (%s) at cfa%+" PRId64
		    "\n",
		    op1, regname (op1), op2 * data_align);
	    break;
	  case DW_CFA_restore_extended:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    printf ("     restore_extended r%" PRIu64 " (%s)\n",
		    op1, regname (op1));
	    break;
	  case DW_CFA_undefined:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    printf ("     undefined r%" PRIu64 " (%s)\n", op1, regname (op1));
	    break;
	  case DW_CFA_same_value:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    printf ("     same_value r%" PRIu64 " (%s)\n", op1, regname (op1));
	    break;
	  case DW_CFA_register:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    get_uleb128 (op2, readp);
	    printf ("     register r%" PRIu64 " (%s) in r%" PRIu64 " (%s)\n",
		    op1, regname (op1), op2, regname (op2));
	    break;
	  case DW_CFA_remember_state:
	    puts ("     remember_state");
	    break;
	  case DW_CFA_restore_state:
	    puts ("     restore_state");
	    break;
	  case DW_CFA_def_cfa:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    get_uleb128 (op2, readp);
	    printf ("     def_cfa r%" PRIu64 " (%s) at offset %" PRIu64 "\n",
		    op1, regname (op1), op2);
	    break;
	  case DW_CFA_def_cfa_register:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    printf ("     def_cfa_register r%" PRIu64 " (%s)\n",
		    op1, regname (op1));
	    break;
	  case DW_CFA_def_cfa_offset:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    printf ("     def_cfa_offset %" PRIu64 "\n", op1);
	    break;
	  case DW_CFA_def_cfa_expression:
	    // XXX overflow check
	    get_uleb128 (op1, readp);	/* Length of DW_FORM_block.  */
	    printf ("     def_cfa_expression %" PRIu64 "\n", op1);
	    print_ops (dwflmod, dbg, 10, 10, version, ptr_size, 0, NULL,
		       op1, readp);
	    readp += op1;
	    break;
	  case DW_CFA_expression:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    get_uleb128 (op2, readp);	/* Length of DW_FORM_block.  */
	    printf ("     expression r%" PRIu64 " (%s) \n",
		    op1, regname (op1));
	    print_ops (dwflmod, dbg, 10, 10, version, ptr_size, 0, NULL,
		       op2, readp);
	    readp += op2;
	    break;
	  case DW_CFA_offset_extended_sf:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    get_sleb128 (sop2, readp);
	    printf ("     offset_extended_sf r%" PRIu64 " (%s) at cfa%+"
		    PRId64 "\n",
		    op1, regname (op1), sop2 * data_align);
	    break;
	  case DW_CFA_def_cfa_sf:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    get_sleb128 (sop2, readp);
	    printf ("     def_cfa_sf r%" PRIu64 " (%s) at offset %" PRId64 "\n",
		    op1, regname (op1), sop2 * data_align);
	    break;
	  case DW_CFA_def_cfa_offset_sf:
	    // XXX overflow check
	    get_sleb128 (sop1, readp);
	    printf ("     def_cfa_offset_sf %" PRId64 "\n", sop1 * data_align);
	    break;
	  case DW_CFA_val_offset:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    get_uleb128 (op2, readp);
	    printf ("     val_offset %" PRIu64 " at offset %" PRIu64 "\n",
		    op1, op2 * data_align);
	    break;
	  case DW_CFA_val_offset_sf:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    get_sleb128 (sop2, readp);
	    printf ("     val_offset_sf %" PRIu64 " at offset %" PRId64 "\n",
		    op1, sop2 * data_align);
	    break;
	  case DW_CFA_val_expression:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    get_uleb128 (op2, readp);	/* Length of DW_FORM_block.  */
	    printf ("     val_expression r%" PRIu64 " (%s)\n",
		    op1, regname (op1));
	    print_ops (dwflmod, dbg, 10, 10, version, ptr_size, 0,
		       NULL, op2, readp);
	    readp += op2;
	    break;
	  case DW_CFA_MIPS_advance_loc8:
	    op1 = read_8ubyte_unaligned_inc (dbg, readp);
	    printf ("     MIPS_advance_loc8 %" PRIu64 " to %#" PRIx64 "\n",
		    op1, pc += op1 * code_align);
	    break;
	  case DW_CFA_GNU_window_save:
	    puts ("     GNU_window_save");
	    break;
	  case DW_CFA_GNU_args_size:
	    // XXX overflow check
	    get_uleb128 (op1, readp);
	    printf ("     args_size %" PRIu64 "\n", op1);
	    break;
	  default:
	    printf ("     ??? (%u)\n", opcode);
	    break;
	  }
      else if (opcode < DW_CFA_offset)
	printf ("     advance_loc %u to %#" PRIx64 "\n",
		opcode & 0x3f, pc += (opcode & 0x3f) * code_align);
      else if (opcode < DW_CFA_restore)
	{
	  uint64_t offset;
	  // XXX overflow check
	  get_uleb128 (offset, readp);
	  printf ("     offset r%u (%s) at cfa%+" PRId64 "\n",
		  opcode & 0x3f, regname (opcode & 0x3f), offset * data_align);
	}
      else
	printf ("     restore r%u (%s)\n",
		opcode & 0x3f, regname (opcode & 0x3f));
    }
}


static unsigned int
encoded_ptr_size (int encoding, unsigned int ptr_size)
{
  switch (encoding & 7)
    {
    case 2:
      return 2;
    case 3:
      return 4;
    case 4:
      return 8;
    default:
      return ptr_size;
    }
}


static unsigned int
print_encoding (unsigned int val)
{
  switch (val & 0xf)
    {
    case DW_EH_PE_absptr:
      fputs ("absptr", stdout);
      break;
    case DW_EH_PE_uleb128:
      fputs ("uleb128", stdout);
      break;
    case DW_EH_PE_udata2:
      fputs ("udata2", stdout);
      break;
    case DW_EH_PE_udata4:
      fputs ("udata4", stdout);
      break;
    case DW_EH_PE_udata8:
      fputs ("udata8", stdout);
      break;
    case DW_EH_PE_sleb128:
      fputs ("sleb128", stdout);
      break;
    case DW_EH_PE_sdata2:
      fputs ("sdata2", stdout);
      break;
    case DW_EH_PE_sdata4:
      fputs ("sdata4", stdout);
      break;
    case DW_EH_PE_sdata8:
      fputs ("sdata8", stdout);
      break;
    default:
      /* We did not use any of the bits after all.  */
      return val;
    }

  return val & ~0xf;
}


static unsigned int
print_relinfo (unsigned int val)
{
  switch (val & 0x70)
    {
    case DW_EH_PE_pcrel:
      fputs ("pcrel", stdout);
      break;
    case DW_EH_PE_textrel:
      fputs ("textrel", stdout);
      break;
    case DW_EH_PE_datarel:
      fputs ("datarel", stdout);
      break;
    case DW_EH_PE_funcrel:
      fputs ("funcrel", stdout);
      break;
    case DW_EH_PE_aligned:
      fputs ("aligned", stdout);
      break;
    default:
      return val;
    }

  return val & ~0x70;
}


static void
print_encoding_base (const char *pfx, unsigned int fde_encoding)
{
  printf ("(%s", pfx);

  if (fde_encoding == DW_EH_PE_omit)
    puts ("omit)");
  else
    {
      unsigned int w = fde_encoding;

      w = print_encoding (w);

      if (w & 0x70)
	{
	  if (w != fde_encoding)
	    fputc_unlocked (' ', stdout);

	  w = print_relinfo (w);
	}

      if (w != 0)
	printf ("%s%x", w != fde_encoding ? " " : "", w);

      puts (")");
    }
}


static const unsigned char *
read_encoded (unsigned int encoding, const unsigned char *readp,
	      const unsigned char *const endp, uint64_t *res, Dwarf *dbg)
{
  if ((encoding & 0xf) == DW_EH_PE_absptr)
    encoding = gelf_getclass (dbg->elf) == ELFCLASS32
      ? DW_EH_PE_udata4 : DW_EH_PE_udata8;

  switch (encoding & 0xf)
    {
    case DW_EH_PE_uleb128:
      // XXX buffer overrun check
      get_uleb128 (*res, readp);
      break;
    case DW_EH_PE_sleb128:
      // XXX buffer overrun check
      get_sleb128 (*res, readp);
      break;
    case DW_EH_PE_udata2:
      if (readp + 2 > endp)
	goto invalid;
      *res = read_2ubyte_unaligned_inc (dbg, readp);
      break;
    case DW_EH_PE_udata4:
      if (readp + 4 > endp)
	goto invalid;
      *res = read_4ubyte_unaligned_inc (dbg, readp);
      break;
    case DW_EH_PE_udata8:
      if (readp + 8 > endp)
	goto invalid;
      *res = read_8ubyte_unaligned_inc (dbg, readp);
      break;
    case DW_EH_PE_sdata2:
      if (readp + 2 > endp)
	goto invalid;
      *res = read_2sbyte_unaligned_inc (dbg, readp);
      break;
    case DW_EH_PE_sdata4:
      if (readp + 4 > endp)
	goto invalid;
      *res = read_4sbyte_unaligned_inc (dbg, readp);
      break;
    case DW_EH_PE_sdata8:
      if (readp + 8 > endp)
	goto invalid;
      *res = read_8sbyte_unaligned_inc (dbg, readp);
      break;
    default:
    invalid:
      error (1, 0,
	     gettext ("invalid encoding"));
    }

  return readp;
}


static void
print_debug_frame_section (Dwfl_Module *dwflmod, Ebl *ebl, GElf_Ehdr *ehdr,
			   Elf_Scn *scn, GElf_Shdr *shdr, Dwarf *dbg)
{
  size_t shstrndx;
  /* We know this call will succeed since it did in the caller.  */
  (void) elf_getshdrstrndx (ebl->elf, &shstrndx);
  const char *scnname = elf_strptr (ebl->elf, shstrndx, shdr->sh_name);

  /* Needed if we find PC-relative addresses.  */
  GElf_Addr bias;
  if (dwfl_module_getelf (dwflmod, &bias) == NULL)
    {
      error (0, 0, gettext ("cannot get ELF: %s"), dwfl_errmsg (-1));
      return;
    }

  Elf_Data *data = elf_rawdata (scn, NULL);

  if (unlikely (data == NULL))
    {
      error (0, 0, gettext ("cannot get %s content: %s"),
	     scnname, elf_errmsg (-1));
      return;
    }
  bool is_eh_frame = strcmp (scnname, ".eh_frame") == 0;

  if (is_eh_frame)
    printf (gettext ("\
\nCall frame information section [%2zu] '%s' at offset %#" PRIx64 ":\n"),
	    elf_ndxscn (scn), scnname, (uint64_t) shdr->sh_offset);
  else
    printf (gettext ("\
\nDWARF section [%2zu] '%s' at offset %#" PRIx64 ":\n"),
	    elf_ndxscn (scn), scnname, (uint64_t) shdr->sh_offset);

  struct cieinfo
  {
    ptrdiff_t cie_offset;
    const char *augmentation;
    unsigned int code_alignment_factor;
    unsigned int data_alignment_factor;
    uint8_t address_size;
    uint8_t fde_encoding;
    uint8_t lsda_encoding;
    struct cieinfo *next;
  } *cies = NULL;

  const unsigned char *readp = data->d_buf;
  const unsigned char *const dataend = ((unsigned char *) data->d_buf
					+ data->d_size);
  while (readp < dataend)
    {
      if (unlikely (readp + 4 > dataend))
	{
	invalid_data:
	  error (0, 0, gettext ("invalid data in section [%zu] '%s'"),
		     elf_ndxscn (scn), scnname);
	      return;
	}

      /* At the beginning there must be a CIE.  There can be multiple,
	 hence we test tis in a loop.  */
      ptrdiff_t offset = readp - (unsigned char *) data->d_buf;

      Dwarf_Word unit_length = read_4ubyte_unaligned_inc (dbg, readp);
      unsigned int length = 4;
      if (unlikely (unit_length == 0xffffffff))
	{
	  if (unlikely (readp + 8 > dataend))
	    goto invalid_data;

	  unit_length = read_8ubyte_unaligned_inc (dbg, readp);
	  length = 8;
	}

      if (unlikely (unit_length == 0))
	{
	  printf (gettext ("\n [%6tx] Zero terminator\n"), offset);
	  continue;
	}

      unsigned int ptr_size = ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 4 : 8;

      ptrdiff_t start = readp - (unsigned char *) data->d_buf;
      const unsigned char *const cieend = readp + unit_length;
      if (unlikely (cieend > dataend || readp + 8 > dataend))
	goto invalid_data;

      Dwarf_Off cie_id;
      if (length == 4)
	{
	  cie_id = read_4ubyte_unaligned_inc (dbg, readp);
	  if (!is_eh_frame && cie_id == DW_CIE_ID_32)
	    cie_id = DW_CIE_ID_64;
	}
      else
	cie_id = read_8ubyte_unaligned_inc (dbg, readp);

      uint_fast8_t version = 2;
      unsigned int code_alignment_factor;
      int data_alignment_factor;
      unsigned int fde_encoding = 0;
      unsigned int lsda_encoding = 0;
      Dwarf_Word initial_location = 0;
      Dwarf_Word vma_base = 0;

      if (cie_id == (is_eh_frame ? 0 : DW_CIE_ID_64))
	{
	  version = *readp++;
	  const char *const augmentation = (const char *) readp;
	  readp = memchr (readp, '\0', cieend - readp);
	  if (unlikely (readp == NULL))
	    goto invalid_data;
	  ++readp;

	  uint_fast8_t segment_size = 0;
	  if (version >= 4)
	    {
	      if (cieend - readp < 5)
		goto invalid_data;
	      ptr_size = *readp++;
	      segment_size = *readp++;
	    }

	  // XXX Check overflow
	  get_uleb128 (code_alignment_factor, readp);
	  // XXX Check overflow
	  get_sleb128 (data_alignment_factor, readp);

	  /* In some variant for unwind data there is another field.  */
	  if (strcmp (augmentation, "eh") == 0)
	    readp += ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 4 : 8;

	  unsigned int return_address_register;
	  if (unlikely (version == 1))
	    return_address_register = *readp++;
	  else
	    // XXX Check overflow
	    get_uleb128 (return_address_register, readp);

	  printf ("\n [%6tx] CIE length=%" PRIu64 "\n"
		  "   CIE_id:                   %" PRIu64 "\n"
		  "   version:                  %u\n"
		  "   augmentation:             \"%s\"\n",
		  offset, (uint64_t) unit_length, (uint64_t) cie_id,
		  version, augmentation);
	  if (version >= 4)
	    printf ("   address_size:             %u\n"
		    "   segment_size:             %u\n",
		    ptr_size, segment_size);
	  printf ("   code_alignment_factor:    %u\n"
		  "   data_alignment_factor:    %d\n"
		  "   return_address_register:  %u\n",
		  code_alignment_factor,
		  data_alignment_factor, return_address_register);

	  if (augmentation[0] == 'z')
	    {
	      unsigned int augmentationlen;
	      get_uleb128 (augmentationlen, readp);

	      if (augmentationlen > (size_t) (dataend - readp))
		error (1, 0, gettext ("invalid augmentation length"));

	      const char *hdr = "Augmentation data:";
	      const char *cp = augmentation + 1;
	      while (*cp != '\0')
		{
		  printf ("   %-26s%#x ", hdr, *readp);
		  hdr = "";

		  if (*cp == 'R')
		    {
		      fde_encoding = *readp++;
		      print_encoding_base (gettext ("FDE address encoding: "),
					   fde_encoding);
		    }
		  else if (*cp == 'L')
		    {
		      lsda_encoding = *readp++;
		      print_encoding_base (gettext ("LSDA pointer encoding: "),
					   lsda_encoding);
		    }
		  else if (*cp == 'P')
		    {
		      /* Personality.  This field usually has a relocation
			 attached pointing to __gcc_personality_v0.  */
		      const unsigned char *startp = readp;
		      unsigned int encoding = *readp++;
		      uint64_t val = 0;
		      readp = read_encoded (encoding, readp,
					    readp - 1 + augmentationlen,
					    &val, dbg);

		      while (++startp < readp)
			printf ("%#x ", *startp);

		      putchar ('(');
		      print_encoding (encoding);
		      putchar (' ');
		      switch (encoding & 0xf)
			{
			case DW_EH_PE_sleb128:
			case DW_EH_PE_sdata2:
			case DW_EH_PE_sdata4:
			  printf ("%" PRId64 ")\n", val);
			  break;
			default:
			  printf ("%#" PRIx64 ")\n", val);
			  break;
			}
		    }
		  else
		    printf ("(%x)\n", *readp++);

		  ++cp;
		}
	    }

	  if (likely (ptr_size == 4 || ptr_size == 8))
	    {
	      struct cieinfo *newp = alloca (sizeof (*newp));
	      newp->cie_offset = offset;
	      newp->augmentation = augmentation;
	      newp->fde_encoding = fde_encoding;
	      newp->lsda_encoding = lsda_encoding;
	      newp->address_size = ptr_size;
	      newp->code_alignment_factor = code_alignment_factor;
	      newp->data_alignment_factor = data_alignment_factor;
	      newp->next = cies;
	      cies = newp;
	    }
	}
      else
	{
	  struct cieinfo *cie = cies;
	  while (cie != NULL)
	    if (is_eh_frame
		? start - (ptrdiff_t) cie_id == cie->cie_offset
		: (ptrdiff_t) cie_id == cie->cie_offset)
	      break;
	    else
	      cie = cie->next;
	  if (unlikely (cie == NULL))
	    {
	      puts ("invalid CIE reference in FDE");
	      return;
	    }

	  /* Initialize from CIE data.  */
	  fde_encoding = cie->fde_encoding;
	  lsda_encoding = cie->lsda_encoding;
	  ptr_size = encoded_ptr_size (fde_encoding, cie->address_size);
	  code_alignment_factor = cie->code_alignment_factor;
	  data_alignment_factor = cie->data_alignment_factor;

	  const unsigned char *base = readp;
	  // XXX There are sometimes relocations for this value
	  initial_location = read_ubyte_unaligned_inc (ptr_size, dbg, readp);
	  Dwarf_Word address_range
	    = read_ubyte_unaligned_inc (ptr_size, dbg, readp);

	  /* pcrel for an FDE address is relative to the runtime
	     address of the start_address field itself.  Sign extend
	     if necessary to make sure the calculation is done on the
	     full 64 bit address even when initial_location only holds
	     the lower 32 bits.  */
	  Dwarf_Addr pc_start = initial_location;
	  if (ptr_size == 4)
	    pc_start = (uint64_t) (int32_t) pc_start;
	  if ((fde_encoding & 0x70) == DW_EH_PE_pcrel)
	    pc_start += ((uint64_t) shdr->sh_addr
			 + (base - (const unsigned char *) data->d_buf)
			 - bias);

	  char *a = format_dwarf_addr (dwflmod, cie->address_size,
				       pc_start, initial_location);
	  printf ("\n [%6tx] FDE length=%" PRIu64 " cie=[%6tx]\n"
		  "   CIE_pointer:              %" PRIu64 "\n"
		  "   initial_location:         %s",
		  offset, (uint64_t) unit_length,
		  cie->cie_offset, (uint64_t) cie_id, a);
	  free (a);
	  if ((fde_encoding & 0x70) == DW_EH_PE_pcrel)
	    {
	      vma_base = (((uint64_t) shdr->sh_offset
			   + (base - (const unsigned char *) data->d_buf)
			   + (uint64_t) initial_location)
			  & (ptr_size == 4
			     ? UINT64_C (0xffffffff)
			     : UINT64_C (0xffffffffffffffff)));
	      printf (gettext (" (offset: %#" PRIx64 ")"),
		      (uint64_t) vma_base);
	    }

	  printf ("\n   address_range:            %#" PRIx64,
		  (uint64_t) address_range);
	  if ((fde_encoding & 0x70) == DW_EH_PE_pcrel)
	    printf (gettext (" (end offset: %#" PRIx64 ")"),
		    ((uint64_t) vma_base + (uint64_t) address_range)
		    & (ptr_size == 4
		       ? UINT64_C (0xffffffff)
		       : UINT64_C (0xffffffffffffffff)));
	  putchar ('\n');

	  if (cie->augmentation[0] == 'z')
	    {
	      unsigned int augmentationlen;
	      get_uleb128 (augmentationlen, readp);

	      if (augmentationlen > 0)
		{
		  const char *hdr = "Augmentation data:";
		  const char *cp = cie->augmentation + 1;
		  unsigned int u = 0;
		  while (*cp != '\0')
		    {
		      if (*cp == 'L')
			{
			  uint64_t lsda_pointer;
			  const unsigned char *p
			    = read_encoded (lsda_encoding, &readp[u],
					    &readp[augmentationlen],
					    &lsda_pointer, dbg);
			  u = p - readp;
			  printf (gettext ("\
   %-26sLSDA pointer: %#" PRIx64 "\n"),
				  hdr, lsda_pointer);
			  hdr = "";
			}
		      ++cp;
		    }

		  while (u < augmentationlen)
		    {
		      printf ("   %-26s%#x\n", hdr, readp[u++]);
		      hdr = "";
		    }
		}

	      readp += augmentationlen;
	    }
	}

      /* Handle the initialization instructions.  */
      print_cfa_program (readp, cieend, vma_base, code_alignment_factor,
			 data_alignment_factor, version, ptr_size,
			 dwflmod, ebl, dbg);
      readp = cieend;
    }
}


struct attrcb_args
{
  Dwfl_Module *dwflmod;
  Dwarf *dbg;
  Dwarf_Die *die;
  int level;
  bool silent;
  unsigned int version;
  unsigned int addrsize;
  unsigned int offset_size;
  struct Dwarf_CU *cu;
};


static int
attr_callback (Dwarf_Attribute *attrp, void *arg)
{
  struct attrcb_args *cbargs = (struct attrcb_args *) arg;
  const int level = cbargs->level;

  unsigned int attr = dwarf_whatattr (attrp);
  if (unlikely (attr == 0))
    {
      if (!cbargs->silent)
	error (0, 0, gettext ("cannot get attribute code: %s"),
	       dwarf_errmsg (-1));
      return DWARF_CB_ABORT;
    }

  unsigned int form = dwarf_whatform (attrp);
  if (unlikely (form == 0))
    {
      if (!cbargs->silent)
	error (0, 0, gettext ("cannot get attribute form: %s"),
	       dwarf_errmsg (-1));
      return DWARF_CB_ABORT;
    }

  switch (form)
    {
    case DW_FORM_addr:
      if (!cbargs->silent)
	{
	  Dwarf_Addr addr;
	  if (unlikely (dwarf_formaddr (attrp, &addr) != 0))
	    {
	    attrval_out:
	      if (!cbargs->silent)
		error (0, 0, gettext ("cannot get attribute value: %s"),
		       dwarf_errmsg (-1));
	      return DWARF_CB_ABORT;
	    }
	  char *a = format_dwarf_addr (cbargs->dwflmod, cbargs->addrsize,
				       addr, addr);
	  printf ("           %*s%-20s (%s) %s\n",
		  (int) (level * 2), "", dwarf_attr_name (attr),
		  dwarf_form_name (form), a);
	  free (a);
	}
      break;

    case DW_FORM_indirect:
    case DW_FORM_strp:
    case DW_FORM_string:
    case DW_FORM_GNU_strp_alt:
      if (cbargs->silent)
	break;
      const char *str = dwarf_formstring (attrp);
      if (unlikely (str == NULL))
	goto attrval_out;
      printf ("           %*s%-20s (%s) \"%s\"\n",
	      (int) (level * 2), "", dwarf_attr_name (attr),
	      dwarf_form_name (form), str);
      break;

    case DW_FORM_ref_addr:
    case DW_FORM_ref_udata:
    case DW_FORM_ref8:
    case DW_FORM_ref4:
    case DW_FORM_ref2:
    case DW_FORM_ref1:
    case DW_FORM_GNU_ref_alt:
      if (cbargs->silent)
	break;
      Dwarf_Die ref;
      if (unlikely (dwarf_formref_die (attrp, &ref) == NULL))
	goto attrval_out;

      printf ("           %*s%-20s (%s) [%6" PRIxMAX "]\n",
	      (int) (level * 2), "", dwarf_attr_name (attr),
	      dwarf_form_name (form), (uintmax_t) dwarf_dieoffset (&ref));
      break;

    case DW_FORM_ref_sig8:
      if (cbargs->silent)
	break;
      printf ("           %*s%-20s (%s) {%6" PRIx64 "}\n",
	      (int) (level * 2), "", dwarf_attr_name (attr),
	      dwarf_form_name (form),
	      (uint64_t) read_8ubyte_unaligned (attrp->cu->dbg, attrp->valp));
      break;

    case DW_FORM_sec_offset:
    case DW_FORM_udata:
    case DW_FORM_sdata:
    case DW_FORM_data8:
    case DW_FORM_data4:
    case DW_FORM_data2:
    case DW_FORM_data1:;
      Dwarf_Word num;
      if (unlikely (dwarf_formudata (attrp, &num) != 0))
	goto attrval_out;

      const char *valuestr = NULL;
      switch (attr)
	{
	  /* This case can take either a constant or a loclistptr.  */
	case DW_AT_data_member_location:
	  if (form != DW_FORM_sec_offset
	      && (cbargs->version >= 4
		  || (form != DW_FORM_data4 && form != DW_FORM_data8)))
	    {
	      if (!cbargs->silent)
		printf ("           %*s%-20s (%s) %" PRIxMAX "\n",
			(int) (level * 2), "", dwarf_attr_name (attr),
			dwarf_form_name (form), (uintmax_t) num);
	      return DWARF_CB_OK;
	    }
	  /* else fallthrough */

	/* These cases always take a loclistptr and no constant. */
	case DW_AT_location:
	case DW_AT_data_location:
	case DW_AT_vtable_elem_location:
	case DW_AT_string_length:
	case DW_AT_use_location:
	case DW_AT_frame_base:
	case DW_AT_return_addr:
	case DW_AT_static_link:
	case DW_AT_GNU_call_site_value:
	case DW_AT_GNU_call_site_data_value:
	case DW_AT_GNU_call_site_target:
	case DW_AT_GNU_call_site_target_clobbered:
	  notice_listptr (section_loc, &known_loclistptr,
			  cbargs->addrsize, cbargs->offset_size,
			  cbargs->cu, num);
	  if (!cbargs->silent)
	    printf ("           %*s%-20s (%s) location list [%6" PRIxMAX "]\n",
		    (int) (level * 2), "", dwarf_attr_name (attr),
		    dwarf_form_name (form), (uintmax_t) num);
	  return DWARF_CB_OK;

	case DW_AT_ranges:
	  notice_listptr (section_ranges, &known_rangelistptr,
			  cbargs->addrsize, cbargs->offset_size,
			  cbargs->cu, num);
	  if (!cbargs->silent)
	    printf ("           %*s%-20s (%s) range list [%6" PRIxMAX "]\n",
		    (int) (level * 2), "", dwarf_attr_name (attr),
		    dwarf_form_name (form), (uintmax_t) num);
	  return DWARF_CB_OK;

	case DW_AT_language:
	  valuestr = dwarf_lang_name (num);
	  break;
	case DW_AT_encoding:
	  valuestr = dwarf_encoding_name (num);
	  break;
	case DW_AT_accessibility:
	  valuestr = dwarf_access_name (num);
	  break;
	case DW_AT_visibility:
	  valuestr = dwarf_visibility_name (num);
	  break;
	case DW_AT_virtuality:
	  valuestr = dwarf_virtuality_name (num);
	  break;
	case DW_AT_identifier_case:
	  valuestr = dwarf_identifier_case_name (num);
	  break;
	case DW_AT_calling_convention:
	  valuestr = dwarf_calling_convention_name (num);
	  break;
	case DW_AT_inline:
	  valuestr = dwarf_inline_name (num);
	  break;
	case DW_AT_ordering:
	  valuestr = dwarf_ordering_name (num);
	  break;
	case DW_AT_discr_list:
	  valuestr = dwarf_discr_list_name (num);
	  break;
	default:
	  /* Nothing.  */
	  break;
	}

      if (cbargs->silent)
	break;

      /* When highpc is in constant form it is relative to lowpc.
	 In that case also show the address.  */
      Dwarf_Addr highpc;
      if (attr == DW_AT_high_pc && dwarf_highpc (cbargs->die, &highpc) == 0)
	{
	  char *a = format_dwarf_addr (cbargs->dwflmod, cbargs->addrsize,
				       highpc, highpc);
	  printf ("           %*s%-20s (%s) %" PRIuMAX " (%s)\n",
		  (int) (level * 2), "", dwarf_attr_name (attr),
		  dwarf_form_name (form), (uintmax_t) num, a);
	  free (a);
	}
      else if (valuestr == NULL)
	printf ("           %*s%-20s (%s) %" PRIuMAX "\n",
		(int) (level * 2), "", dwarf_attr_name (attr),
		dwarf_form_name (form), (uintmax_t) num);
      else
	printf ("           %*s%-20s (%s) %s (%" PRIuMAX ")\n",
		(int) (level * 2), "", dwarf_attr_name (attr),
		dwarf_form_name (form), valuestr, (uintmax_t) num);
      break;

    case DW_FORM_flag:
      if (cbargs->silent)
	break;
      bool flag;
      if (unlikely (dwarf_formflag (attrp, &flag) != 0))
	goto attrval_out;

      printf ("           %*s%-20s (%s) %s\n",
	      (int) (level * 2), "", dwarf_attr_name (attr),
	      dwarf_form_name (form), nl_langinfo (flag ? YESSTR : NOSTR));
      break;

    case DW_FORM_flag_present:
      if (cbargs->silent)
	break;
      printf ("           %*s%-20s (%s) %s\n",
	      (int) (level * 2), "", dwarf_attr_name (attr),
	      dwarf_form_name (form), nl_langinfo (YESSTR));
      break;

    case DW_FORM_exprloc:
    case DW_FORM_block4:
    case DW_FORM_block2:
    case DW_FORM_block1:
    case DW_FORM_block:
      if (cbargs->silent)
	break;
      Dwarf_Block block;
      if (unlikely (dwarf_formblock (attrp, &block) != 0))
	goto attrval_out;

      printf ("           %*s%-20s (%s) ",
	      (int) (level * 2), "", dwarf_attr_name (attr),
	      dwarf_form_name (form));

      switch (attr)
	{
	default:
	  if (form != DW_FORM_exprloc)
	    {
	      print_block (block.length, block.data);
	      break;
	    }
	  /* Fall through.  */

	case DW_AT_location:
	case DW_AT_data_location:
	case DW_AT_data_member_location:
	case DW_AT_vtable_elem_location:
	case DW_AT_string_length:
	case DW_AT_use_location:
	case DW_AT_frame_base:
	case DW_AT_return_addr:
	case DW_AT_static_link:
	case DW_AT_allocated:
	case DW_AT_associated:
	case DW_AT_bit_size:
	case DW_AT_bit_offset:
	case DW_AT_bit_stride:
	case DW_AT_byte_size:
	case DW_AT_byte_stride:
	case DW_AT_count:
	case DW_AT_lower_bound:
	case DW_AT_upper_bound:
	case DW_AT_GNU_call_site_value:
	case DW_AT_GNU_call_site_data_value:
	case DW_AT_GNU_call_site_target:
	case DW_AT_GNU_call_site_target_clobbered:
	  putchar ('\n');
	  print_ops (cbargs->dwflmod, cbargs->dbg,
		     12 + level * 2, 12 + level * 2,
		     cbargs->version, cbargs->addrsize, cbargs->offset_size,
		     attrp->cu, block.length, block.data);
	  break;
	}
      break;

    default:
      if (cbargs->silent)
	break;
      printf ("           %*s%-20s (form: %#x) ???\n",
	      (int) (level * 2), "", dwarf_attr_name (attr),
	      (int) form);
      break;
    }

  return DWARF_CB_OK;
}

static void
print_debug_units (Dwfl_Module *dwflmod,
		   Ebl *ebl, GElf_Ehdr *ehdr,
		   Elf_Scn *scn, GElf_Shdr *shdr,
		   Dwarf *dbg, bool debug_types)
{
  const bool silent = !(print_debug_sections & section_info);
  const char *secname = section_name (ebl, ehdr, shdr);

  if (!silent)
    printf (gettext ("\
\nDWARF section [%2zu] '%s' at offset %#" PRIx64 ":\n [Offset]\n"),
	    elf_ndxscn (scn), secname, (uint64_t) shdr->sh_offset);

  /* If the section is empty we don't have to do anything.  */
  if (!silent && shdr->sh_size == 0)
    return;

  int maxdies = 20;
  Dwarf_Die *dies = (Dwarf_Die *) xmalloc (maxdies * sizeof (Dwarf_Die));

  Dwarf_Off offset = 0;

  /* New compilation unit.  */
  size_t cuhl;
  Dwarf_Half version;
  Dwarf_Off abbroffset;
  uint8_t addrsize;
  uint8_t offsize;
  Dwarf_Off nextcu;
  uint64_t typesig;
  Dwarf_Off typeoff;
 next_cu:
  if (dwarf_next_unit (dbg, offset, &nextcu, &cuhl, &version,
		       &abbroffset, &addrsize, &offsize,
		       debug_types ? &typesig : NULL,
		       debug_types ? &typeoff : NULL) != 0)
    goto do_return;

  if (!silent)
    {
      if (debug_types)
	printf (gettext (" Type unit at offset %" PRIu64 ":\n"
			 " Version: %" PRIu16 ", Abbreviation section offset: %"
			 PRIu64 ", Address size: %" PRIu8
			 ", Offset size: %" PRIu8
			 "\n Type signature: %#" PRIx64
			 ", Type offset: %#" PRIx64 "\n"),
		(uint64_t) offset, version, abbroffset, addrsize, offsize,
		typesig, (uint64_t) typeoff);
      else
	printf (gettext (" Compilation unit at offset %" PRIu64 ":\n"
			 " Version: %" PRIu16 ", Abbreviation section offset: %"
			 PRIu64 ", Address size: %" PRIu8
			 ", Offset size: %" PRIu8 "\n"),
		(uint64_t) offset, version, abbroffset, addrsize, offsize);
    }

  struct attrcb_args args =
    {
      .dwflmod = dwflmod,
      .dbg = dbg,
      .silent = silent,
      .version = version,
      .addrsize = addrsize,
      .offset_size = offsize
    };

  offset += cuhl;

  int level = 0;

  if (unlikely ((debug_types ? dwarf_offdie_types : dwarf_offdie)
		(dbg, offset, &dies[level]) == NULL))
    {
      if (!silent)
	error (0, 0, gettext ("cannot get DIE at offset %" PRIu64
			      " in section '%s': %s"),
	       (uint64_t) offset, secname, dwarf_errmsg (-1));
      goto do_return;
    }

  args.cu = dies[0].cu;

  do
    {
      offset = dwarf_dieoffset (&dies[level]);
      if (unlikely (offset == ~0ul))
	{
	  if (!silent)
	    error (0, 0, gettext ("cannot get DIE offset: %s"),
		   dwarf_errmsg (-1));
	  goto do_return;
	}

      int tag = dwarf_tag (&dies[level]);
      if (unlikely (tag == DW_TAG_invalid))
	{
	  if (!silent)
	    error (0, 0, gettext ("cannot get tag of DIE at offset %" PRIu64
				  " in section '%s': %s"),
		   (uint64_t) offset, secname, dwarf_errmsg (-1));
	  goto do_return;
	}

      if (!silent)
	printf (" [%6" PRIx64 "]  %*s%s\n",
		(uint64_t) offset, (int) (level * 2), "",
		dwarf_tag_name (tag));

      /* Print the attribute values.  */
      args.level = level;
      args.die = &dies[level];
      (void) dwarf_getattrs (&dies[level], attr_callback, &args, 0);

      /* Make room for the next level's DIE.  */
      if (level + 1 == maxdies)
	dies = (Dwarf_Die *) xrealloc (dies,
				       (maxdies += 10)
				       * sizeof (Dwarf_Die));

      int res = dwarf_child (&dies[level], &dies[level + 1]);
      if (res > 0)
	{
	  while ((res = dwarf_siblingof (&dies[level], &dies[level])) == 1)
	    if (level-- == 0)
	      break;

	  if (unlikely (res == -1))
	    {
	      if (!silent)
		error (0, 0, gettext ("cannot get next DIE: %s\n"),
		       dwarf_errmsg (-1));
	      goto do_return;
	    }
	}
      else if (unlikely (res < 0))
	{
	  if (!silent)
	    error (0, 0, gettext ("cannot get next DIE: %s"),
		   dwarf_errmsg (-1));
	  goto do_return;
	}
      else
	++level;
    }
  while (level >= 0);

  offset = nextcu;
  if (offset != 0)
     goto next_cu;

 do_return:
  free (dies);
}

static void
print_debug_info_section (Dwfl_Module *dwflmod, Ebl *ebl, GElf_Ehdr *ehdr,
			  Elf_Scn *scn, GElf_Shdr *shdr, Dwarf *dbg)
{
  print_debug_units (dwflmod, ebl, ehdr, scn, shdr, dbg, false);
}

static void
print_debug_types_section (Dwfl_Module *dwflmod, Ebl *ebl, GElf_Ehdr *ehdr,
			   Elf_Scn *scn, GElf_Shdr *shdr, Dwarf *dbg)
{
  print_debug_units (dwflmod, ebl, ehdr, scn, shdr, dbg, true);
}


static void
print_decoded_line_section (Dwfl_Module *dwflmod, Ebl *ebl, GElf_Ehdr *ehdr,
			    Elf_Scn *scn, GElf_Shdr *shdr, Dwarf *dbg)
{
  printf (gettext ("\
\nDWARF section [%2zu] '%s' at offset %#" PRIx64 ":\n\n"),
	  elf_ndxscn (scn), section_name (ebl, ehdr, shdr),
	  (uint64_t) shdr->sh_offset);

  size_t address_size
    = elf_getident (ebl->elf, NULL)[EI_CLASS] == ELFCLASS32 ? 4 : 8;

  Dwarf_Off cuoffset;
  Dwarf_Off ncuoffset = 0;
  size_t hsize;
  while (dwarf_nextcu (dbg, cuoffset = ncuoffset, &ncuoffset, &hsize,
		       NULL, NULL, NULL) == 0)
    {
      Dwarf_Die cudie;
      if (dwarf_offdie (dbg, cuoffset + hsize, &cudie) == NULL)
	continue;

      size_t nlines;
      Dwarf_Lines *lines;
      if (dwarf_getsrclines (&cudie, &lines, &nlines) != 0)
	continue;

      printf (" CU [%" PRIx64 "] %s\n",
	      dwarf_dieoffset (&cudie), dwarf_diename (&cudie));
      printf ("  line:col SBPE* disc isa op address"
	      " (Statement Block Prologue Epilogue *End)\n");
      const char *last_file = "";
      for (size_t n = 0; n < nlines; n++)
	{
	  Dwarf_Line *line = dwarf_onesrcline (lines, n);
	  Dwarf_Word mtime, length;
	  const char *file = dwarf_linesrc (line, &mtime, &length);
	  if (strcmp (last_file, file) != 0)
	    {
	      printf ("  %s (mtime: %" PRIu64 ", length: %" PRIu64 ")\n",
		      file, mtime, length);
	      last_file = file;
	    }

	  int lineno, colno;
	  bool statement, endseq, block, prologue_end, epilogue_begin;
	  unsigned int lineop, isa, disc;
	  Dwarf_Addr address;
	  dwarf_lineaddr (line, &address);
	  dwarf_lineno (line, &lineno);
	  dwarf_linecol (line, &colno);
	  dwarf_lineop_index (line, &lineop);
	  dwarf_linebeginstatement (line, &statement);
	  dwarf_lineendsequence (line, &endseq);
	  dwarf_lineblock (line, &block);
	  dwarf_lineprologueend (line, &prologue_end);
	  dwarf_lineepiloguebegin (line, &epilogue_begin);
	  dwarf_lineisa (line, &isa);
	  dwarf_linediscriminator (line, &disc);

	  /* End sequence is special, it is one byte past.  */
	  char *a = format_dwarf_addr (dwflmod, address_size,
				       address - (endseq ? 1 : 0), address);
	  printf ("  %4d:%-3d %c%c%c%c%c %4d %3d %2d %s\n",
		  lineno, colno,
		  (statement ? 'S' : ' '),
		  (block ? 'B' : ' '),
		  (prologue_end ? 'P' : ' '),
		  (epilogue_begin ? 'E' : ' '),
		  (endseq ? '*' : ' '),
		  disc, isa, lineop, a);
	  free (a);

	  if (endseq)
	    printf("\n");
	}
    }
}


static void
print_debug_line_section (Dwfl_Module *dwflmod, Ebl *ebl, GElf_Ehdr *ehdr,
			  Elf_Scn *scn, GElf_Shdr *shdr, Dwarf *dbg)
{
  if (decodedline)
    {
      print_decoded_line_section (dwflmod, ebl, ehdr, scn, shdr, dbg);
      return;
    }

  printf (gettext ("\
\nDWARF section [%2zu] '%s' at offset %#" PRIx64 ":\n"),
	  elf_ndxscn (scn), section_name (ebl, ehdr, shdr),
	  (uint64_t) shdr->sh_offset);

  if (shdr->sh_size == 0)
    return;

  /* There is no functionality in libdw to read the information in the
     way it is represented here.  Hardcode the decoder.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (unlikely (data == NULL || data->d_buf == NULL))
    {
      error (0, 0, gettext ("cannot get line data section data: %s"),
	     elf_errmsg (-1));
      return;
    }

  const unsigned char *linep = (const unsigned char *) data->d_buf;
  const unsigned char *lineendp;

  while (linep
	 < (lineendp = (const unsigned char *) data->d_buf + data->d_size))
    {
      size_t start_offset = linep - (const unsigned char *) data->d_buf;

      printf (gettext ("\nTable at offset %Zu:\n"), start_offset);

      Dwarf_Word unit_length = read_4ubyte_unaligned_inc (dbg, linep);
      unsigned int length = 4;
      if (unlikely (unit_length == 0xffffffff))
	{
	  if (unlikely (linep + 8 > lineendp))
	    {
	    invalid_data:
	      error (0, 0, gettext ("invalid data in section [%zu] '%s'"),
		     elf_ndxscn (scn), section_name (ebl, ehdr, shdr));
	      return;
	    }
	  unit_length = read_8ubyte_unaligned_inc (dbg, linep);
	  length = 8;
	}

      /* Check whether we have enough room in the section.  */
      if (unit_length < 2 + length + 5 * 1
	  || unlikely (linep + unit_length > lineendp))
	goto invalid_data;
      lineendp = linep + unit_length;

      /* The next element of the header is the version identifier.  */
      uint_fast16_t version = read_2ubyte_unaligned_inc (dbg, linep);

      /* Next comes the header length.  */
      Dwarf_Word header_length;
      if (length == 4)
	header_length = read_4ubyte_unaligned_inc (dbg, linep);
      else
	header_length = read_8ubyte_unaligned_inc (dbg, linep);
      //const unsigned char *header_start = linep;

      /* Next the minimum instruction length.  */
      uint_fast8_t minimum_instr_len = *linep++;

      /* Next the maximum operations per instruction, in version 4 format.  */
      uint_fast8_t max_ops_per_instr = version < 4 ? 1 : *linep++;

	/* Then the flag determining the default value of the is_stmt
	   register.  */
      uint_fast8_t default_is_stmt = *linep++;

      /* Now the line base.  */
      int_fast8_t line_base = *((const int_fast8_t *) linep);
      ++linep;

      /* And the line range.  */
      uint_fast8_t line_range = *linep++;

      /* The opcode base.  */
      uint_fast8_t opcode_base = *linep++;

      /* Print what we got so far.  */
      printf (gettext ("\n"
		       " Length:                     %" PRIu64 "\n"
		       " DWARF version:              %" PRIuFAST16 "\n"
		       " Prologue length:            %" PRIu64 "\n"
		       " Minimum instruction length: %" PRIuFAST8 "\n"
		       " Maximum operations per instruction: %" PRIuFAST8 "\n"
		       " Initial value if '%s': %" PRIuFAST8 "\n"
		       " Line base:                  %" PRIdFAST8 "\n"
		       " Line range:                 %" PRIuFAST8 "\n"
		       " Opcode base:                %" PRIuFAST8 "\n"
		       "\n"
		       "Opcodes:\n"),
	      (uint64_t) unit_length, version, (uint64_t) header_length,
	      minimum_instr_len, max_ops_per_instr,
	      "is_stmt", default_is_stmt, line_base,
	      line_range, opcode_base);

      if (unlikely (linep + opcode_base - 1 >= lineendp))
	{
	invalid_unit:
	  error (0, 0,
		 gettext ("invalid data at offset %tu in section [%zu] '%s'"),
		 linep - (const unsigned char *) data->d_buf,
		 elf_ndxscn (scn), section_name (ebl, ehdr, shdr));
	  linep = lineendp;
	  continue;
	}
      int opcode_base_l10 = 1;
      unsigned int tmp = opcode_base;
      while (tmp > 10)
	{
	  tmp /= 10;
	  ++opcode_base_l10;
	}
      const uint8_t *standard_opcode_lengths = linep - 1;
      for (uint_fast8_t cnt = 1; cnt < opcode_base; ++cnt)
	printf (ngettext ("  [%*" PRIuFAST8 "]  %hhu argument\n",
			  "  [%*" PRIuFAST8 "]  %hhu arguments\n",
			  (int) linep[cnt - 1]),
		opcode_base_l10, cnt, linep[cnt - 1]);
      linep += opcode_base - 1;
      if (unlikely (linep >= lineendp))
	goto invalid_unit;

      puts (gettext ("\nDirectory table:"));
      while (*linep != 0)
	{
	  unsigned char *endp = memchr (linep, '\0', lineendp - linep);
	  if (unlikely (endp == NULL))
	    goto invalid_unit;

	  printf (" %s\n", (char *) linep);

	  linep = endp + 1;
	}
      /* Skip the final NUL byte.  */
      ++linep;

      if (unlikely (linep >= lineendp))
	goto invalid_unit;
      puts (gettext ("\nFile name table:\n"
		     " Entry Dir   Time      Size      Name"));
      for (unsigned int cnt = 1; *linep != 0; ++cnt)
	{
	  /* First comes the file name.  */
	  char *fname = (char *) linep;
	  unsigned char *endp = memchr (fname, '\0', lineendp - linep);
	  if (unlikely (endp == NULL))
	    goto invalid_unit;
	  linep = endp + 1;

	  /* Then the index.  */
	  unsigned int diridx;
	  get_uleb128 (diridx, linep);

	  /* Next comes the modification time.  */
	  unsigned int mtime;
	  get_uleb128 (mtime, linep);

	  /* Finally the length of the file.  */
	  unsigned int fsize;
	  get_uleb128 (fsize, linep);

	  printf (" %-5u %-5u %-9u %-9u %s\n",
		  cnt, diridx, mtime, fsize, fname);
	}
      /* Skip the final NUL byte.  */
      ++linep;

      puts (gettext ("\nLine number statements:"));
      Dwarf_Word address = 0;
      unsigned int op_index = 0;
      size_t line = 1;
      uint_fast8_t is_stmt = default_is_stmt;

      /* Default address value, in case we do not find the CU.  */
      size_t address_size
	= elf_getident (ebl->elf, NULL)[EI_CLASS] == ELFCLASS32 ? 4 : 8;

      /* Determine the CU this block is for.  */
      Dwarf_Off cuoffset;
      Dwarf_Off ncuoffset = 0;
      size_t hsize;
      while (dwarf_nextcu (dbg, cuoffset = ncuoffset, &ncuoffset, &hsize,
			   NULL, NULL, NULL) == 0)
	{
	  Dwarf_Die cudie;
	  if (dwarf_offdie (dbg, cuoffset + hsize, &cudie) == NULL)
	    continue;
	  Dwarf_Attribute stmt_list;
	  if (dwarf_attr (&cudie, DW_AT_stmt_list, &stmt_list) == NULL)
	    continue;
	  Dwarf_Word lineoff;
	  if (dwarf_formudata (&stmt_list, &lineoff) != 0)
	    continue;
	  if (lineoff == start_offset)
	    {
	      /* Found the CU.  */
	      address_size = cudie.cu->address_size;
	      break;
	    }
	}

      /* Apply the "operation advance" from a special opcode
	 or DW_LNS_advance_pc (as per DWARF4 6.2.5.1).  */
      unsigned int op_addr_advance;
      bool show_op_index;
      inline void advance_pc (unsigned int op_advance)
      {
	op_addr_advance = minimum_instr_len * ((op_index + op_advance)
					       / max_ops_per_instr);
	address += op_advance;
	show_op_index = (op_index > 0 ||
			 (op_index + op_advance) % max_ops_per_instr > 0);
	op_index = (op_index + op_advance) % max_ops_per_instr;
      }

      while (linep < lineendp)
	{
	  size_t offset = linep - (const unsigned char *) data->d_buf;
	  unsigned int u128;
	  int s128;

	  /* Read the opcode.  */
	  unsigned int opcode = *linep++;

	  printf (" [%6" PRIx64 "]", (uint64_t)offset);
	  /* Is this a special opcode?  */
	  if (likely (opcode >= opcode_base))
	    {
	      /* Yes.  Handling this is quite easy since the opcode value
		 is computed with

		 opcode = (desired line increment - line_base)
			   + (line_range * address advance) + opcode_base
	      */
	      int line_increment = (line_base
				    + (opcode - opcode_base) % line_range);

	      /* Perform the increments.  */
	      line += line_increment;
	      advance_pc ((opcode - opcode_base) / line_range);

	      char *a = format_dwarf_addr (dwflmod, 0, address, address);
	      if (show_op_index)
		printf (gettext ("\
 special opcode %u: address+%u = %s, op_index = %u, line%+d = %zu\n"),
			opcode, op_addr_advance, a, op_index,
			line_increment, line);
	      else
		printf (gettext ("\
 special opcode %u: address+%u = %s, line%+d = %zu\n"),
			opcode, op_addr_advance, a, line_increment, line);
	      free (a);
	    }
	  else if (opcode == 0)
	    {
	      /* This an extended opcode.  */
	      if (unlikely (linep + 2 > lineendp))
		goto invalid_unit;

	      /* The length.  */
	      unsigned int len = *linep++;

	      if (unlikely (linep + len > lineendp))
		goto invalid_unit;

	      /* The sub-opcode.  */
	      opcode = *linep++;

	      printf (gettext (" extended opcode %u: "), opcode);

	      switch (opcode)
		{
		case DW_LNE_end_sequence:
		  puts (gettext (" end of sequence"));

		  /* Reset the registers we care about.  */
		  address = 0;
		  op_index = 0;
		  line = 1;
		  is_stmt = default_is_stmt;
		  break;

		case DW_LNE_set_address:
		  op_index = 0;
		  if (address_size == 4)
		    address = read_4ubyte_unaligned_inc (dbg, linep);
		  else
		    address = read_8ubyte_unaligned_inc (dbg, linep);
		  {
		    char *a = format_dwarf_addr (dwflmod, 0, address, address);
		    printf (gettext (" set address to %s\n"), a);
		    free (a);
		  }
		  break;

		case DW_LNE_define_file:
		  {
		    char *fname = (char *) linep;
		    unsigned char *endp = memchr (linep, '\0',
						  lineendp - linep);
		    if (unlikely (endp == NULL))
		      goto invalid_unit;
		    linep = endp + 1;

		    unsigned int diridx;
		    get_uleb128 (diridx, linep);
		    Dwarf_Word mtime;
		    get_uleb128 (mtime, linep);
		    Dwarf_Word filelength;
		    get_uleb128 (filelength, linep);

		    printf (gettext ("\
 define new file: dir=%u, mtime=%" PRIu64 ", length=%" PRIu64 ", name=%s\n"),
			    diridx, (uint64_t) mtime, (uint64_t) filelength,
			    fname);
		  }
		  break;

		case DW_LNE_set_discriminator:
		  /* Takes one ULEB128 parameter, the discriminator.  */
		  if (unlikely (standard_opcode_lengths[opcode] != 1))
		    goto invalid_unit;

		  get_uleb128 (u128, linep);
		  printf (gettext (" set discriminator to %u\n"), u128);
		  break;

		default:
		  /* Unknown, ignore it.  */
		  puts (gettext (" unknown opcode"));
		  linep += len - 1;
		  break;
		}
	    }
	  else if (opcode <= DW_LNS_set_isa)
	    {
	      /* This is a known standard opcode.  */
	      switch (opcode)
		{
		case DW_LNS_copy:
		  /* Takes no argument.  */
		  puts (gettext (" copy"));
		  break;

		case DW_LNS_advance_pc:
		  /* Takes one uleb128 parameter which is added to the
		     address.  */
		  get_uleb128 (u128, linep);
		  advance_pc (u128);
		  {
		    char *a = format_dwarf_addr (dwflmod, 0, address, address);
		    if (show_op_index)
		      printf (gettext ("\
 advance address by %u to %s, op_index to %u\n"),
			      op_addr_advance, a, op_index);
		    else
		      printf (gettext (" advance address by %u to %s\n"),
			      op_addr_advance, a);
		    free (a);
		  }
		  break;

		case DW_LNS_advance_line:
		  /* Takes one sleb128 parameter which is added to the
		     line.  */
		  get_sleb128 (s128, linep);
		  line += s128;
		  printf (gettext ("\
 advance line by constant %d to %" PRId64 "\n"),
			  s128, (int64_t) line);
		  break;

		case DW_LNS_set_file:
		  /* Takes one uleb128 parameter which is stored in file.  */
		  get_uleb128 (u128, linep);
		  printf (gettext (" set file to %" PRIu64 "\n"),
			  (uint64_t) u128);
		  break;

		case DW_LNS_set_column:
		  /* Takes one uleb128 parameter which is stored in column.  */
		  if (unlikely (standard_opcode_lengths[opcode] != 1))
		    goto invalid_unit;

		  get_uleb128 (u128, linep);
		  printf (gettext (" set column to %" PRIu64 "\n"),
			  (uint64_t) u128);
		  break;

		case DW_LNS_negate_stmt:
		  /* Takes no argument.  */
		  is_stmt = 1 - is_stmt;
		  printf (gettext (" set '%s' to %" PRIuFAST8 "\n"),
			  "is_stmt", is_stmt);
		  break;

		case DW_LNS_set_basic_block:
		  /* Takes no argument.  */
		  puts (gettext (" set basic block flag"));
		  break;

		case DW_LNS_const_add_pc:
		  /* Takes no argument.  */
		  advance_pc ((255 - opcode_base) / line_range);
		  {
		    char *a = format_dwarf_addr (dwflmod, 0, address, address);
		    if (show_op_index)
		      printf (gettext ("\
 advance address by constant %u to %s, op_index to %u\n"),
			      op_addr_advance, a, op_index);
		    else
		      printf (gettext ("\
 advance address by constant %u to %s\n"),
			      op_addr_advance, a);
		    free (a);
		  }
		  break;

		case DW_LNS_fixed_advance_pc:
		  /* Takes one 16 bit parameter which is added to the
		     address.  */
		  if (unlikely (standard_opcode_lengths[opcode] != 1))
		    goto invalid_unit;

		  u128 = read_2ubyte_unaligned_inc (dbg, linep);
		  address += u128;
		  op_index = 0;
		  {
		    char *a = format_dwarf_addr (dwflmod, 0, address, address);
		    printf (gettext ("\
 advance address by fixed value %u to %s\n"),
			    u128, a);
		    free (a);
		  }
		  break;

		case DW_LNS_set_prologue_end:
		  /* Takes no argument.  */
		  puts (gettext (" set prologue end flag"));
		  break;

		case DW_LNS_set_epilogue_begin:
		  /* Takes no argument.  */
		  puts (gettext (" set epilogue begin flag"));
		  break;

		case DW_LNS_set_isa:
		  /* Takes one uleb128 parameter which is stored in isa.  */
		  if (unlikely (standard_opcode_lengths[opcode] != 1))
		    goto invalid_unit;

		  get_uleb128 (u128, linep);
		  printf (gettext (" set isa to %u\n"), u128);
		  break;
		}
	    }
	  else
	    {
	      /* This is a new opcode the generator but not we know about.
		 Read the parameters associated with it but then discard
		 everything.  Read all the parameters for this opcode.  */
	      printf (ngettext (" unknown opcode with %" PRIu8 " parameter:",
				" unknown opcode with %" PRIu8 " parameters:",
				standard_opcode_lengths[opcode]),
		      standard_opcode_lengths[opcode]);
	      for (int n = standard_opcode_lengths[opcode]; n > 0; --n)
		{
		  get_uleb128 (u128, linep);
		  if (n != standard_opcode_lengths[opcode])
		    putc_unlocked (',', stdout);
		  printf (" %u", u128);
		}

	      /* Next round, ignore this opcode.  */
	      continue;
	    }
	}
    }

  /* There must only be one data block.  */
  assert (elf_getdata (scn, data) == NULL);
}


static void
print_debug_loc_section (Dwfl_Module *dwflmod,
			 Ebl *ebl, GElf_Ehdr *ehdr,
			 Elf_Scn *scn, GElf_Shdr *shdr, Dwarf *dbg)
{
  Elf_Data *data = elf_rawdata (scn, NULL);

  if (unlikely (data == NULL))
    {
      error (0, 0, gettext ("cannot get .debug_loc content: %s"),
	     elf_errmsg (-1));
      return;
    }

  printf (gettext ("\
\nDWARF section [%2zu] '%s' at offset %#" PRIx64 ":\n"),
	  elf_ndxscn (scn), section_name (ebl, ehdr, shdr),
	  (uint64_t) shdr->sh_offset);

  sort_listptr (&known_loclistptr, "loclistptr");
  size_t listptr_idx = 0;

  uint_fast8_t address_size = ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 4 : 8;
  uint_fast8_t offset_size = 4;

  bool first = true;
  struct Dwarf_CU *cu = NULL;
  Dwarf_Addr base = 0;
  unsigned char *readp = data->d_buf;
  unsigned char *const endp = (unsigned char *) data->d_buf + data->d_size;
  while (readp < endp)
    {
      ptrdiff_t offset = readp - (unsigned char *) data->d_buf;

      if (first && skip_listptr_hole (&known_loclistptr, &listptr_idx,
				      &address_size, &offset_size, &base,
				      &cu, offset, &readp, endp))
	continue;

      if (unlikely (data->d_size - offset < (size_t) address_size * 2))
	{
	  printf (gettext (" [%6tx]  <INVALID DATA>\n"), offset);
	  break;
	}

      Dwarf_Addr begin;
      Dwarf_Addr end;
      if (address_size == 8)
	{
	  begin = read_8ubyte_unaligned_inc (dbg, readp);
	  end = read_8ubyte_unaligned_inc (dbg, readp);
	}
      else
	{
	  begin = read_4ubyte_unaligned_inc (dbg, readp);
	  end = read_4ubyte_unaligned_inc (dbg, readp);
	  if (begin == (Dwarf_Addr) (uint32_t) -1)
	    begin = (Dwarf_Addr) -1l;
	}

      if (begin == (Dwarf_Addr) -1l) /* Base address entry.  */
	{
	  char *b = format_dwarf_addr (dwflmod, address_size, end, end);
	  printf (gettext (" [%6tx]  base address %s\n"), offset, b);
	  free (b);
	  base = end;
	}
      else if (begin == 0 && end == 0) /* End of list entry.  */
	{
	  if (first)
	    printf (gettext (" [%6tx]  empty list\n"), offset);
	  first = true;
	}
      else
	{
	  /* We have a location expression entry.  */
	  uint_fast16_t len = read_2ubyte_unaligned_inc (dbg, readp);

	  char *b = format_dwarf_addr (dwflmod, address_size, base + begin,
				       begin);
	  char *e = format_dwarf_addr (dwflmod, address_size, base + end,
				       end);

	  if (first)		/* First entry in a list.  */
	    printf (gettext (" [%6tx]  %s..%s"), offset, b, e);
	  else
	    printf (gettext ("           %s..%s"), b, e);

	  free (b);
	  free (e);

	  if (endp - readp <= (ptrdiff_t) len)
	    {
	      fputs (gettext ("   <INVALID DATA>\n"), stdout);
	      break;
	    }

	  print_ops (dwflmod, dbg, 1, 18 + (address_size * 4),
		     3 /*XXX*/, address_size, offset_size, cu, len, readp);

	  first = false;
	  readp += len;
	}
    }
}

struct mac_culist
{
  Dwarf_Die die;
  Dwarf_Off offset;
  Dwarf_Files *files;
  struct mac_culist *next;
};


static int
mac_compare (const void *p1, const void *p2)
{
  struct mac_culist *m1 = (struct mac_culist *) p1;
  struct mac_culist *m2 = (struct mac_culist *) p2;

  if (m1->offset < m2->offset)
    return -1;
  if (m1->offset > m2->offset)
    return 1;
  return 0;
}


static void
print_debug_macinfo_section (Dwfl_Module *dwflmod __attribute__ ((unused)),
			     Ebl *ebl, GElf_Ehdr *ehdr,
			     Elf_Scn *scn, GElf_Shdr *shdr, Dwarf *dbg)
{
  printf (gettext ("\
\nDWARF section [%2zu] '%s' at offset %#" PRIx64 ":\n"),
	  elf_ndxscn (scn), section_name (ebl, ehdr, shdr),
	  (uint64_t) shdr->sh_offset);
  putc_unlocked ('\n', stdout);

  /* There is no function in libdw to iterate over the raw content of
     the section but it is easy enough to do.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (unlikely (data == NULL || data->d_buf == NULL))
    {
      error (0, 0, gettext ("cannot get macro information section data: %s"),
	     elf_errmsg (-1));
      return;
    }

  /* Get the source file information for all CUs.  */
  Dwarf_Off offset;
  Dwarf_Off ncu = 0;
  size_t hsize;
  struct mac_culist *culist = NULL;
  size_t nculist = 0;
  while (dwarf_nextcu (dbg, offset = ncu, &ncu, &hsize, NULL, NULL, NULL) == 0)
    {
      Dwarf_Die cudie;
      if (dwarf_offdie (dbg, offset + hsize, &cudie) == NULL)
	continue;

      Dwarf_Attribute attr;
      if (dwarf_attr (&cudie, DW_AT_macro_info, &attr) == NULL)
	continue;

      Dwarf_Word macoff;
      if (dwarf_formudata (&attr, &macoff) != 0)
	continue;

      struct mac_culist *newp = (struct mac_culist *) alloca (sizeof (*newp));
      newp->die = cudie;
      newp->offset = macoff;
      newp->files = NULL;
      newp->next = culist;
      culist = newp;
      ++nculist;
    }

  /* Convert the list into an array for easier consumption.  */
  struct mac_culist *cus = (struct mac_culist *) alloca ((nculist + 1)
							 * sizeof (*cus));
  /* Add sentinel.  */
  cus[nculist].offset = data->d_size;
  if (nculist > 0)
    {
      for (size_t cnt = nculist - 1; culist != NULL; --cnt)
	{
	  assert (cnt < nculist);
	  cus[cnt] = *culist;
	  culist = culist->next;
	}

      /* Sort the array according to the offset in the .debug_macinfo
	 section.  Note we keep the sentinel at the end.  */
      qsort (cus, nculist, sizeof (*cus), mac_compare);
    }

  const unsigned char *readp = (const unsigned char *) data->d_buf;
  const unsigned char *readendp = readp + data->d_size;
  int level = 1;

  while (readp < readendp)
    {
      unsigned int opcode = *readp++;
      unsigned int u128;
      unsigned int u128_2;
      const unsigned char *endp;

      switch (opcode)
	{
	case DW_MACINFO_define:
	case DW_MACINFO_undef:
	case DW_MACINFO_vendor_ext:
	  /*  For the first two opcodes the parameters are
		line, string
	      For the latter
		number, string.
	      We can treat these cases together.  */
	  get_uleb128 (u128, readp);

	  endp = memchr (readp, '\0', readendp - readp);
	  if (unlikely (endp == NULL))
	    {
	      printf (gettext ("\
%*s*** non-terminated string at end of section"),
		      level, "");
	      return;
	    }

	  if (opcode == DW_MACINFO_define)
	    printf ("%*s#define %s, line %u\n",
		    level, "", (char *) readp, u128);
	  else if (opcode == DW_MACINFO_undef)
	    printf ("%*s#undef %s, line %u\n",
		    level, "", (char *) readp, u128);
	  else
	    printf (" #vendor-ext %s, number %u\n", (char *) readp, u128);

	  readp = endp + 1;
	  break;

	case DW_MACINFO_start_file:
	  /* The two parameters are line and file index, in this order.  */
	  get_uleb128 (u128, readp);
	  get_uleb128 (u128_2, readp);

	  /* Find the CU DIE for this file.  */
	  size_t macoff = readp - (const unsigned char *) data->d_buf;
	  const char *fname = "???";
	  if (macoff >= cus[0].offset)
	    {
	      while (macoff >= cus[1].offset)
		++cus;

	      if (cus[0].files == NULL
		&& dwarf_getsrcfiles (&cus[0].die, &cus[0].files, NULL) != 0)
		cus[0].files = (Dwarf_Files *) -1l;

	      if (cus[0].files != (Dwarf_Files *) -1l)
		fname = (dwarf_filesrc (cus[0].files, u128_2, NULL, NULL)
			 ?: "???");
	    }

	  printf ("%*sstart_file %u, [%u] %s\n",
		  level, "", u128, u128_2, fname);
	  ++level;
	  break;

	case DW_MACINFO_end_file:
	  --level;
	  printf ("%*send_file\n", level, "");
	  /* Nothing more to do.  */
	  break;

	default:
	  // XXX gcc seems to generate files with a trailing zero.
	  if (unlikely (opcode != 0 || readp != readendp))
	    printf ("%*s*** invalid opcode %u\n", level, "", opcode);
	  break;
	}
    }
}


static void
print_debug_macro_section (Dwfl_Module *dwflmod __attribute__ ((unused)),
			   Ebl *ebl, GElf_Ehdr *ehdr,
			   Elf_Scn *scn, GElf_Shdr *shdr, Dwarf *dbg)
{
  printf (gettext ("\
\nDWARF section [%2zu] '%s' at offset %#" PRIx64 ":\n"),
	  elf_ndxscn (scn), section_name (ebl, ehdr, shdr),
	  (uint64_t) shdr->sh_offset);
  putc_unlocked ('\n', stdout);

  Elf_Data *data = elf_getdata (scn, NULL);
  if (unlikely (data == NULL || data->d_buf == NULL))
    {
      error (0, 0, gettext ("cannot get macro information section data: %s"),
	     elf_errmsg (-1));
      return;
    }

  /* Get the source file information for all CUs.  Uses same
     datastructure as macinfo.  But uses offset field to directly
     match .debug_line offset.  And just stored in a list.  */
  Dwarf_Off offset;
  Dwarf_Off ncu = 0;
  size_t hsize;
  struct mac_culist *culist = NULL;
  size_t nculist = 0;
  while (dwarf_nextcu (dbg, offset = ncu, &ncu, &hsize, NULL, NULL, NULL) == 0)
    {
      Dwarf_Die cudie;
      if (dwarf_offdie (dbg, offset + hsize, &cudie) == NULL)
	continue;

      Dwarf_Attribute attr;
      if (dwarf_attr (&cudie, DW_AT_stmt_list, &attr) == NULL)
	continue;

      Dwarf_Word lineoff;
      if (dwarf_formudata (&attr, &lineoff) != 0)
	continue;

      struct mac_culist *newp = (struct mac_culist *) alloca (sizeof (*newp));
      newp->die = cudie;
      newp->offset = lineoff;
      newp->files = NULL;
      newp->next = culist;
      culist = newp;
      ++nculist;
    }

  const unsigned char *readp = (const unsigned char *) data->d_buf;
  const unsigned char *readendp = readp + data->d_size;

  while (readp < readendp)
    {
      printf (gettext (" Offset:             0x%" PRIx64 "\n"),
	      (uint64_t) (readp - (const unsigned char *) data->d_buf));

      // Header, 2 byte version, 1 byte flag, optional .debug_line offset,
      // optional vendor extension macro entry table.
      if (readp + 2 > readendp)
	{
	invalid_data:
	  error (0, 0, gettext ("invalid data"));
	  return;
	}
      const uint16_t vers = read_2ubyte_unaligned_inc (dbg, readp);
      printf (gettext (" Version:            %" PRIu16 "\n"), vers);

      // Version 4 is the GNU extension for DWARF4.  DWARF5 will use version
      // 5 when it gets standardized.
      if (vers != 4)
	{
	  printf (gettext ("  unknown version, cannot parse section\n"));
	  return;
	}

      if (readp + 1 > readendp)
	goto invalid_data;
      const unsigned char flag = *readp++;
      printf (gettext (" Flag:               0x%" PRIx8 "\n"), flag);

      unsigned int offset_len = (flag & 0x01) ? 8 : 4;
      printf (gettext (" Offset length:      %" PRIu8 "\n"), offset_len);
      Dwarf_Off line_offset = -1;
      if (flag & 0x02)
	{
	  if (offset_len == 8)
	    line_offset = read_8ubyte_unaligned_inc (dbg, readp);
	  else
	    line_offset = read_4ubyte_unaligned_inc (dbg, readp);
	  printf (gettext (" .debug_line offset: 0x%" PRIx64 "\n"),
		  line_offset);
	}

      const unsigned char *vendor[DW_MACRO_GNU_hi_user - DW_MACRO_GNU_lo_user];
      if (flag & 0x04)
	{
	  // 1 byte length, for each item, 1 byte opcode, uleb128 number
	  // of arguments, for each argument 1 byte form code.
	  if (readp + 1 > readendp)
	    goto invalid_data;
	  unsigned int tlen = *readp++;
	  printf (gettext ("  extension opcode table, %" PRIu8 " items:\n"),
		  tlen);
	  for (unsigned int i = 0; i < tlen; i++)
	    {
	      if (readp + 1 > readendp)
		goto invalid_data;
	      unsigned int opcode = *readp++;
	      printf (gettext ("    [%" PRIx8 "]"), opcode);
	      if (opcode < DW_MACRO_GNU_lo_user
		  || opcode > DW_MACRO_GNU_hi_user)
		goto invalid_data;
	      // Record the start of description for this vendor opcode.
	      // uleb128 nr args, 1 byte per arg form.
	      vendor[opcode - DW_MACRO_GNU_lo_user] = readp;
	      if (readp + 1 > readendp)
		goto invalid_data;
	      unsigned int args = *readp++;
	      if (args > 0)
		{
		  printf (gettext (" %" PRIu8 " arguments:"), args);
		  while (args > 0)
		    {
		      if (readp + 1 > readendp)
			goto invalid_data;
		      unsigned int form = *readp++;
		      printf (" %s", dwarf_form_string (form));
		      if (form != DW_FORM_data1
			  && form != DW_FORM_data2
			  && form != DW_FORM_data4
			  && form != DW_FORM_data8
			  && form != DW_FORM_sdata
			  && form != DW_FORM_udata
			  && form != DW_FORM_block
			  && form != DW_FORM_block1
			  && form != DW_FORM_block2
			  && form != DW_FORM_block4
			  && form != DW_FORM_flag
			  && form != DW_FORM_string
			  && form != DW_FORM_strp
			  && form != DW_FORM_sec_offset)
			goto invalid_data;
		      args--;
		      if (args > 0)
			putchar_unlocked (',');
		    }
		}
	      else
		printf (gettext (" no arguments."));
	      putchar_unlocked ('\n');
	    }
	}
      putchar_unlocked ('\n');

      int level = 1;
      if (readp + 1 > readendp)
	goto invalid_data;
      unsigned int opcode = *readp++;
      while (opcode != 0)
	{
	  unsigned int u128;
	  unsigned int u128_2;
	  const unsigned char *endp;
	  uint64_t off;

          switch (opcode)
            {
            case DW_MACRO_GNU_start_file:
	      get_uleb128 (u128, readp);
	      get_uleb128 (u128_2, readp);

	      /* Find the CU DIE that matches this line offset.  */
	      const char *fname = "???";
	      if (line_offset != (Dwarf_Off) -1)
		{
		  struct mac_culist *cu = culist;
		  while (cu != NULL && line_offset != cu->offset)
		    cu = cu->next;
		  if (cu != NULL)
		    {
		      if (cu->files == NULL
			  && dwarf_getsrcfiles (&cu->die, &cu->files,
						NULL) != 0)
			cu->files = (Dwarf_Files *) -1l;

		      if (cu->files != (Dwarf_Files *) -1l)
			fname = (dwarf_filesrc (cu->files, u128_2,
						NULL, NULL) ?: "???");
		    }
		}
	      printf ("%*sstart_file %u, [%u] %s\n",
		      level, "", u128, u128_2, fname);
	      ++level;
	      break;

	    case DW_MACRO_GNU_end_file:
	      --level;
	      printf ("%*send_file\n", level, "");
	      break;

	    case DW_MACRO_GNU_define:
	      get_uleb128 (u128, readp);
	      endp = memchr (readp, '\0', readendp - readp);
	      if (endp == NULL)
		goto invalid_data;
	      printf ("%*s#define %s, line %u\n",
		      level, "", readp, u128);
	      readp = endp + 1;
	      break;

	    case DW_MACRO_GNU_undef:
	      get_uleb128 (u128, readp);
	      endp = memchr (readp, '\0', readendp - readp);
	      if (endp == NULL)
		goto invalid_data;
	      printf ("%*s#undef %s, line %u\n",
		      level, "", readp, u128);
	      readp = endp + 1;
	      break;

	    case DW_MACRO_GNU_define_indirect:
	      get_uleb128 (u128, readp);
	      if (readp + offset_len > readendp)
		goto invalid_data;
	      if (offset_len == 8)
		off = read_8ubyte_unaligned_inc (dbg, readp);
	      else
		off = read_4ubyte_unaligned_inc (dbg, readp);
	      printf ("%*s#define %s, line %u (indirect)\n",
		      level, "", dwarf_getstring (dbg, off, NULL), u128);
	      break;

	    case DW_MACRO_GNU_undef_indirect:
	      get_uleb128 (u128, readp);
	      if (readp + offset_len > readendp)
		goto invalid_data;
	      if (offset_len == 8)
		off = read_8ubyte_unaligned_inc (dbg, readp);
	      else
		off = read_4ubyte_unaligned_inc (dbg, readp);
	      printf ("%*s#undef %s, line %u (indirect)\n",
		      level, "", dwarf_getstring (dbg, off, NULL), u128);
	      break;

	    case DW_MACRO_GNU_transparent_include:
	      if (readp + offset_len > readendp)
		goto invalid_data;
	      if (offset_len == 8)
		off = read_8ubyte_unaligned_inc (dbg, readp);
	      else
		off = read_4ubyte_unaligned_inc (dbg, readp);
	      printf ("%*s#include offset 0x%" PRIx64 "\n",
		      level, "", off);
	      break;

	    default:
	      printf ("%*svendor opcode 0x%" PRIx8, level, "", opcode);
	      if (opcode < DW_MACRO_GNU_lo_user
		  || opcode > DW_MACRO_GNU_lo_user
		  || vendor[opcode - DW_MACRO_GNU_lo_user] == NULL)
		goto invalid_data;

	      const unsigned char *op_desc;
	      op_desc = vendor[opcode - DW_MACRO_GNU_lo_user];

	      // Just skip the arguments, we cannot really interpret them,
	      // but print as much as we can.
	      unsigned int args = *op_desc++;
	      while (args > 0)
		{
		  unsigned int form = *op_desc++;
		  Dwarf_Word val;
		  switch (form)
		    {
		    case DW_FORM_data1:
		      if (readp + 1 > readendp)
			goto invalid_data;
		      val = *readp++;
		      printf (" %" PRIx8, (unsigned int) val);
		      break;

		    case DW_FORM_data2:
		      if (readp + 2 > readendp)
			goto invalid_data;
		      val = read_2ubyte_unaligned_inc (dbg, readp);
		      printf(" %" PRIx16, (unsigned int) val);
		      break;

		    case DW_FORM_data4:
		      if (readp + 4 > readendp)
			goto invalid_data;
		      val = read_4ubyte_unaligned_inc (dbg, readp);
		      printf (" %" PRIx32, (unsigned int) val);
		      break;

		    case DW_FORM_data8:
		      if (readp + 8 > readendp)
			goto invalid_data;
		      val = read_8ubyte_unaligned_inc (dbg, readp);
		      printf (" %" PRIx64, val);
		      break;

		    case DW_FORM_sdata:
		      get_sleb128 (val, readp);
		      printf (" %" PRIx64, val);
		      break;

		    case DW_FORM_udata:
		      get_uleb128 (val, readp);
		      printf (" %" PRIx64, val);
		      break;

		    case DW_FORM_block:
		      get_uleb128 (val, readp);
		      printf (" block[%" PRIu64 "]", val);
		      if (readp + val > readendp)
			goto invalid_data;
		      readp += val;
		      break;

		    case DW_FORM_block1:
		      if (readp + 1 > readendp)
			goto invalid_data;
		      val = *readp++;
		      printf (" block[%" PRIu64 "]", val);
		      if (readp + val > readendp)
			goto invalid_data;
		      break;

		    case DW_FORM_block2:
		      if (readp + 2 > readendp)
			goto invalid_data;
		      val = read_2ubyte_unaligned_inc (dbg, readp);
		      printf (" block[%" PRIu64 "]", val);
		      if (readp + val > readendp)
			goto invalid_data;
		      break;

		    case DW_FORM_block4:
		      if (readp + 2 > readendp)
			goto invalid_data;
		      val =read_4ubyte_unaligned_inc (dbg, readp);
		      printf (" block[%" PRIu64 "]", val);
		      if (readp + val > readendp)
			goto invalid_data;
		      break;

		    case DW_FORM_flag:
		      if (readp + 1 > readendp)
			goto invalid_data;
		      val = *readp++;
		      printf (" %s", nl_langinfo (val != 0 ? YESSTR : NOSTR));
		      break;

		    case DW_FORM_string:
		      endp = memchr (readp, '\0', readendp - readp);
		      if (endp == NULL)
			goto invalid_data;
		      printf (" %s", readp);
		      readp = endp + 1;
		      break;

		    case DW_FORM_strp:
		      if (readp + offset_len > readendp)
			goto invalid_data;
		      if (offset_len == 8)
			val = read_8ubyte_unaligned_inc (dbg, readp);
		      else
			val = read_4ubyte_unaligned_inc (dbg, readp);
		      printf (" %s", dwarf_getstring (dbg, val, NULL));
		      break;

		    case DW_FORM_sec_offset:
		      if (readp + offset_len > readendp)
			goto invalid_data;
		      if (offset_len == 8)
			val = read_8ubyte_unaligned_inc (dbg, readp);
		      else
			val = read_4ubyte_unaligned_inc (dbg, readp);
		      printf (" %" PRIx64, val);
		      break;

		      default:
			error (0, 0, gettext ("vendor opcode not verified?"));
			return;
		    }

		  args--;
		  if (args > 0)
		    putchar_unlocked (',');
		}
	      putchar_unlocked ('\n');
	    }

	  if (readp + 1 > readendp)
	    goto invalid_data;
	  opcode = *readp++;
	  if (opcode == 0)
	    putchar_unlocked ('\n');
	}
    }
}


/* Callback for printing global names.  */
static int
print_pubnames (Dwarf *dbg __attribute__ ((unused)), Dwarf_Global *global,
		void *arg)
{
  int *np = (int *) arg;

  printf (gettext (" [%5d] DIE offset: %6" PRId64
		   ", CU DIE offset: %6" PRId64 ", name: %s\n"),
	  (*np)++, global->die_offset, global->cu_offset, global->name);

  return 0;
}


/* Print the known exported symbols in the DWARF section '.debug_pubnames'.  */
static void
print_debug_pubnames_section (Dwfl_Module *dwflmod __attribute__ ((unused)),
			      Ebl *ebl, GElf_Ehdr *ehdr,
			      Elf_Scn *scn, GElf_Shdr *shdr, Dwarf *dbg)
{
  printf (gettext ("\nDWARF section [%2zu] '%s' at offset %#" PRIx64 ":\n"),
	  elf_ndxscn (scn), section_name (ebl, ehdr, shdr),
	  (uint64_t) shdr->sh_offset);

  int n = 0;
  (void) dwarf_getpubnames (dbg, print_pubnames, &n, 0);
}

/* Print the content of the DWARF string section '.debug_str'.  */
static void
print_debug_str_section (Dwfl_Module *dwflmod __attribute__ ((unused)),
			 Ebl *ebl, GElf_Ehdr *ehdr,
			 Elf_Scn *scn, GElf_Shdr *shdr, Dwarf *dbg)
{
  const size_t sh_size = (dbg->sectiondata[IDX_debug_str] ?
			  dbg->sectiondata[IDX_debug_str]->d_size : 0);

  /* Compute floor(log16(shdr->sh_size)).  */
  GElf_Addr tmp = sh_size;
  int digits = 1;
  while (tmp >= 16)
    {
      ++digits;
      tmp >>= 4;
    }
  digits = MAX (4, digits);

  printf (gettext ("\nDWARF section [%2zu] '%s' at offset %#" PRIx64 ":\n"
		   " %*s  String\n"),
	  elf_ndxscn (scn),
	  section_name (ebl, ehdr, shdr), (uint64_t) shdr->sh_offset,
	  /* TRANS: the debugstr| prefix makes the string unique.  */
	  digits + 2, sgettext ("debugstr|Offset"));

  Dwarf_Off offset = 0;
  while (offset < sh_size)
    {
      size_t len;
      const char *str = dwarf_getstring (dbg, offset, &len);
      if (unlikely (str == NULL))
	{
	  printf (gettext (" *** error while reading strings: %s\n"),
		  dwarf_errmsg (-1));
	  break;
	}

      printf (" [%*" PRIx64 "]  \"%s\"\n", digits, (uint64_t) offset, str);

      offset += len + 1;
    }
}


/* Print the content of the call frame search table section
   '.eh_frame_hdr'.  */
static void
print_debug_frame_hdr_section (Dwfl_Module *dwflmod __attribute__ ((unused)),
			       Ebl *ebl __attribute__ ((unused)),
			       GElf_Ehdr *ehdr __attribute__ ((unused)),
			       Elf_Scn *scn, GElf_Shdr *shdr, Dwarf *dbg)
{
  printf (gettext ("\
\nCall frame search table section [%2zu] '.eh_frame_hdr':\n"),
	  elf_ndxscn (scn));

  Elf_Data *data = elf_rawdata (scn, NULL);

  if (unlikely (data == NULL))
    {
      error (0, 0, gettext ("cannot get %s content: %s"),
	     ".eh_frame_hdr", elf_errmsg (-1));
      return;
    }

  const unsigned char *readp = data->d_buf;
  const unsigned char *const dataend = ((unsigned char *) data->d_buf
					+ data->d_size);

  if (unlikely (readp + 4 > dataend))
    {
    invalid_data:
      error (0, 0, gettext ("invalid data"));
      return;
    }

  unsigned int version = *readp++;
  unsigned int eh_frame_ptr_enc = *readp++;
  unsigned int fde_count_enc = *readp++;
  unsigned int table_enc = *readp++;

  printf (" version:          %u\n"
	  " eh_frame_ptr_enc: %#x ",
	  version, eh_frame_ptr_enc);
  print_encoding_base ("", eh_frame_ptr_enc);
  printf (" fde_count_enc:    %#x ", fde_count_enc);
  print_encoding_base ("", fde_count_enc);
  printf (" table_enc:        %#x ", table_enc);
  print_encoding_base ("", table_enc);

  uint64_t eh_frame_ptr = 0;
  if (eh_frame_ptr_enc != DW_EH_PE_omit)
    {
      readp = read_encoded (eh_frame_ptr_enc, readp, dataend, &eh_frame_ptr,
			    dbg);
      if (unlikely (readp == NULL))
	goto invalid_data;

      printf (" eh_frame_ptr:     %#" PRIx64, eh_frame_ptr);
      if ((eh_frame_ptr_enc & 0x70) == DW_EH_PE_pcrel)
	printf (" (offset: %#" PRIx64 ")",
		/* +4 because of the 4 byte header of the section.  */
		(uint64_t) shdr->sh_offset + 4 + eh_frame_ptr);

      putchar_unlocked ('\n');
    }

  uint64_t fde_count = 0;
  if (fde_count_enc != DW_EH_PE_omit)
    {
      readp = read_encoded (fde_count_enc, readp, dataend, &fde_count, dbg);
      if (unlikely (readp == NULL))
	goto invalid_data;

      printf (" fde_count:        %" PRIu64 "\n", fde_count);
    }

  if (fde_count == 0 || table_enc == DW_EH_PE_omit)
    return;

  puts (" Table:");

  /* Optimize for the most common case.  */
  if (table_enc == (DW_EH_PE_datarel | DW_EH_PE_sdata4))
    while (fde_count > 0 && readp + 8 <= dataend)
      {
	int32_t initial_location = read_4sbyte_unaligned_inc (dbg, readp);
	uint64_t initial_offset = ((uint64_t) shdr->sh_offset
				   + (int64_t) initial_location);
	int32_t address = read_4sbyte_unaligned_inc (dbg, readp);
	// XXX Possibly print symbol name or section offset for initial_offset
	printf ("  %#" PRIx32 " (offset: %#6" PRIx64 ") -> %#" PRIx32
		" fde=[%6" PRIx64 "]\n",
		initial_location, initial_offset,
		address, address - (eh_frame_ptr + 4));
      }
  else
    while (0 && readp < dataend)
      {

      }
}


/* Print the content of the exception handling table section
   '.eh_frame_hdr'.  */
static void
print_debug_exception_table (Dwfl_Module *dwflmod __attribute__ ((unused)),
			     Ebl *ebl __attribute__ ((unused)),
			     GElf_Ehdr *ehdr __attribute__ ((unused)),
			     Elf_Scn *scn,
			     GElf_Shdr *shdr __attribute__ ((unused)),
			     Dwarf *dbg __attribute__ ((unused)))
{
  printf (gettext ("\
\nException handling table section [%2zu] '.gcc_except_table':\n"),
	  elf_ndxscn (scn));

  Elf_Data *data = elf_rawdata (scn, NULL);

  if (unlikely (data == NULL))
    {
      error (0, 0, gettext ("cannot get %s content: %s"),
	     ".gcc_except_table", elf_errmsg (-1));
      return;
    }

  const unsigned char *readp = data->d_buf;
  const unsigned char *const dataend = readp + data->d_size;

  if (unlikely (readp + 1 > dataend))
    {
    invalid_data:
      error (0, 0, gettext ("invalid data"));
      return;
    }
  unsigned int lpstart_encoding = *readp++;
  printf (gettext (" LPStart encoding:    %#x "), lpstart_encoding);
  print_encoding_base ("", lpstart_encoding);
  if (lpstart_encoding != DW_EH_PE_omit)
    {
      uint64_t lpstart;
      readp = read_encoded (lpstart_encoding, readp, dataend, &lpstart, dbg);
      printf (" LPStart:             %#" PRIx64 "\n", lpstart);
    }

  if (unlikely (readp + 1 > dataend))
    goto invalid_data;
  unsigned int ttype_encoding = *readp++;
  printf (gettext (" TType encoding:      %#x "), ttype_encoding);
  print_encoding_base ("", ttype_encoding);
  const unsigned char *ttype_base = NULL;
  if (ttype_encoding != DW_EH_PE_omit)
    {
      unsigned int ttype_base_offset;
      get_uleb128 (ttype_base_offset, readp);
      printf (" TType base offset:   %#x\n", ttype_base_offset);
      ttype_base = readp + ttype_base_offset;
    }

  if (unlikely (readp + 1 > dataend))
    goto invalid_data;
  unsigned int call_site_encoding = *readp++;
  printf (gettext (" Call site encoding:  %#x "), call_site_encoding);
  print_encoding_base ("", call_site_encoding);
  unsigned int call_site_table_len;
  get_uleb128 (call_site_table_len, readp);

  const unsigned char *const action_table = readp + call_site_table_len;
  if (unlikely (action_table > dataend))
    goto invalid_data;
  unsigned int u = 0;
  unsigned int max_action = 0;
  while (readp < action_table)
    {
      if (u == 0)
	puts (gettext ("\n Call site table:"));

      uint64_t call_site_start;
      readp = read_encoded (call_site_encoding, readp, dataend,
			    &call_site_start, dbg);
      uint64_t call_site_length;
      readp = read_encoded (call_site_encoding, readp, dataend,
			    &call_site_length, dbg);
      uint64_t landing_pad;
      readp = read_encoded (call_site_encoding, readp, dataend,
			    &landing_pad, dbg);
      unsigned int action;
      get_uleb128 (action, readp);
      max_action = MAX (action, max_action);
      printf (gettext (" [%4u] Call site start:   %#" PRIx64 "\n"
		       "        Call site length:  %" PRIu64 "\n"
		       "        Landing pad:       %#" PRIx64 "\n"
		       "        Action:            %u\n"),
	      u++, call_site_start, call_site_length, landing_pad, action);
    }
  assert (readp == action_table);

  unsigned int max_ar_filter = 0;
  if (max_action > 0)
    {
      puts ("\n Action table:");

      const unsigned char *const action_table_end
	= action_table + max_action + 1;

      u = 0;
      do
	{
	  int ar_filter;
	  get_sleb128 (ar_filter, readp);
	  if (ar_filter > 0 && (unsigned int) ar_filter > max_ar_filter)
	    max_ar_filter = ar_filter;
	  int ar_disp;
	  get_sleb128 (ar_disp, readp);

	  printf (" [%4u] ar_filter:  % d\n"
		  "        ar_disp:    % -5d",
		  u, ar_filter, ar_disp);
	  if (abs (ar_disp) & 1)
	    printf (" -> [%4u]\n", u + (ar_disp + 1) / 2);
	  else if (ar_disp != 0)
	    puts (" -> ???");
	  else
	    putchar_unlocked ('\n');
	  ++u;
	}
      while (readp < action_table_end);
    }

  if (max_ar_filter > 0)
    {
      puts ("\n TType table:");

      // XXX Not *4, size of encoding;
      switch (ttype_encoding & 7)
	{
	case DW_EH_PE_udata2:
	case DW_EH_PE_sdata2:
	  readp = ttype_base - max_ar_filter * 2;
	  break;
	case DW_EH_PE_udata4:
	case DW_EH_PE_sdata4:
	  readp = ttype_base - max_ar_filter * 4;
	  break;
	case DW_EH_PE_udata8:
	case DW_EH_PE_sdata8:
	  readp = ttype_base - max_ar_filter * 8;
	  break;
	default:
	  error (1, 0, gettext ("invalid TType encoding"));
	}

      do
	{
	  uint64_t ttype;
	  readp = read_encoded (ttype_encoding, readp, ttype_base, &ttype,
				dbg);
	  printf (" [%4u] %#" PRIx64 "\n", max_ar_filter--, ttype);
	}
      while (readp < ttype_base);
    }
}

/* Print the content of the '.gdb_index' section.
   http://sourceware.org/gdb/current/onlinedocs/gdb/Index-Section-Format.html
*/
static void
print_gdb_index_section (Dwfl_Module *dwflmod, Ebl *ebl, GElf_Ehdr *ehdr,
			 Elf_Scn *scn, GElf_Shdr *shdr, Dwarf *dbg)
{
  printf (gettext ("\nGDB section [%2zu] '%s' at offset %#" PRIx64
		   " contains %" PRId64 " bytes :\n"),
	  elf_ndxscn (scn), section_name (ebl, ehdr, shdr),
	  (uint64_t) shdr->sh_offset, (uint64_t) shdr->sh_size);

  Elf_Data *data = elf_rawdata (scn, NULL);

  if (unlikely (data == NULL))
    {
      error (0, 0, gettext ("cannot get %s content: %s"),
	     ".gdb_index", elf_errmsg (-1));
      return;
    }

  // .gdb_index is always in little endian.
  Dwarf dummy_dbg = { .other_byte_order = MY_ELFDATA != ELFDATA2LSB };
  dbg = &dummy_dbg;

  const unsigned char *readp = data->d_buf;
  const unsigned char *const dataend = readp + data->d_size;

  if (unlikely (readp + 4 > dataend))
    {
    invalid_data:
      error (0, 0, gettext ("invalid data"));
      return;
    }

  int32_t vers = read_4ubyte_unaligned (dbg, readp);
  printf (gettext (" Version:         %" PRId32 "\n"), vers);

  // The only difference between version 4 and version 5 is the
  // hash used for generating the table.  Version 6 contains symbols
  // for inlined functions, older versions didn't.  Version 7 adds
  // symbol kinds.  Version 8 just indicates that it correctly includes
  // TUs for symbols.
  if (vers < 4 || vers > 8)
    {
      printf (gettext ("  unknown version, cannot parse section\n"));
      return;
    }

  readp += 4;
  if (unlikely (readp + 4 > dataend))
    goto invalid_data;

  uint32_t cu_off = read_4ubyte_unaligned (dbg, readp);
  printf (gettext (" CU offset:       %#" PRIx32 "\n"), cu_off);

  readp += 4;
  if (unlikely (readp + 4 > dataend))
    goto invalid_data;

  uint32_t tu_off = read_4ubyte_unaligned (dbg, readp);
  printf (gettext (" TU offset:       %#" PRIx32 "\n"), tu_off);

  readp += 4;
  if (unlikely (readp + 4 > dataend))
    goto invalid_data;

  uint32_t addr_off = read_4ubyte_unaligned (dbg, readp);
  printf (gettext (" address offset:  %#" PRIx32 "\n"), addr_off);

  readp += 4;
  if (unlikely (readp + 4 > dataend))
    goto invalid_data;

  uint32_t sym_off = read_4ubyte_unaligned (dbg, readp);
  printf (gettext (" symbol offset:   %#" PRIx32 "\n"), sym_off);

  readp += 4;
  if (unlikely (readp + 4 > dataend))
    goto invalid_data;

  uint32_t const_off = read_4ubyte_unaligned (dbg, readp);
  printf (gettext (" constant offset: %#" PRIx32 "\n"), const_off);

  readp = data->d_buf + cu_off;

  const unsigned char *nextp = data->d_buf + tu_off;
  size_t cu_nr = (nextp - readp) / 16;

  printf (gettext ("\n CU list at offset %#" PRIx32
		   " contains %zu entries:\n"),
	  cu_off, cu_nr);

  size_t n = 0;
  while (readp + 16 <= dataend && n < cu_nr)
    {
      uint64_t off = read_8ubyte_unaligned (dbg, readp);
      readp += 8;

      uint64_t len = read_8ubyte_unaligned (dbg, readp);
      readp += 8;

      printf (" [%4zu] start: %0#8" PRIx64
	      ", length: %5" PRIu64 "\n", n, off, len);
      n++;
    }

  readp = data->d_buf + tu_off;
  nextp = data->d_buf + addr_off;
  size_t tu_nr = (nextp - readp) / 24;

  printf (gettext ("\n TU list at offset %#" PRIx32
		   " contains %zu entries:\n"),
	  tu_off, tu_nr);

  n = 0;
  while (readp + 24 <= dataend && n < tu_nr)
    {
      uint64_t off = read_8ubyte_unaligned (dbg, readp);
      readp += 8;

      uint64_t type = read_8ubyte_unaligned (dbg, readp);
      readp += 8;

      uint64_t sig = read_8ubyte_unaligned (dbg, readp);
      readp += 8;

      printf (" [%4zu] CU offset: %5" PRId64
	      ", type offset: %5" PRId64
	      ", signature: %0#8" PRIx64 "\n", n, off, type, sig);
      n++;
    }

  readp = data->d_buf + addr_off;
  nextp = data->d_buf + sym_off;
  size_t addr_nr = (nextp - readp) / 20;

  printf (gettext ("\n Address list at offset %#" PRIx32
		   " contains %zu entries:\n"),
	  addr_off, addr_nr);

  n = 0;
  while (readp + 20 <= dataend && n < addr_nr)
    {
      uint64_t low = read_8ubyte_unaligned (dbg, readp);
      readp += 8;

      uint64_t high = read_8ubyte_unaligned (dbg, readp);
      readp += 8;

      uint32_t idx = read_4ubyte_unaligned (dbg, readp);
      readp += 4;

      char *l = format_dwarf_addr (dwflmod, 8, low, low);
      char *h = format_dwarf_addr (dwflmod, 8, high - 1, high);
      printf (" [%4zu] %s..%s, CU index: %5" PRId32 "\n",
	      n, l, h, idx);
      free (l);
      free (h);
      n++;
    }

  readp = data->d_buf + sym_off;
  nextp = data->d_buf + const_off;
  size_t sym_nr = (nextp - readp) / 8;

  printf (gettext ("\n Symbol table at offset %#" PRIx32
		   " contains %zu slots:\n"),
	  addr_off, sym_nr);

  n = 0;
  while (readp + 8 <= dataend && n < sym_nr)
    {
      uint32_t name = read_4ubyte_unaligned (dbg, readp);
      readp += 4;

      uint32_t vector = read_4ubyte_unaligned (dbg, readp);
      readp += 4;

      if (name != 0 || vector != 0)
	{
	  const unsigned char *sym = data->d_buf + const_off + name;
	  if (unlikely (sym > dataend))
	    goto invalid_data;

	  printf (" [%4zu] symbol: %s, CUs: ", n, sym);

	  const unsigned char *readcus = data->d_buf + const_off + vector;
	  if (unlikely (readcus + 8 > dataend))
	    goto invalid_data;

	  uint32_t cus = read_4ubyte_unaligned (dbg, readcus);
	  while (cus--)
	    {
	      uint32_t cu_kind, cu, kind;
	      bool is_static;
	      readcus += 4;
	      cu_kind = read_4ubyte_unaligned (dbg, readcus);
	      cu = cu_kind & ((1 << 24) - 1);
	      kind = (cu_kind >> 28) & 7;
	      is_static = cu_kind & (1 << 31);
	      if (cu > cu_nr - 1)
		printf ("%" PRId32 "T", cu - (uint32_t) cu_nr);
	      else
		printf ("%" PRId32, cu);
	      if (kind != 0)
		{
		  printf (" (");
		  switch (kind)
		    {
		    case 1:
		      printf ("type");
		      break;
		    case 2:
		      printf ("var");
		      break;
		    case 3:
		      printf ("func");
		      break;
		    case 4:
		      printf ("other");
		      break;
		    default:
		      printf ("unknown-0x%" PRIx32, kind);
		      break;
		    }
		  printf (":%c)", (is_static ? 'S' : 'G'));
		}
	      if (cus > 0)
		printf (", ");
	    }
	  printf ("\n");
	}
      n++;
    }
}

static void
print_debug (Dwfl_Module *dwflmod, Ebl *ebl, GElf_Ehdr *ehdr)
{
  /* Before we start the real work get a debug context descriptor.  */
  Dwarf_Addr dwbias;
  Dwarf *dbg = dwfl_module_getdwarf (dwflmod, &dwbias);
  Dwarf dummy_dbg =
    {
      .elf = ebl->elf,
      .other_byte_order = MY_ELFDATA != ehdr->e_ident[EI_DATA]
    };
  if (dbg == NULL)
    {
      if ((print_debug_sections & ~section_exception) != 0)
	error (0, 0, gettext ("cannot get debug context descriptor: %s"),
	       dwfl_errmsg (-1));
      if ((print_debug_sections & section_exception) == 0)
	return;
      dbg = &dummy_dbg;
    }

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (unlikely (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  /* Look through all the sections for the debugging sections to print.  */
  Elf_Scn *scn = NULL;
  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (shdr != NULL && shdr->sh_type == SHT_PROGBITS)
	{
	  static const struct
	  {
	    const char *name;
	    enum section_e bitmask;
	    void (*fp) (Dwfl_Module *, Ebl *,
			GElf_Ehdr *, Elf_Scn *, GElf_Shdr *, Dwarf *);
	  } debug_sections[] =
	    {
#define NEW_SECTION(name) \
	      { ".debug_" #name, section_##name, print_debug_##name##_section }
	      NEW_SECTION (abbrev),
	      NEW_SECTION (aranges),
	      NEW_SECTION (frame),
	      NEW_SECTION (info),
	      NEW_SECTION (types),
	      NEW_SECTION (line),
	      NEW_SECTION (loc),
	      NEW_SECTION (pubnames),
	      NEW_SECTION (str),
	      NEW_SECTION (macinfo),
	      NEW_SECTION (macro),
	      NEW_SECTION (ranges),
	      { ".eh_frame", section_frame | section_exception,
		print_debug_frame_section },
	      { ".eh_frame_hdr", section_frame | section_exception,
		print_debug_frame_hdr_section },
	      { ".gcc_except_table", section_frame | section_exception,
		print_debug_exception_table },
	      { ".gdb_index", section_gdb_index, print_gdb_index_section }
	    };
	  const int ndebug_sections = (sizeof (debug_sections)
				       / sizeof (debug_sections[0]));
	  const char *name = elf_strptr (ebl->elf, shstrndx,
					 shdr->sh_name);
	  int n;

	  for (n = 0; n < ndebug_sections; ++n)
	    if (strcmp (name, debug_sections[n].name) == 0
#if USE_ZLIB
		|| (name[0] == '.' && name[1] == 'z'
		    && debug_sections[n].name[1] == 'd'
		    && strcmp (&name[2], &debug_sections[n].name[1]) == 0)
#endif
		)
	      {
		if ((print_debug_sections | implicit_debug_sections)
		    & debug_sections[n].bitmask)
		  debug_sections[n].fp (dwflmod, ebl, ehdr, scn, shdr, dbg);
		break;
	      }
	}
    }

  reset_listptr (&known_loclistptr);
  reset_listptr (&known_rangelistptr);
}


#define ITEM_INDENT		4
#define WRAP_COLUMN		75

/* Print "NAME: FORMAT", wrapping when output text would make the line
   exceed WRAP_COLUMN.  Unpadded numbers look better for the core items
   but this function is also used for registers which should be printed
   aligned.  Fortunately registers output uses fixed fields width (such
   as %11d) for the alignment.

   Line breaks should not depend on the particular values although that
   may happen in some cases of the core items.  */

static unsigned int
__attribute__ ((format (printf, 6, 7)))
print_core_item (unsigned int colno, char sep, unsigned int wrap,
		 size_t name_width, const char *name, const char *format, ...)
{
  size_t len = strlen (name);
  if (name_width < len)
    name_width = len;

  char *out;
  va_list ap;
  va_start (ap, format);
  int out_len = vasprintf (&out, format, ap);
  va_end (ap);
  if (out_len == -1)
    error (EXIT_FAILURE, 0, _("memory exhausted"));

  size_t n = name_width + sizeof ": " - 1 + out_len;

  if (colno == 0)
    {
      printf ("%*s", ITEM_INDENT, "");
      colno = ITEM_INDENT + n;
    }
  else if (colno + 2 + n < wrap)
    {
      printf ("%c ", sep);
      colno += 2 + n;
    }
  else
    {
      printf ("\n%*s", ITEM_INDENT, "");
      colno = ITEM_INDENT + n;
    }

  printf ("%s: %*s%s", name, (int) (name_width - len), "", out);

  free (out);

  return colno;
}

static const void *
convert (Elf *core, Elf_Type type, uint_fast16_t count,
	 void *value, const void *data, size_t size)
{
  Elf_Data valuedata =
    {
      .d_type = type,
      .d_buf = value,
      .d_size = size ?: gelf_fsize (core, type, count, EV_CURRENT),
      .d_version = EV_CURRENT,
    };
  Elf_Data indata =
    {
      .d_type = type,
      .d_buf = (void *) data,
      .d_size = valuedata.d_size,
      .d_version = EV_CURRENT,
    };

  Elf_Data *d = (gelf_getclass (core) == ELFCLASS32
		 ? elf32_xlatetom : elf64_xlatetom)
    (&valuedata, &indata, elf_getident (core, NULL)[EI_DATA]);
  if (d == NULL)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot convert core note data: %s"), elf_errmsg (-1));

  return data + indata.d_size;
}

typedef uint8_t GElf_Byte;

static unsigned int
handle_core_item (Elf *core, const Ebl_Core_Item *item, const void *desc,
		  unsigned int colno, size_t *repeated_size)
{
  uint_fast16_t count = item->count ?: 1;

#define TYPES								      \
  DO_TYPE (BYTE, Byte, "0x%.2" PRIx8, "%" PRId8);			      \
  DO_TYPE (HALF, Half, "0x%.4" PRIx16, "%" PRId16);			      \
  DO_TYPE (WORD, Word, "0x%.8" PRIx32, "%" PRId32);			      \
  DO_TYPE (SWORD, Sword, "%" PRId32, "%" PRId32);			      \
  DO_TYPE (XWORD, Xword, "0x%.16" PRIx64, "%" PRId64);			      \
  DO_TYPE (SXWORD, Sxword, "%" PRId64, "%" PRId64)

#define DO_TYPE(NAME, Name, hex, dec) GElf_##Name Name[count]
  union { TYPES; } value;
#undef DO_TYPE

  void *data = &value;
  size_t size = gelf_fsize (core, item->type, count, EV_CURRENT);
  size_t convsize = size;
  if (repeated_size != NULL)
    {
      if (*repeated_size > size && (item->format == 'b' || item->format == 'B'))
	{
	  data = alloca (*repeated_size);
	  count *= *repeated_size / size;
	  convsize = count * size;
	  *repeated_size -= convsize;
	}
      else if (item->count != 0 || item->format != '\n')
	*repeated_size -= size;
    }

  convert (core, item->type, count, data, desc + item->offset, convsize);

  Elf_Type type = item->type;
  if (type == ELF_T_ADDR)
    type = gelf_getclass (core) == ELFCLASS32 ? ELF_T_WORD : ELF_T_XWORD;

  switch (item->format)
    {
    case 'd':
      assert (count == 1);
      switch (type)
	{
#define DO_TYPE(NAME, Name, hex, dec)					      \
	  case ELF_T_##NAME:						      \
	    colno = print_core_item (colno, ',', WRAP_COLUMN,		      \
				     0, item->name, dec, value.Name[0]); \
	    break
	  TYPES;
#undef DO_TYPE
	default:
	  abort ();
	}
      break;

    case 'x':
      assert (count == 1);
      switch (type)
	{
#define DO_TYPE(NAME, Name, hex, dec)					      \
	  case ELF_T_##NAME:						      \
	    colno = print_core_item (colno, ',', WRAP_COLUMN,		      \
				     0, item->name, hex, value.Name[0]);      \
	    break
	  TYPES;
#undef DO_TYPE
	default:
	  abort ();
	}
      break;

    case 'b':
    case 'B':
      assert (size % sizeof (unsigned int) == 0);
      unsigned int nbits = count * size * 8;
      unsigned int pop = 0;
      for (const unsigned int *i = data; (void *) i < data + count * size; ++i)
	pop += __builtin_popcount (*i);
      bool negate = pop > nbits / 2;
      const unsigned int bias = item->format == 'b';

      {
	char printed[(negate ? nbits - pop : pop) * 16];
	char *p = printed;
	*p = '\0';

	if (BYTE_ORDER != LITTLE_ENDIAN && size > sizeof (unsigned int))
	  {
	    assert (size == sizeof (unsigned int) * 2);
	    for (unsigned int *i = data;
		 (void *) i < data + count * size; i += 2)
	      {
		unsigned int w = i[1];
		i[1] = i[0];
		i[0] = w;
	      }
	  }

	unsigned int lastbit = 0;
	unsigned int run = 0;
	for (const unsigned int *i = data;
	     (void *) i < data + count * size; ++i)
	  {
	    unsigned int bit = ((void *) i - data) * 8;
	    unsigned int w = negate ? ~*i : *i;
	    while (w != 0)
	      {
		int n = ffs (w);
		w >>= n;
		bit += n;

		if (lastbit != 0 && lastbit + 1 == bit)
		  ++run;
		else
		  {
		    if (lastbit == 0)
		      p += sprintf (p, "%u", bit - bias);
		    else if (run == 0)
		      p += sprintf (p, ",%u", bit - bias);
		    else
		      p += sprintf (p, "-%u,%u", lastbit - bias, bit - bias);
		    run = 0;
		  }

		lastbit = bit;
	      }
	  }
	if (lastbit > 0 && run > 0 && lastbit + 1 != nbits)
	  p += sprintf (p, "-%u", lastbit - bias);

	colno = print_core_item (colno, ',', WRAP_COLUMN, 0, item->name,
				 negate ? "~<%s>" : "<%s>", printed);
      }
      break;

    case 'T':
    case (char) ('T'|0x80):
      assert (count == 2);
      Dwarf_Word sec;
      Dwarf_Word usec;
      switch (type)
	{
#define DO_TYPE(NAME, Name, hex, dec)					      \
	  case ELF_T_##NAME:						      \
	    sec = value.Name[0];					      \
	    usec = value.Name[1];					      \
	    break
	  TYPES;
#undef DO_TYPE
	default:
	  abort ();
	}
      if (unlikely (item->format == (char) ('T'|0x80)))
	{
	  /* This is a hack for an ill-considered 64-bit ABI where
	     tv_usec is actually a 32-bit field with 32 bits of padding
	     rounding out struct timeval.  We've already converted it as
	     a 64-bit field.  For little-endian, this just means the
	     high half is the padding; it's presumably zero, but should
	     be ignored anyway.  For big-endian, it means the 32-bit
	     field went into the high half of USEC.  */
	  GElf_Ehdr ehdr_mem;
	  GElf_Ehdr *ehdr = gelf_getehdr (core, &ehdr_mem);
	  if (likely (ehdr->e_ident[EI_DATA] == ELFDATA2MSB))
	    usec >>= 32;
	  else
	    usec &= UINT32_MAX;
	}
      colno = print_core_item (colno, ',', WRAP_COLUMN, 0, item->name,
			       "%" PRIu64 ".%.6" PRIu64, sec, usec);
      break;

    case 'c':
      assert (count == 1);
      colno = print_core_item (colno, ',', WRAP_COLUMN, 0, item->name,
			       "%c", value.Byte[0]);
      break;

    case 's':
      colno = print_core_item (colno, ',', WRAP_COLUMN, 0, item->name,
			       "%.*s", (int) count, value.Byte);
      break;

    case '\n':
      /* This is a list of strings separated by '\n'.  */
      assert (item->count == 0);
      assert (repeated_size != NULL);
      assert (item->name == NULL);
      if (unlikely (item->offset >= *repeated_size))
	break;

      const char *s = desc + item->offset;
      size = *repeated_size - item->offset;
      *repeated_size = 0;
      while (size > 0)
	{
	  const char *eol = memchr (s, '\n', size);
	  int len = size;
	  if (eol != NULL)
	    len = eol - s;
	  printf ("%*s%.*s\n", ITEM_INDENT, "", len, s);
	  if (eol == NULL)
	    break;
	  size -= eol + 1 - s;
	  s = eol + 1;
	}

      colno = WRAP_COLUMN;
      break;

    case 'h':
      break;

    default:
      error (0, 0, "XXX not handling format '%c' for %s",
	     item->format, item->name);
      break;
    }

#undef TYPES

  return colno;
}


/* Sort items by group, and by layout offset within each group.  */
static int
compare_core_items (const void *a, const void *b)
{
  const Ebl_Core_Item *const *p1 = a;
  const Ebl_Core_Item *const *p2 = b;
  const Ebl_Core_Item *item1 = *p1;
  const Ebl_Core_Item *item2 = *p2;

  return ((item1->group == item2->group ? 0
	   : strcmp (item1->group, item2->group))
	  ?: (int) item1->offset - (int) item2->offset);
}

/* Sort item groups by layout offset of the first item in the group.  */
static int
compare_core_item_groups (const void *a, const void *b)
{
  const Ebl_Core_Item *const *const *p1 = a;
  const Ebl_Core_Item *const *const *p2 = b;
  const Ebl_Core_Item *const *group1 = *p1;
  const Ebl_Core_Item *const *group2 = *p2;
  const Ebl_Core_Item *item1 = *group1;
  const Ebl_Core_Item *item2 = *group2;

  return (int) item1->offset - (int) item2->offset;
}

static unsigned int
handle_core_items (Elf *core, const void *desc, size_t descsz,
		   const Ebl_Core_Item *items, size_t nitems)
{
  if (nitems == 0)
    return 0;
  unsigned int colno = 0;

  /* FORMAT '\n' makes sense to be present only as a single item as it
     processes all the data of a note.  FORMATs 'b' and 'B' have a special case
     if present as a single item but they can be also processed with other
     items below.  */
  if (nitems == 1 && (items[0].format == '\n' || items[0].format == 'b'
		      || items[0].format == 'B'))
    {
      assert (items[0].offset == 0);
      size_t size = descsz;
      colno = handle_core_item (core, items, desc, colno, &size);
      /* If SIZE is not zero here there is some remaining data.  But we do not
	 know how to process it anyway.  */
      return colno;
    }
  for (size_t i = 0; i < nitems; ++i)
    assert (items[i].format != '\n');

  /* Sort to collect the groups together.  */
  const Ebl_Core_Item *sorted_items[nitems];
  for (size_t i = 0; i < nitems; ++i)
    sorted_items[i] = &items[i];
  qsort (sorted_items, nitems, sizeof sorted_items[0], &compare_core_items);

  /* Collect the unique groups and sort them.  */
  const Ebl_Core_Item **groups[nitems];
  groups[0] = &sorted_items[0];
  size_t ngroups = 1;
  for (size_t i = 1; i < nitems; ++i)
    if (sorted_items[i]->group != sorted_items[i - 1]->group
	&& strcmp (sorted_items[i]->group, sorted_items[i - 1]->group))
      groups[ngroups++] = &sorted_items[i];
  qsort (groups, ngroups, sizeof groups[0], &compare_core_item_groups);

  /* Write out all the groups.  */
  const void *last = desc;
  do
    {
      for (size_t i = 0; i < ngroups; ++i)
	{
	  for (const Ebl_Core_Item **item = groups[i];
	       (item < &sorted_items[nitems]
		&& ((*item)->group == groups[i][0]->group
		    || !strcmp ((*item)->group, groups[i][0]->group)));
	       ++item)
	    colno = handle_core_item (core, *item, desc, colno, NULL);

	  /* Force a line break at the end of the group.  */
	  colno = WRAP_COLUMN;
	}

      if (descsz == 0)
	break;

      /* This set of items consumed a certain amount of the note's data.
	 If there is more data there, we have another unit of the same size.
	 Loop to print that out too.  */
      const Ebl_Core_Item *item = &items[nitems - 1];
      size_t eltsz = item->offset + gelf_fsize (core, item->type,
						item->count ?: 1, EV_CURRENT);

      int reps = -1;
      do
	{
	  ++reps;
	  desc += eltsz;
	  descsz -= eltsz;
	}
      while (descsz >= eltsz && !memcmp (desc, last, eltsz));

      if (reps == 1)
	{
	  /* For just one repeat, print it unabridged twice.  */
	  desc -= eltsz;
	  descsz += eltsz;
	}
      else if (reps > 1)
	printf (gettext ("\n%*s... <repeats %u more times> ..."),
		ITEM_INDENT, "", reps);

      last = desc;
    }
  while (descsz > 0);

  return colno;
}

static unsigned int
handle_bit_registers (const Ebl_Register_Location *regloc, const void *desc,
		      unsigned int colno)
{
  desc += regloc->offset;

  abort ();			/* XXX */
  return colno;
}


static unsigned int
handle_core_register (Ebl *ebl, Elf *core, int maxregname,
		      const Ebl_Register_Location *regloc, const void *desc,
		      unsigned int colno)
{
  if (regloc->bits % 8 != 0)
    return handle_bit_registers (regloc, desc, colno);

  desc += regloc->offset;

  for (int reg = regloc->regno; reg < regloc->regno + regloc->count; ++reg)
    {
      char name[REGNAMESZ];
      int bits;
      int type;
      register_info (ebl, reg, regloc, name, &bits, &type);

#define TYPES								      \
      BITS (8, BYTE, "%4" PRId8, "0x%.2" PRIx8);			      \
      BITS (16, HALF, "%6" PRId16, "0x%.4" PRIx16);			      \
      BITS (32, WORD, "%11" PRId32, " 0x%.8" PRIx32);			      \
      BITS (64, XWORD, "%20" PRId64, "  0x%.16" PRIx64)

#define BITS(bits, xtype, sfmt, ufmt)				\
      uint##bits##_t b##bits; int##bits##_t b##bits##s
      union { TYPES; uint64_t b128[2]; } value;
#undef	BITS

      switch (type)
	{
	case DW_ATE_unsigned:
	case DW_ATE_signed:
	case DW_ATE_address:
	  switch (bits)
	    {
#define BITS(bits, xtype, sfmt, ufmt)					      \
	    case bits:							      \
	      desc = convert (core, ELF_T_##xtype, 1, &value, desc, 0);	      \
	      if (type == DW_ATE_signed)				      \
		colno = print_core_item (colno, ' ', WRAP_COLUMN,	      \
					 maxregname, name,		      \
					 sfmt, value.b##bits##s);	      \
	      else							      \
		colno = print_core_item (colno, ' ', WRAP_COLUMN,	      \
					 maxregname, name,		      \
					 ufmt, value.b##bits);		      \
	      break

	    TYPES;

	    case 128:
	      assert (type == DW_ATE_unsigned);
	      desc = convert (core, ELF_T_XWORD, 2, &value, desc, 0);
	      int be = elf_getident (core, NULL)[EI_DATA] == ELFDATA2MSB;
	      colno = print_core_item (colno, ' ', WRAP_COLUMN,
				       maxregname, name,
				       "0x%.16" PRIx64 "%.16" PRIx64,
				       value.b128[!be], value.b128[be]);
	      break;

	    default:
	      abort ();
#undef	BITS
	    }
	  break;

	default:
	  /* Print each byte in hex, the whole thing in native byte order.  */
	  assert (bits % 8 == 0);
	  const uint8_t *bytes = desc;
	  desc += bits / 8;
	  char hex[bits / 4 + 1];
	  hex[bits / 4] = '\0';
	  int incr = 1;
	  if (elf_getident (core, NULL)[EI_DATA] == ELFDATA2LSB)
	    {
	      bytes += bits / 8 - 1;
	      incr = -1;
	    }
	  size_t idx = 0;
	  for (char *h = hex; bits > 0; bits -= 8, idx += incr)
	    {
	      *h++ = "0123456789abcdef"[bytes[idx] >> 4];
	      *h++ = "0123456789abcdef"[bytes[idx] & 0xf];
	    }
	  colno = print_core_item (colno, ' ', WRAP_COLUMN,
				   maxregname, name, "0x%s", hex);
	  break;
	}
      desc += regloc->pad;

#undef TYPES
    }

  return colno;
}


struct register_info
{
  const Ebl_Register_Location *regloc;
  const char *set;
  char name[REGNAMESZ];
  int regno;
  int bits;
  int type;
};

static int
register_bitpos (const struct register_info *r)
{
  return (r->regloc->offset * 8
	  + ((r->regno - r->regloc->regno)
	     * (r->regloc->bits + r->regloc->pad * 8)));
}

static int
compare_sets_by_info (const struct register_info *r1,
		      const struct register_info *r2)
{
  return ((int) r2->bits - (int) r1->bits
	  ?: register_bitpos (r1) - register_bitpos (r2));
}

/* Sort registers by set, and by size and layout offset within each set.  */
static int
compare_registers (const void *a, const void *b)
{
  const struct register_info *r1 = a;
  const struct register_info *r2 = b;

  /* Unused elements sort last.  */
  if (r1->regloc == NULL)
    return r2->regloc == NULL ? 0 : 1;
  if (r2->regloc == NULL)
    return -1;

  return ((r1->set == r2->set ? 0 : strcmp (r1->set, r2->set))
	  ?: compare_sets_by_info (r1, r2));
}

/* Sort register sets by layout offset of the first register in the set.  */
static int
compare_register_sets (const void *a, const void *b)
{
  const struct register_info *const *p1 = a;
  const struct register_info *const *p2 = b;
  return compare_sets_by_info (*p1, *p2);
}

static unsigned int
handle_core_registers (Ebl *ebl, Elf *core, const void *desc,
		       const Ebl_Register_Location *reglocs, size_t nregloc)
{
  if (nregloc == 0)
    return 0;

  ssize_t maxnreg = ebl_register_info (ebl, 0, NULL, 0, NULL, NULL, NULL, NULL);
  if (maxnreg <= 0)
    {
      for (size_t i = 0; i < nregloc; ++i)
	if (maxnreg < reglocs[i].regno + reglocs[i].count)
	  maxnreg = reglocs[i].regno + reglocs[i].count;
      assert (maxnreg > 0);
    }

  struct register_info regs[maxnreg];
  memset (regs, 0, sizeof regs);

  /* Sort to collect the sets together.  */
  int maxreg = 0;
  for (size_t i = 0; i < nregloc; ++i)
    for (int reg = reglocs[i].regno;
	 reg < reglocs[i].regno + reglocs[i].count;
	 ++reg)
      {
	assert (reg < maxnreg);
	if (reg > maxreg)
	  maxreg = reg;
	struct register_info *info = &regs[reg];
	info->regloc = &reglocs[i];
	info->regno = reg;
	info->set = register_info (ebl, reg, &reglocs[i],
				   info->name, &info->bits, &info->type);
      }
  qsort (regs, maxreg + 1, sizeof regs[0], &compare_registers);

  /* Collect the unique sets and sort them.  */
  inline bool same_set (const struct register_info *a,
			const struct register_info *b)
  {
    return (a < &regs[maxnreg] && a->regloc != NULL
	    && b < &regs[maxnreg] && b->regloc != NULL
	    && a->bits == b->bits
	    && (a->set == b->set || !strcmp (a->set, b->set)));
  }
  struct register_info *sets[maxreg + 1];
  sets[0] = &regs[0];
  size_t nsets = 1;
  for (int i = 1; i <= maxreg; ++i)
    if (regs[i].regloc != NULL && !same_set (&regs[i], &regs[i - 1]))
      sets[nsets++] = &regs[i];
  qsort (sets, nsets, sizeof sets[0], &compare_register_sets);

  /* Write out all the sets.  */
  unsigned int colno = 0;
  for (size_t i = 0; i < nsets; ++i)
    {
      /* Find the longest name of a register in this set.  */
      size_t maxname = 0;
      const struct register_info *end;
      for (end = sets[i]; same_set (sets[i], end); ++end)
	{
	  size_t len = strlen (end->name);
	  if (len > maxname)
	    maxname = len;
	}

      for (const struct register_info *reg = sets[i];
	   reg < end;
	   reg += reg->regloc->count ?: 1)
	colno = handle_core_register (ebl, core, maxname,
				      reg->regloc, desc, colno);

      /* Force a line break at the end of the group.  */
      colno = WRAP_COLUMN;
    }

  return colno;
}

static void
handle_auxv_note (Ebl *ebl, Elf *core, GElf_Word descsz, GElf_Off desc_pos)
{
  Elf_Data *data = elf_getdata_rawchunk (core, desc_pos, descsz, ELF_T_AUXV);
  if (data == NULL)
  elf_error:
    error (EXIT_FAILURE, 0,
	   gettext ("cannot convert core note data: %s"), elf_errmsg (-1));

  const size_t nauxv = descsz / gelf_fsize (core, ELF_T_AUXV, 1, EV_CURRENT);
  for (size_t i = 0; i < nauxv; ++i)
    {
      GElf_auxv_t av_mem;
      GElf_auxv_t *av = gelf_getauxv (data, i, &av_mem);
      if (av == NULL)
	goto elf_error;

      const char *name;
      const char *fmt;
      if (ebl_auxv_info (ebl, av->a_type, &name, &fmt) == 0)
	{
	  /* Unknown type.  */
	  if (av->a_un.a_val == 0)
	    printf ("    %" PRIu64 "\n", av->a_type);
	  else
	    printf ("    %" PRIu64 ": %#" PRIx64 "\n",
		    av->a_type, av->a_un.a_val);
	}
      else
	switch (fmt[0])
	  {
	  case '\0':		/* Normally zero.  */
	    if (av->a_un.a_val == 0)
	      {
		printf ("    %s\n", name);
		break;
	      }
	    /* Fall through */
	  case 'x':		/* hex */
	  case 'p':		/* address */
	  case 's':		/* address of string */
	    printf ("    %s: %#" PRIx64 "\n", name, av->a_un.a_val);
	    break;
	  case 'u':
	    printf ("    %s: %" PRIu64 "\n", name, av->a_un.a_val);
	    break;
	  case 'd':
	    printf ("    %s: %" PRId64 "\n", name, av->a_un.a_val);
	    break;

	  case 'b':
	    printf ("    %s: %#" PRIx64 "  ", name, av->a_un.a_val);
	    GElf_Xword bit = 1;
	    const char *pfx = "<";
	    for (const char *p = fmt + 1; *p != 0; p = strchr (p, '\0') + 1)
	      {
		if (av->a_un.a_val & bit)
		  {
		    printf ("%s%s", pfx, p);
		    pfx = " ";
		  }
		bit <<= 1;
	      }
	    printf (">\n");
	    break;

	  default:
	    abort ();
	  }
    }
}

static bool
buf_has_data (unsigned char const *ptr, unsigned char const *end, size_t sz)
{
  return ptr < end && (size_t) (end - ptr) >= sz;
}

static bool
buf_read_int (Elf *core, unsigned char const **ptrp, unsigned char const *end,
	      int *retp)
{
  if (! buf_has_data (*ptrp, end, 4))
    return false;

  *ptrp = convert (core, ELF_T_WORD, 1, retp, *ptrp, 4);
  return true;
}

static bool
buf_read_ulong (Elf *core, unsigned char const **ptrp, unsigned char const *end,
		uint64_t *retp)
{
  size_t sz = gelf_fsize (core, ELF_T_ADDR, 1, EV_CURRENT);
  if (! buf_has_data (*ptrp, end, sz))
    return false;

  union
  {
    uint64_t u64;
    uint32_t u32;
  } u;

  *ptrp = convert (core, ELF_T_ADDR, 1, &u, *ptrp, sizeof u);

  if (sz == 4)
    *retp = u.u32;
  else
    *retp = u.u64;
  return true;
}

static void
handle_siginfo_note (Elf *core, GElf_Word descsz, GElf_Off desc_pos)
{
  Elf_Data *data = elf_getdata_rawchunk (core, desc_pos, descsz, ELF_T_BYTE);
  if (data == NULL)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot convert core note data: %s"), elf_errmsg (-1));

  unsigned char const *ptr = data->d_buf;
  unsigned char const *const end = data->d_buf + data->d_size;

  /* Siginfo head is three ints: signal number, error number, origin
     code.  */
  int si_signo, si_errno, si_code;
  if (! buf_read_int (core, &ptr, end, &si_signo)
      || ! buf_read_int (core, &ptr, end, &si_errno)
      || ! buf_read_int (core, &ptr, end, &si_code))
    {
    fail:
      printf ("    Not enough data in NT_SIGINFO note.\n");
      return;
    }

  /* Next is a pointer-aligned union of structures.  On 64-bit
     machines, that implies a word of padding.  */
  if (gelf_getclass (core) == ELFCLASS64)
    ptr += 4;

  printf ("    si_signo: %d, si_errno: %d, si_code: %d\n",
	  si_signo, si_errno, si_code);

  if (si_code > 0)
    switch (si_signo)
      {
      case SIGILL:
      case SIGFPE:
      case SIGSEGV:
      case SIGBUS:
	{
	  uint64_t addr;
	  if (! buf_read_ulong (core, &ptr, end, &addr))
	    goto fail;
	  printf ("    fault address: %#" PRIx64 "\n", addr);
	  break;
	}
      default:
	;
      }
  else if (si_code == SI_USER)
    {
      int pid, uid;
      if (! buf_read_int (core, &ptr, end, &pid)
	  || ! buf_read_int (core, &ptr, end, &uid))
	goto fail;
      printf ("    sender PID: %d, sender UID: %d\n", pid, uid);
    }
}

static void
handle_file_note (Elf *core, GElf_Word descsz, GElf_Off desc_pos)
{
  Elf_Data *data = elf_getdata_rawchunk (core, desc_pos, descsz, ELF_T_BYTE);
  if (data == NULL)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot convert core note data: %s"), elf_errmsg (-1));

  unsigned char const *ptr = data->d_buf;
  unsigned char const *const end = data->d_buf + data->d_size;

  uint64_t count, page_size;
  if (! buf_read_ulong (core, &ptr, end, &count)
      || ! buf_read_ulong (core, &ptr, end, &page_size))
    {
    fail:
      printf ("    Not enough data in NT_FILE note.\n");
      return;
    }

  /* Where file names are stored.  */
  unsigned char const *const fstart
    = ptr + 3 * count * gelf_fsize (core, ELF_T_ADDR, 1, EV_CURRENT);
  char const *fptr = (char *) fstart;

  printf ("    %" PRId64 " files:\n", count);
  for (uint64_t i = 0; i < count; ++i)
    {
      uint64_t mstart, mend, moffset;
      if (! buf_read_ulong (core, &ptr, fstart, &mstart)
	  || ! buf_read_ulong (core, &ptr, fstart, &mend)
	  || ! buf_read_ulong (core, &ptr, fstart, &moffset))
	goto fail;

      const char *fnext = memchr (fptr, '\0', (char *) end - fptr);
      if (fnext == NULL)
	goto fail;

      int ct = printf ("      %08" PRIx64 "-%08" PRIx64
		       " %08" PRIx64 " %" PRId64,
		       mstart, mend, moffset * page_size, mend - mstart);
      printf ("%*s%s\n", ct > 50 ? 3 : 53 - ct, "", fptr);

      fptr = fnext + 1;
    }
}

static void
handle_core_note (Ebl *ebl, const GElf_Nhdr *nhdr,
		  const char *name, const void *desc)
{
  GElf_Word regs_offset;
  size_t nregloc;
  const Ebl_Register_Location *reglocs;
  size_t nitems;
  const Ebl_Core_Item *items;

  if (! ebl_core_note (ebl, nhdr, name,
		       &regs_offset, &nregloc, &reglocs, &nitems, &items))
    return;

  /* Pass 0 for DESCSZ when there are registers in the note,
     so that the ITEMS array does not describe the whole thing.
     For non-register notes, the actual descsz might be a multiple
     of the unit size, not just exactly the unit size.  */
  unsigned int colno = handle_core_items (ebl->elf, desc,
					  nregloc == 0 ? nhdr->n_descsz : 0,
					  items, nitems);
  if (colno != 0)
    putchar_unlocked ('\n');

  colno = handle_core_registers (ebl, ebl->elf, desc + regs_offset,
				 reglocs, nregloc);
  if (colno != 0)
    putchar_unlocked ('\n');
}

static void
handle_notes_data (Ebl *ebl, const GElf_Ehdr *ehdr,
		   GElf_Off start, Elf_Data *data)
{
  fputs_unlocked (gettext ("  Owner          Data size  Type\n"), stdout);

  if (data == NULL)
    goto bad_note;

  size_t offset = 0;
  GElf_Nhdr nhdr;
  size_t name_offset;
  size_t desc_offset;
  while (offset < data->d_size
	 && (offset = gelf_getnote (data, offset,
				    &nhdr, &name_offset, &desc_offset)) > 0)
    {
      const char *name = data->d_buf + name_offset;
      const char *desc = data->d_buf + desc_offset;

      char buf[100];
      char buf2[100];
      printf (gettext ("  %-13.*s  %9" PRId32 "  %s\n"),
	      (int) nhdr.n_namesz, name, nhdr.n_descsz,
	      ehdr->e_type == ET_CORE
	      ? ebl_core_note_type_name (ebl, nhdr.n_type,
					 buf, sizeof (buf))
	      : ebl_object_note_type_name (ebl, name, nhdr.n_type,
					   buf2, sizeof (buf2)));

      /* Filter out invalid entries.  */
      if (memchr (name, '\0', nhdr.n_namesz) != NULL
	  /* XXX For now help broken Linux kernels.  */
	  || 1)
	{
	  if (ehdr->e_type == ET_CORE)
	    {
	      if (nhdr.n_type == NT_AUXV
		  && (nhdr.n_namesz == 4 /* Broken old Linux kernels.  */
		      || (nhdr.n_namesz == 5 && name[4] == '\0'))
		  && !memcmp (name, "CORE", 4))
		handle_auxv_note (ebl, ebl->elf, nhdr.n_descsz,
				  start + desc_offset);
	      else if (nhdr.n_namesz == 5 && strcmp (name, "CORE") == 0)
		switch (nhdr.n_type)
		  {
		  case NT_SIGINFO:
		    handle_siginfo_note (ebl->elf, nhdr.n_descsz,
					 start + desc_offset);
		    break;

		  case NT_FILE:
		    handle_file_note (ebl->elf, nhdr.n_descsz,
				      start + desc_offset);
		    break;

		  default:
		    handle_core_note (ebl, &nhdr, name, desc);
		  }
	      else
		handle_core_note (ebl, &nhdr, name, desc);
	    }
	  else
	    ebl_object_note (ebl, name, nhdr.n_type, nhdr.n_descsz, desc);
	}
    }

  if (offset == data->d_size)
    return;

 bad_note:
  error (EXIT_FAILURE, 0,
	 gettext ("cannot get content of note section: %s"),
	 elf_errmsg (-1));
}

static void
handle_notes (Ebl *ebl, GElf_Ehdr *ehdr)
{
  /* If we have section headers, just look for SHT_NOTE sections.
     In a debuginfo file, the program headers are not reliable.  */
  if (shnum != 0)
    {
      /* Get the section header string table index.  */
      size_t shstrndx;
      if (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0)
	error (EXIT_FAILURE, 0,
	       gettext ("cannot get section header string table index"));

      Elf_Scn *scn = NULL;
      while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
	{
	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

	  if (shdr == NULL || shdr->sh_type != SHT_NOTE)
	    /* Not what we are looking for.  */
	    continue;

	  printf (gettext ("\
\nNote section [%2zu] '%s' of %" PRIu64 " bytes at offset %#0" PRIx64 ":\n"),
		  elf_ndxscn (scn),
		  elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
		  shdr->sh_size, shdr->sh_offset);

	  handle_notes_data (ebl, ehdr, shdr->sh_offset,
			     elf_getdata (scn, NULL));
	}
      return;
    }

  /* We have to look through the program header to find the note
     sections.  There can be more than one.  */
  for (size_t cnt = 0; cnt < phnum; ++cnt)
    {
      GElf_Phdr mem;
      GElf_Phdr *phdr = gelf_getphdr (ebl->elf, cnt, &mem);

      if (phdr == NULL || phdr->p_type != PT_NOTE)
	/* Not what we are looking for.  */
	continue;

      printf (gettext ("\
\nNote segment of %" PRIu64 " bytes at offset %#0" PRIx64 ":\n"),
	      phdr->p_filesz, phdr->p_offset);

      handle_notes_data (ebl, ehdr, phdr->p_offset,
			 elf_getdata_rawchunk (ebl->elf,
					       phdr->p_offset, phdr->p_filesz,
					       ELF_T_NHDR));
    }
}


static void
hex_dump (const uint8_t *data, size_t len)
{
  size_t pos = 0;
  while (pos < len)
    {
      printf ("  0x%08Zx ", pos);

      const size_t chunk = MIN (len - pos, 16);

      for (size_t i = 0; i < chunk; ++i)
	if (i % 4 == 3)
	  printf ("%02x ", data[pos + i]);
	else
	  printf ("%02x", data[pos + i]);

      if (chunk < 16)
	printf ("%*s", (int) ((16 - chunk) * 2 + (16 - chunk + 3) / 4), "");

      for (size_t i = 0; i < chunk; ++i)
	{
	  unsigned char b = data[pos + i];
	  printf ("%c", isprint (b) ? b : '.');
	}

      putchar ('\n');
      pos += chunk;
    }
}

static void
dump_data_section (Elf_Scn *scn, const GElf_Shdr *shdr, const char *name)
{
  if (shdr->sh_size == 0 || shdr->sh_type == SHT_NOBITS)
    printf (gettext ("\nSection [%Zu] '%s' has no data to dump.\n"),
	    elf_ndxscn (scn), name);
  else
    {
      Elf_Data *data = elf_rawdata (scn, NULL);
      if (data == NULL)
	error (0, 0, gettext ("cannot get data for section [%Zu] '%s': %s"),
	       elf_ndxscn (scn), name, elf_errmsg (-1));
      else
	{
	  printf (gettext ("\nHex dump of section [%Zu] '%s', %" PRIu64
			   " bytes at offset %#0" PRIx64 ":\n"),
		  elf_ndxscn (scn), name,
		  shdr->sh_size, shdr->sh_offset);
	  hex_dump (data->d_buf, data->d_size);
	}
    }
}

static void
print_string_section (Elf_Scn *scn, const GElf_Shdr *shdr, const char *name)
{
  if (shdr->sh_size == 0 || shdr->sh_type == SHT_NOBITS)
    printf (gettext ("\nSection [%Zu] '%s' has no strings to dump.\n"),
	    elf_ndxscn (scn), name);
  else
    {
      Elf_Data *data = elf_rawdata (scn, NULL);
      if (data == NULL)
	error (0, 0, gettext ("cannot get data for section [%Zu] '%s': %s"),
	       elf_ndxscn (scn), name, elf_errmsg (-1));
      else
	{
	  printf (gettext ("\nString section [%Zu] '%s' contains %" PRIu64
			   " bytes at offset %#0" PRIx64 ":\n"),
		  elf_ndxscn (scn), name,
		  shdr->sh_size, shdr->sh_offset);

	  const char *start = data->d_buf;
	  const char *const limit = start + data->d_size;
	  do
	    {
	      const char *end = memchr (start, '\0', limit - start);
	      const size_t pos = start - (const char *) data->d_buf;
	      if (unlikely (end == NULL))
		{
		  printf ("  [%6Zx]- %.*s\n",
			  pos, (int) (limit - start), start);
		  break;
		}
	      printf ("  [%6Zx]  %s\n", pos, start);
	      start = end + 1;
	    } while (start < limit);
	}
    }
}

static void
for_each_section_argument (Elf *elf, const struct section_argument *list,
			   void (*dump) (Elf_Scn *scn, const GElf_Shdr *shdr,
					 const char *name))
{
  /* Get the section header string table index.  */
  size_t shstrndx;
  if (elf_getshdrstrndx (elf, &shstrndx) < 0)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  for (const struct section_argument *a = list; a != NULL; a = a->next)
    {
      Elf_Scn *scn;
      GElf_Shdr shdr_mem;
      const char *name = NULL;

      char *endp = NULL;
      unsigned long int shndx = strtoul (a->arg, &endp, 0);
      if (endp != a->arg && *endp == '\0')
	{
	  scn = elf_getscn (elf, shndx);
	  if (scn == NULL)
	    {
	      error (0, 0, gettext ("\nsection [%lu] does not exist"), shndx);
	      continue;
	    }

	  if (gelf_getshdr (scn, &shdr_mem) == NULL)
	    error (EXIT_FAILURE, 0, gettext ("cannot get section header: %s"),
		   elf_errmsg (-1));
	  name = elf_strptr (elf, shstrndx, shdr_mem.sh_name);
	}
      else
	{
	  /* Need to look up the section by name.  */
	  scn = NULL;
	  bool found = false;
	  while ((scn = elf_nextscn (elf, scn)) != NULL)
	    {
	      if (gelf_getshdr (scn, &shdr_mem) == NULL)
		continue;
	      name = elf_strptr (elf, shstrndx, shdr_mem.sh_name);
	      if (name == NULL)
		continue;
	      if (!strcmp (name, a->arg))
		{
		  found = true;
		  (*dump) (scn, &shdr_mem, name);
		}
	    }

	  if (unlikely (!found) && !a->implicit)
	    error (0, 0, gettext ("\nsection '%s' does not exist"), a->arg);
	}
    }
}

static void
dump_data (Ebl *ebl)
{
  for_each_section_argument (ebl->elf, dump_data_sections, &dump_data_section);
}

static void
dump_strings (Ebl *ebl)
{
  for_each_section_argument (ebl->elf, string_sections, &print_string_section);
}

static void
print_strings (Ebl *ebl)
{
  /* Get the section header string table index.  */
  size_t shstrndx;
  if (unlikely (elf_getshdrstrndx (ebl->elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  Elf_Scn *scn;
  GElf_Shdr shdr_mem;
  const char *name;
  scn = NULL;
  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
      if (gelf_getshdr (scn, &shdr_mem) == NULL)
	continue;

      if (shdr_mem.sh_type != SHT_PROGBITS
	  || !(shdr_mem.sh_flags & SHF_STRINGS))
	continue;

      name = elf_strptr (ebl->elf, shstrndx, shdr_mem.sh_name);
      if (name == NULL)
	continue;

      print_string_section (scn, &shdr_mem, name);
    }
}

static void
dump_archive_index (Elf *elf, const char *fname)
{
  size_t narsym;
  const Elf_Arsym *arsym = elf_getarsym (elf, &narsym);
  if (arsym == NULL)
    {
      int result = elf_errno ();
      if (unlikely (result != ELF_E_NO_INDEX))
	error (EXIT_FAILURE, 0,
	       gettext ("cannot get symbol index of archive '%s': %s"),
	       fname, elf_errmsg (result));
      else
	printf (gettext ("\nArchive '%s' has no symbol index\n"), fname);
      return;
    }

  printf (gettext ("\nIndex of archive '%s' has %Zu entries:\n"),
	  fname, narsym);

  size_t as_off = 0;
  for (const Elf_Arsym *s = arsym; s < &arsym[narsym - 1]; ++s)
    {
      if (s->as_off != as_off)
	{
	  as_off = s->as_off;

	  Elf *subelf;
	  if (unlikely (elf_rand (elf, as_off) == 0)
	      || unlikely ((subelf = elf_begin (-1, ELF_C_READ_MMAP, elf))
			   == NULL))
#if __GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 7)
	    while (1)
#endif
	      error (EXIT_FAILURE, 0,
		     gettext ("cannot extract member at offset %Zu in '%s': %s"),
		     as_off, fname, elf_errmsg (-1));

	  const Elf_Arhdr *h = elf_getarhdr (subelf);

	  printf (gettext ("Archive member '%s' contains:\n"), h->ar_name);

	  elf_end (subelf);
	}

      printf ("\t%s\n", s->as_name);
    }
}

#include "debugpred.h"
