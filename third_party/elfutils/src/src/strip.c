/* Discard section not used at runtime from object files.
   Copyright (C) 2000-2012 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2000.

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
#include <byteswap.h>
#include <endian.h>
#include <error.h>
#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>
#include <libintl.h>
#include <locale.h>
#include <mcheck.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <elf-knowledge.h>
#include <libebl.h>
#include <system.h>

typedef uint8_t GElf_Byte;

/* Name and version of program.  */
static void print_version (FILE *stream, struct argp_state *state);
ARGP_PROGRAM_VERSION_HOOK_DEF = print_version;

/* Bug report address.  */
ARGP_PROGRAM_BUG_ADDRESS_DEF = PACKAGE_BUGREPORT;


/* Values for the parameters which have no short form.  */
#define OPT_REMOVE_COMMENT	0x100
#define OPT_PERMISSIVE		0x101
#define OPT_STRIP_SECTIONS	0x102
#define OPT_RELOC_DEBUG 	0x103


/* Definitions of arguments for argp functions.  */
static const struct argp_option options[] =
{
  { NULL, 0, NULL, 0, N_("Output selection:"), 0 },
  { "output", 'o', "FILE", 0, N_("Place stripped output into FILE"), 0 },
  { NULL, 'f', "FILE", 0, N_("Extract the removed sections into FILE"), 0 },
  { NULL, 'F', "FILE", 0, N_("Embed name FILE instead of -f argument"), 0 },

  { NULL, 0, NULL, 0, N_("Output options:"), 0 },
  { "strip-all", 's', NULL, OPTION_HIDDEN, NULL, 0 },
  { "strip-debug", 'g', NULL, 0, N_("Remove all debugging symbols"), 0 },
  { NULL, 'd', NULL, OPTION_ALIAS, NULL, 0 },
  { NULL, 'S', NULL, OPTION_ALIAS, NULL, 0 },
  { "strip-sections", OPT_STRIP_SECTIONS, NULL, 0,
    N_("Remove section headers (not recommended)"), 0 },
  { "preserve-dates", 'p', NULL, 0,
    N_("Copy modified/access timestamps to the output"), 0 },
  { "reloc-debug-sections", OPT_RELOC_DEBUG, NULL, 0,
    N_("Resolve all trivial relocations between debug sections if the removed sections are placed in a debug file (only relevant for ET_REL files, operation is not reversable, needs -f)"), 0 },
  { "remove-comment", OPT_REMOVE_COMMENT, NULL, 0,
    N_("Remove .comment section"), 0 },
  { "remove-section", 'R', "SECTION", OPTION_HIDDEN, NULL, 0 },
  { "permissive", OPT_PERMISSIVE, NULL, 0,
    N_("Relax a few rules to handle slightly broken ELF files"), 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Short description of program.  */
static const char doc[] = N_("Discard symbols from object files.");

/* Strings for arguments in help texts.  */
static const char args_doc[] = N_("[FILE...]");

/* Prototype for option handler.  */
static error_t parse_opt (int key, char *arg, struct argp_state *state);

/* Data structure to communicate with argp functions.  */
static struct argp argp =
{
  options, parse_opt, args_doc, doc, NULL, NULL, NULL
};


/* Print symbols in file named FNAME.  */
static int process_file (const char *fname);

/* Handle one ELF file.  */
static int handle_elf (int fd, Elf *elf, const char *prefix,
		       const char *fname, mode_t mode, struct timeval tvp[2]);

/* Handle all files contained in the archive.  */
static int handle_ar (int fd, Elf *elf, const char *prefix, const char *fname,
		      struct timeval tvp[2]);

#define INTERNAL_ERROR(fname) \
  error (EXIT_FAILURE, 0, gettext ("%s: INTERNAL ERROR %d (%s-%s): %s"),      \
	 fname, __LINE__, PACKAGE_VERSION, __DATE__, elf_errmsg (-1))


/* Name of the output file.  */
static const char *output_fname;

/* Name of the debug output file.  */
static const char *debug_fname;

/* Name to pretend the debug output file has.  */
static const char *debug_fname_embed;

/* If true output files shall have same date as the input file.  */
static bool preserve_dates;

/* If true .comment sections will be removed.  */
static bool remove_comment;

/* If true remove all debug sections.  */
static bool remove_debug;

/* If true remove all section headers.  */
static bool remove_shdrs;

/* If true relax some ELF rules for input files.  */
static bool permissive;

/* If true perform relocations between debug sections.  */
static bool reloc_debug;


int
main (int argc, char *argv[])
{
  int remaining;
  int result = 0;

  /* Make memory leak detection possible.  */
  mtrace ();

  /* We use no threads here which can interfere with handling a stream.  */
  __fsetlocking (stdin, FSETLOCKING_BYCALLER);
  __fsetlocking (stdout, FSETLOCKING_BYCALLER);
  __fsetlocking (stderr, FSETLOCKING_BYCALLER);

  /* Set locale.  */
  setlocale (LC_ALL, "");

  /* Make sure the message catalog can be found.  */
  bindtextdomain (PACKAGE_TARNAME, LOCALEDIR);

  /* Initialize the message catalog.  */
  textdomain (PACKAGE_TARNAME);

  /* Parse and process arguments.  */
  if (argp_parse (&argp, argc, argv, 0, &remaining, NULL) != 0)
    return EXIT_FAILURE;

  if (reloc_debug && debug_fname == NULL)
    error (EXIT_FAILURE, 0,
	   gettext ("--reloc-debug-sections used without -f"));

  /* Tell the library which version we are expecting.  */
  elf_version (EV_CURRENT);

  if (remaining == argc)
    /* The user didn't specify a name so we use a.out.  */
    result = process_file ("a.out");
  else
    {
      /* If we have seen the '-o' or '-f' option there must be exactly one
	 input file.  */
      if ((output_fname != NULL || debug_fname != NULL)
	  && remaining + 1 < argc)
	error (EXIT_FAILURE, 0, gettext ("\
Only one input file allowed together with '-o' and '-f'"));

      /* Process all the remaining files.  */
      do
	result |= process_file (argv[remaining]);
      while (++remaining < argc);
    }

  return result;
}


/* Print the version information.  */
static void
print_version (FILE *stream, struct argp_state *state __attribute__ ((unused)))
{
  fprintf (stream, "strip (%s) %s\n", PACKAGE_NAME, PACKAGE_VERSION);
  fprintf (stream, gettext ("\
Copyright (C) %s Red Hat, Inc.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "2012");
  fprintf (stream, gettext ("Written by %s.\n"), "Ulrich Drepper");
}


/* Handle program arguments.  */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'f':
      if (debug_fname != NULL)
	{
	  error (0, 0, gettext ("-f option specified twice"));
	  return EINVAL;
	}
      debug_fname = arg;
      break;

    case 'F':
      if (debug_fname_embed != NULL)
	{
	  error (0, 0, gettext ("-F option specified twice"));
	  return EINVAL;
	}
      debug_fname_embed = arg;
      break;

    case 'o':
      if (output_fname != NULL)
	{
	  error (0, 0, gettext ("-o option specified twice"));
	  return EINVAL;
	}
      output_fname = arg;
      break;

    case 'p':
      preserve_dates = true;
      break;

    case OPT_RELOC_DEBUG:
      reloc_debug = true;
      break;

    case OPT_REMOVE_COMMENT:
      remove_comment = true;
      break;

    case 'R':
      if (!strcmp (arg, ".comment"))
	remove_comment = true;
      else
	{
	  argp_error (state,
		      gettext ("-R option supports only .comment section"));
	  return EINVAL;
	}
      break;

    case 'g':
    case 'd':
    case 'S':
      remove_debug = true;
      break;

    case OPT_STRIP_SECTIONS:
      remove_shdrs = true;
      break;

    case OPT_PERMISSIVE:
      permissive = true;
      break;

    case 's':			/* Ignored for compatibility.  */
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


static int
process_file (const char *fname)
{
  /* If we have to preserve the modify and access timestamps get them
     now.  We cannot use fstat() after opening the file since the open
     would change the access time.  */
  struct stat64 pre_st;
  struct timeval tv[2];
 again:
  if (preserve_dates)
    {
      if (stat64 (fname, &pre_st) != 0)
	{
	  error (0, errno, gettext ("cannot stat input file '%s'"), fname);
	  return 1;
	}

      /* If we have to preserve the timestamp, we need it in the
	 format utimes() understands.  */
      TIMESPEC_TO_TIMEVAL (&tv[0], &pre_st.st_atim);
      TIMESPEC_TO_TIMEVAL (&tv[1], &pre_st.st_mtim);
    }

  /* Open the file.  */
  int fd = open (fname, output_fname == NULL ? O_RDWR : O_RDONLY);
  if (fd == -1)
    {
      error (0, errno, gettext ("while opening '%s'"), fname);
      return 1;
    }

  /* We always use fstat() even if we called stat() before.  This is
     done to make sure the information returned by stat() is for the
     same file.  */
  struct stat64 st;
  if (fstat64 (fd, &st) != 0)
    {
      error (0, errno, gettext ("cannot stat input file '%s'"), fname);
      return 1;
    }
  /* Paranoid mode on.  */
  if (preserve_dates
      && (st.st_ino != pre_st.st_ino || st.st_dev != pre_st.st_dev))
    {
      /* We detected a race.  Try again.  */
      close (fd);
      goto again;
    }

  /* Now get the ELF descriptor.  */
  Elf *elf = elf_begin (fd, output_fname == NULL ? ELF_C_RDWR : ELF_C_READ,
			NULL);
  int result;
  switch (elf_kind (elf))
    {
    case ELF_K_ELF:
      result = handle_elf (fd, elf, NULL, fname, st.st_mode & ACCESSPERMS,
			   preserve_dates ? tv : NULL);
      break;

    case ELF_K_AR:
      /* It is not possible to strip the content of an archive direct
	 the output to a specific file.  */
      if (unlikely (output_fname != NULL || debug_fname != NULL))
	{
	  error (0, 0, gettext ("%s: cannot use -o or -f when stripping archive"),
		 fname);
	  result = 1;
	}
      else
	result = handle_ar (fd, elf, NULL, fname, preserve_dates ? tv : NULL);
      break;

    default:
      error (0, 0, gettext ("%s: File format not recognized"), fname);
      result = 1;
      break;
    }

  if (unlikely (elf_end (elf) != 0))
    INTERNAL_ERROR (fname);

  close (fd);

  return result;
}


/* Maximum size of array allocated on stack.  */
#define MAX_STACK_ALLOC	(400 * 1024)

static int
handle_elf (int fd, Elf *elf, const char *prefix, const char *fname,
	    mode_t mode, struct timeval tvp[2])
{
  size_t prefix_len = prefix == NULL ? 0 : strlen (prefix);
  size_t fname_len = strlen (fname) + 1;
  char *fullname = alloca (prefix_len + 1 + fname_len);
  char *cp = fullname;
  Elf *debugelf = NULL;
  char *tmp_debug_fname = NULL;
  int result = 0;
  size_t shdridx = 0;
  size_t shstrndx;
  struct shdr_info
  {
    Elf_Scn *scn;
    GElf_Shdr shdr;
    Elf_Data *data;
    Elf_Data *debug_data;
    const char *name;
    Elf32_Word idx;		/* Index in new file.  */
    Elf32_Word old_sh_link;	/* Original value of shdr.sh_link.  */
    Elf32_Word symtab_idx;
    Elf32_Word version_idx;
    Elf32_Word group_idx;
    Elf32_Word group_cnt;
    Elf_Scn *newscn;
    struct Ebl_Strent *se;
    Elf32_Word *newsymidx;
  } *shdr_info = NULL;
  Elf_Scn *scn;
  size_t cnt;
  size_t idx;
  bool changes;
  GElf_Ehdr newehdr_mem;
  GElf_Ehdr *newehdr;
  GElf_Ehdr debugehdr_mem;
  GElf_Ehdr *debugehdr;
  struct Ebl_Strtab *shst = NULL;
  Elf_Data debuglink_crc_data;
  bool any_symtab_changes = false;
  Elf_Data *shstrtab_data = NULL;
  void *debuglink_buf = NULL;

  /* Create the full name of the file.  */
  if (prefix != NULL)
    {
      cp = mempcpy (cp, prefix, prefix_len);
      *cp++ = ':';
    }
  memcpy (cp, fname, fname_len);

  /* If we are not replacing the input file open a new file here.  */
  if (output_fname != NULL)
    {
      fd = open (output_fname, O_RDWR | O_CREAT, mode);
      if (unlikely (fd == -1))
	{
	  error (0, errno, gettext ("cannot open '%s'"), output_fname);
	  return 1;
	}
    }

  int debug_fd = -1;

  /* Get the EBL handling.  Removing all debugging symbols with the -g
     option or resolving all relocations between debug sections with
     the --reloc-debug-sections option are currently the only reasons
     we need EBL so don't open the backend unless necessary.  */
  Ebl *ebl = NULL;
  if (remove_debug || reloc_debug)
    {
      ebl = ebl_openbackend (elf);
      if (ebl == NULL)
	{
	  error (0, errno, gettext ("cannot open EBL backend"));
	  result = 1;
	  goto fail;
	}
    }

  /* Open the additional file the debug information will be stored in.  */
  if (debug_fname != NULL)
    {
      /* Create a temporary file name.  We do not want to overwrite
	 the debug file if the file would not contain any
	 information.  */
      size_t debug_fname_len = strlen (debug_fname);
      tmp_debug_fname = (char *) alloca (debug_fname_len + sizeof (".XXXXXX"));
      strcpy (mempcpy (tmp_debug_fname, debug_fname, debug_fname_len),
	      ".XXXXXX");

      debug_fd = mkstemp (tmp_debug_fname);
      if (unlikely (debug_fd == -1))
	{
	  error (0, errno, gettext ("cannot open '%s'"), debug_fname);
	  result = 1;
	  goto fail;
	}
    }

  /* Get the information from the old file.  */
  GElf_Ehdr ehdr_mem;
  GElf_Ehdr *ehdr = gelf_getehdr (elf, &ehdr_mem);
  if (ehdr == NULL)
    INTERNAL_ERROR (fname);

  /* Get the section header string table index.  */
  if (unlikely (elf_getshdrstrndx (elf, &shstrndx) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  /* We now create a new ELF descriptor for the same file.  We
     construct it almost exactly in the same way with some information
     dropped.  */
  Elf *newelf;
  if (output_fname != NULL)
    newelf = elf_begin (fd, ELF_C_WRITE_MMAP, NULL);
  else
    newelf = elf_clone (elf, ELF_C_EMPTY);

  if (unlikely (gelf_newehdr (newelf, gelf_getclass (elf)) == 0)
      || (ehdr->e_type != ET_REL
	  && unlikely (gelf_newphdr (newelf, ehdr->e_phnum) == 0)))
    {
      error (0, 0, gettext ("cannot create new file '%s': %s"),
	     output_fname, elf_errmsg (-1));
      goto fail;
    }

  /* Copy over the old program header if needed.  */
  if (ehdr->e_type != ET_REL)
    for (cnt = 0; cnt < ehdr->e_phnum; ++cnt)
      {
	GElf_Phdr phdr_mem;
	GElf_Phdr *phdr = gelf_getphdr (elf, cnt, &phdr_mem);
	if (phdr == NULL
	    || unlikely (gelf_update_phdr (newelf, cnt, phdr) == 0))
	  INTERNAL_ERROR (fname);
      }

  if (debug_fname != NULL)
    {
      /* Also create an ELF descriptor for the debug file */
      debugelf = elf_begin (debug_fd, ELF_C_WRITE_MMAP, NULL);
      if (unlikely (gelf_newehdr (debugelf, gelf_getclass (elf)) == 0)
	  || (ehdr->e_type != ET_REL
	      && unlikely (gelf_newphdr (debugelf, ehdr->e_phnum) == 0)))
	{
	  error (0, 0, gettext ("cannot create new file '%s': %s"),
		 debug_fname, elf_errmsg (-1));
	  goto fail_close;
	}

      /* Copy over the old program header if needed.  */
      if (ehdr->e_type != ET_REL)
	for (cnt = 0; cnt < ehdr->e_phnum; ++cnt)
	  {
	    GElf_Phdr phdr_mem;
	    GElf_Phdr *phdr = gelf_getphdr (elf, cnt, &phdr_mem);
	    if (phdr == NULL
		|| unlikely (gelf_update_phdr (debugelf, cnt, phdr) == 0))
	      INTERNAL_ERROR (fname);
	  }
    }

  /* Number of sections.  */
  size_t shnum;
  if (unlikely (elf_getshdrnum (elf, &shnum) < 0))
    {
      error (0, 0, gettext ("cannot determine number of sections: %s"),
	     elf_errmsg (-1));
      goto fail_close;
    }

  /* Storage for section information.  We leave room for two more
     entries since we unconditionally create a section header string
     table.  Maybe some weird tool created an ELF file without one.
     The other one is used for the debug link section.  */
  if ((shnum + 2) * sizeof (struct shdr_info) > MAX_STACK_ALLOC)
    shdr_info = (struct shdr_info *) xcalloc (shnum + 2,
					      sizeof (struct shdr_info));
  else
    {
      shdr_info = (struct shdr_info *) alloca ((shnum + 2)
					       * sizeof (struct shdr_info));
      memset (shdr_info, '\0', (shnum + 2) * sizeof (struct shdr_info));
    }

  /* Prepare section information data structure.  */
  scn = NULL;
  cnt = 1;
  while ((scn = elf_nextscn (elf, scn)) != NULL)
    {
      /* This should always be true (i.e., there should not be any
	 holes in the numbering).  */
      assert (elf_ndxscn (scn) == cnt);

      shdr_info[cnt].scn = scn;

      /* Get the header.  */
      if (gelf_getshdr (scn, &shdr_info[cnt].shdr) == NULL)
	INTERNAL_ERROR (fname);

      /* Get the name of the section.  */
      shdr_info[cnt].name = elf_strptr (elf, shstrndx,
					shdr_info[cnt].shdr.sh_name);
      if (shdr_info[cnt].name == NULL)
	{
	  error (0, 0, gettext ("illformed file '%s'"), fname);
	  goto fail_close;
	}

      /* Mark them as present but not yet investigated.  */
      shdr_info[cnt].idx = 1;

      /* Remember the shdr.sh_link value.  */
      shdr_info[cnt].old_sh_link = shdr_info[cnt].shdr.sh_link;

      /* Sections in files other than relocatable object files which
	 are not loaded can be freely moved by us.  In relocatable
	 object files everything can be moved.  */
      if (ehdr->e_type == ET_REL
	  || (shdr_info[cnt].shdr.sh_flags & SHF_ALLOC) == 0)
	shdr_info[cnt].shdr.sh_offset = 0;

      /* If this is an extended section index table store an
	 appropriate reference.  */
      if (unlikely (shdr_info[cnt].shdr.sh_type == SHT_SYMTAB_SHNDX))
	{
	  assert (shdr_info[shdr_info[cnt].shdr.sh_link].symtab_idx == 0);
	  shdr_info[shdr_info[cnt].shdr.sh_link].symtab_idx = cnt;
	}
      else if (unlikely (shdr_info[cnt].shdr.sh_type == SHT_GROUP))
	{
	  /* Cross-reference the sections contained in the section
	     group.  */
	  shdr_info[cnt].data = elf_getdata (shdr_info[cnt].scn, NULL);
	  if (shdr_info[cnt].data == NULL)
	    INTERNAL_ERROR (fname);

	  /* XXX Fix for unaligned access.  */
	  Elf32_Word *grpref = (Elf32_Word *) shdr_info[cnt].data->d_buf;
	  size_t inner;
	  for (inner = 1;
	       inner < shdr_info[cnt].data->d_size / sizeof (Elf32_Word);
	       ++inner)
	    shdr_info[grpref[inner]].group_idx = cnt;

	  if (inner == 1 || (inner == 2 && (grpref[0] & GRP_COMDAT) == 0))
	    /* If the section group contains only one element and this
	       is n COMDAT section we can drop it right away.  */
	    shdr_info[cnt].idx = 0;
	  else
	    shdr_info[cnt].group_cnt = inner - 1;
	}
      else if (unlikely (shdr_info[cnt].shdr.sh_type == SHT_GNU_versym))
	{
	  assert (shdr_info[shdr_info[cnt].shdr.sh_link].version_idx == 0);
	  shdr_info[shdr_info[cnt].shdr.sh_link].version_idx = cnt;
	}

      /* If this section is part of a group make sure it is not
	 discarded right away.  */
      if ((shdr_info[cnt].shdr.sh_flags & SHF_GROUP) != 0)
	{
	  assert (shdr_info[cnt].group_idx != 0);

	  if (shdr_info[shdr_info[cnt].group_idx].idx == 0)
	    {
	      /* The section group section will be removed.  */
	      shdr_info[cnt].group_idx = 0;
	      shdr_info[cnt].shdr.sh_flags &= ~SHF_GROUP;
	    }
	}

      /* Increment the counter.  */
      ++cnt;
    }

  /* Now determine which sections can go away.  The general rule is that
     all sections which are not used at runtime are stripped out.  But
     there are a few exceptions:

     - special sections named ".comment" and ".note" are kept
     - OS or architecture specific sections are kept since we might not
       know how to handle them
     - if a section is referred to from a section which is not removed
       in the sh_link or sh_info element it cannot be removed either
  */
  for (cnt = 1; cnt < shnum; ++cnt)
    /* Check whether the section can be removed.  */
    if (remove_shdrs ? !(shdr_info[cnt].shdr.sh_flags & SHF_ALLOC)
	: ebl_section_strip_p (ebl, ehdr, &shdr_info[cnt].shdr,
			       shdr_info[cnt].name, remove_comment,
			       remove_debug))
      {
	/* For now assume this section will be removed.  */
	shdr_info[cnt].idx = 0;

	idx = shdr_info[cnt].group_idx;
	while (idx != 0)
	  {
	    /* The section group data is already loaded.  */
	    assert (shdr_info[idx].data != NULL);

	    /* If the references section group is a normal section
	       group and has one element remaining, or if it is an
	       empty COMDAT section group it is removed.  */
	    bool is_comdat = (((Elf32_Word *) shdr_info[idx].data->d_buf)[0]
			      & GRP_COMDAT) != 0;

	    --shdr_info[idx].group_cnt;
	    if ((!is_comdat && shdr_info[idx].group_cnt == 1)
		|| (is_comdat && shdr_info[idx].group_cnt == 0))
	      {
		shdr_info[idx].idx = 0;
		/* Continue recursively.  */
		idx = shdr_info[idx].group_idx;
	      }
	    else
	      break;
	  }
      }

  /* Mark the SHT_NULL section as handled.  */
  shdr_info[0].idx = 2;


  /* Handle exceptions: section groups and cross-references.  We might
     have to repeat this a few times since the resetting of the flag
     might propagate.  */
  do
    {
      changes = false;

      for (cnt = 1; cnt < shnum; ++cnt)
	{
	  if (shdr_info[cnt].idx == 0)
	    {
	      /* If a relocation section is marked as being removed make
		 sure the section it is relocating is removed, too.  */
	      if ((shdr_info[cnt].shdr.sh_type == SHT_REL
		   || shdr_info[cnt].shdr.sh_type == SHT_RELA)
		  && shdr_info[shdr_info[cnt].shdr.sh_info].idx != 0)
		shdr_info[cnt].idx = 1;

	      /* If a group section is marked as being removed make
		 sure all the sections it contains are being removed, too.  */
	      if (shdr_info[cnt].shdr.sh_type == SHT_GROUP)
		{
		  Elf32_Word *grpref;
		  grpref = (Elf32_Word *) shdr_info[cnt].data->d_buf;
		  for (size_t in = 1;
		       in < shdr_info[cnt].data->d_size / sizeof (Elf32_Word);
		       ++in)
		    if (shdr_info[grpref[in]].idx != 0)
		      {
			shdr_info[cnt].idx = 1;
			break;
		      }
		}
	    }

	  if (shdr_info[cnt].idx == 1)
	    {
	      /* The content of symbol tables we don't remove must not
		 reference any section which we do remove.  Otherwise
		 we cannot remove the section.  */
	      if (debug_fname != NULL
		  && shdr_info[cnt].debug_data == NULL
		  && (shdr_info[cnt].shdr.sh_type == SHT_DYNSYM
		      || shdr_info[cnt].shdr.sh_type == SHT_SYMTAB))
		{
		  /* Make sure the data is loaded.  */
		  if (shdr_info[cnt].data == NULL)
		    {
		      shdr_info[cnt].data
			= elf_getdata (shdr_info[cnt].scn, NULL);
		      if (shdr_info[cnt].data == NULL)
			INTERNAL_ERROR (fname);
		    }
		  Elf_Data *symdata = shdr_info[cnt].data;

		  /* If there is an extended section index table load it
		     as well.  */
		  if (shdr_info[cnt].symtab_idx != 0
		      && shdr_info[shdr_info[cnt].symtab_idx].data == NULL)
		    {
		      assert (shdr_info[cnt].shdr.sh_type == SHT_SYMTAB);

		      shdr_info[shdr_info[cnt].symtab_idx].data
			= elf_getdata (shdr_info[shdr_info[cnt].symtab_idx].scn,
				       NULL);
		      if (shdr_info[shdr_info[cnt].symtab_idx].data == NULL)
			INTERNAL_ERROR (fname);
		    }
		  Elf_Data *xndxdata
		    = shdr_info[shdr_info[cnt].symtab_idx].data;

		  /* Go through all symbols and make sure the section they
		     reference is not removed.  */
		  size_t elsize = gelf_fsize (elf, ELF_T_SYM, 1,
					      ehdr->e_version);

		  for (size_t inner = 0;
		       inner < shdr_info[cnt].data->d_size / elsize;
		       ++inner)
		    {
		      GElf_Sym sym_mem;
		      Elf32_Word xndx;
		      GElf_Sym *sym = gelf_getsymshndx (symdata, xndxdata,
							inner, &sym_mem,
							&xndx);
		      if (sym == NULL)
			INTERNAL_ERROR (fname);

		      size_t scnidx = sym->st_shndx;
		      if (scnidx == SHN_UNDEF || scnidx >= shnum
			  || (scnidx >= SHN_LORESERVE
			      && scnidx <= SHN_HIRESERVE
			      && scnidx != SHN_XINDEX)
			  /* Don't count in the section symbols.  */
			  || GELF_ST_TYPE (sym->st_info) == STT_SECTION)
			/* This is no section index, leave it alone.  */
			continue;
		      else if (scnidx == SHN_XINDEX)
			scnidx = xndx;

		      if (shdr_info[scnidx].idx == 0)
			/* This symbol table has a real symbol in
			   a discarded section.  So preserve the
			   original table in the debug file.  */
			shdr_info[cnt].debug_data = symdata;
		    }
		}

	      /* Cross referencing happens:
		 - for the cases the ELF specification says.  That are
		   + SHT_DYNAMIC in sh_link to string table
		   + SHT_HASH in sh_link to symbol table
		   + SHT_REL and SHT_RELA in sh_link to symbol table
		   + SHT_SYMTAB and SHT_DYNSYM in sh_link to string table
		   + SHT_GROUP in sh_link to symbol table
		   + SHT_SYMTAB_SHNDX in sh_link to symbol table
		   Other (OS or architecture-specific) sections might as
		   well use this field so we process it unconditionally.
		 - references inside section groups
		 - specially marked references in sh_info if the SHF_INFO_LINK
		 flag is set
	      */

	      if (shdr_info[shdr_info[cnt].shdr.sh_link].idx == 0)
		{
		  shdr_info[shdr_info[cnt].shdr.sh_link].idx = 1;
		  changes |= shdr_info[cnt].shdr.sh_link < cnt;
		}

	      /* Handle references through sh_info.  */
	      if (SH_INFO_LINK_P (&shdr_info[cnt].shdr)
		  && shdr_info[shdr_info[cnt].shdr.sh_info].idx == 0)
		{
		  shdr_info[shdr_info[cnt].shdr.sh_info].idx = 1;
		  changes |= shdr_info[cnt].shdr.sh_info < cnt;
		}

	      /* Mark the section as investigated.  */
	      shdr_info[cnt].idx = 2;
	    }

	  if (debug_fname != NULL
	      && (shdr_info[cnt].idx == 0 || shdr_info[cnt].debug_data != NULL))
	    {
	      /* This section is being preserved in the debug file.
		 Sections it refers to must be preserved there too.

		 In this pass we mark sections to be preserved in both
		 files by setting the .debug_data pointer to the original
		 file's .data pointer.  Below, we'll copy the section
		 contents.  */

	      inline void check_preserved (size_t i)
	      {
		if (i != 0 && shdr_info[i].idx != 0
		    && shdr_info[i].debug_data == NULL)
		  {
		    if (shdr_info[i].data == NULL)
		      shdr_info[i].data = elf_getdata (shdr_info[i].scn, NULL);
		    if (shdr_info[i].data == NULL)
		      INTERNAL_ERROR (fname);

		    shdr_info[i].debug_data = shdr_info[i].data;
		    changes |= i < cnt;
		  }
	      }

	      check_preserved (shdr_info[cnt].shdr.sh_link);
	      if (SH_INFO_LINK_P (&shdr_info[cnt].shdr))
		check_preserved (shdr_info[cnt].shdr.sh_info);
	    }
	}
    }
  while (changes);

  /* Copy the removed sections to the debug output file.
     The ones that are not removed in the stripped file are SHT_NOBITS.  */
  if (debug_fname != NULL)
    {
      for (cnt = 1; cnt < shnum; ++cnt)
	{
	  scn = elf_newscn (debugelf);
	  if (scn == NULL)
	    error (EXIT_FAILURE, 0,
		   gettext ("while generating output file: %s"),
		   elf_errmsg (-1));

	  bool discard_section = (shdr_info[cnt].idx > 0
				  && shdr_info[cnt].debug_data == NULL
				  && shdr_info[cnt].shdr.sh_type != SHT_NOTE
				  && shdr_info[cnt].shdr.sh_type != SHT_GROUP
				  && cnt != ehdr->e_shstrndx);

	  /* Set the section header in the new file.  */
	  GElf_Shdr debugshdr = shdr_info[cnt].shdr;
	  if (discard_section)
	    debugshdr.sh_type = SHT_NOBITS;

	  if (unlikely (gelf_update_shdr (scn, &debugshdr) == 0))
	    /* There cannot be any overflows.  */
	    INTERNAL_ERROR (fname);

	  /* Get the data from the old file if necessary. */
	  if (shdr_info[cnt].data == NULL)
	    {
	      shdr_info[cnt].data = elf_getdata (shdr_info[cnt].scn, NULL);
	      if (shdr_info[cnt].data == NULL)
		INTERNAL_ERROR (fname);
	    }

	  /* Set the data.  This is done by copying from the old file.  */
	  Elf_Data *debugdata = elf_newdata (scn);
	  if (debugdata == NULL)
	    INTERNAL_ERROR (fname);

	  /* Copy the structure.  This data may be modified in place
	     before we write out the file.  */
	  *debugdata = *shdr_info[cnt].data;
	  if (discard_section)
	    debugdata->d_buf = NULL;
	  else if (shdr_info[cnt].debug_data != NULL
		   || shdr_info[cnt].shdr.sh_type == SHT_GROUP)
	    {
	      /* Copy the original data before it gets modified.  */
	      shdr_info[cnt].debug_data = debugdata;
	      debugdata->d_buf = memcpy (xmalloc (debugdata->d_size),
					 debugdata->d_buf, debugdata->d_size);
	    }
	}

      /* Finish the ELF header.  Fill in the fields not handled by
	 libelf from the old file.  */
      debugehdr = gelf_getehdr (debugelf, &debugehdr_mem);
      if (debugehdr == NULL)
	INTERNAL_ERROR (fname);

      memcpy (debugehdr->e_ident, ehdr->e_ident, EI_NIDENT);
      debugehdr->e_type = ehdr->e_type;
      debugehdr->e_machine = ehdr->e_machine;
      debugehdr->e_version = ehdr->e_version;
      debugehdr->e_entry = ehdr->e_entry;
      debugehdr->e_flags = ehdr->e_flags;
      debugehdr->e_shstrndx = ehdr->e_shstrndx;

      if (unlikely (gelf_update_ehdr (debugelf, debugehdr) == 0))
	{
	  error (0, 0, gettext ("%s: error while creating ELF header: %s"),
		 debug_fname, elf_errmsg (-1));
	  result = 1;
	  goto fail_close;
	}
    }

  /* Mark the section header string table as unused, we will create
     a new one.  */
  shdr_info[shstrndx].idx = 0;

  /* We need a string table for the section headers.  */
  shst = ebl_strtabinit (true);
  if (shst == NULL)
    error (EXIT_FAILURE, errno, gettext ("while preparing output for '%s'"),
	   output_fname ?: fname);

  /* Assign new section numbers.  */
  shdr_info[0].idx = 0;
  for (cnt = idx = 1; cnt < shnum; ++cnt)
    if (shdr_info[cnt].idx > 0)
      {
	shdr_info[cnt].idx = idx++;

	/* Create a new section.  */
	shdr_info[cnt].newscn = elf_newscn (newelf);
	if (shdr_info[cnt].newscn == NULL)
	  error (EXIT_FAILURE, 0, gettext ("while generating output file: %s"),
		 elf_errmsg (-1));

	assert (elf_ndxscn (shdr_info[cnt].newscn) == shdr_info[cnt].idx);

	/* Add this name to the section header string table.  */
	shdr_info[cnt].se = ebl_strtabadd (shst, shdr_info[cnt].name, 0);
      }

  /* Test whether we are doing anything at all.  */
  if (cnt == idx)
    /* Nope, all removable sections are already gone.  */
    goto fail_close;

  /* Create the reference to the file with the debug info.  */
  if (debug_fname != NULL && !remove_shdrs)
    {
      /* Add the section header string table section name.  */
      shdr_info[cnt].se = ebl_strtabadd (shst, ".gnu_debuglink", 15);
      shdr_info[cnt].idx = idx++;

      /* Create the section header.  */
      shdr_info[cnt].shdr.sh_type = SHT_PROGBITS;
      shdr_info[cnt].shdr.sh_flags = 0;
      shdr_info[cnt].shdr.sh_addr = 0;
      shdr_info[cnt].shdr.sh_link = SHN_UNDEF;
      shdr_info[cnt].shdr.sh_info = SHN_UNDEF;
      shdr_info[cnt].shdr.sh_entsize = 0;
      shdr_info[cnt].shdr.sh_addralign = 4;
      /* We set the offset to zero here.  Before we write the ELF file the
	 field must have the correct value.  This is done in the final
	 loop over all section.  Then we have all the information needed.  */
      shdr_info[cnt].shdr.sh_offset = 0;

      /* Create the section.  */
      shdr_info[cnt].newscn = elf_newscn (newelf);
      if (shdr_info[cnt].newscn == NULL)
	error (EXIT_FAILURE, 0,
	       gettext ("while create section header section: %s"),
	       elf_errmsg (-1));
      assert (elf_ndxscn (shdr_info[cnt].newscn) == shdr_info[cnt].idx);

      shdr_info[cnt].data = elf_newdata (shdr_info[cnt].newscn);
      if (shdr_info[cnt].data == NULL)
	error (EXIT_FAILURE, 0, gettext ("cannot allocate section data: %s"),
	       elf_errmsg (-1));

      char *debug_basename = basename (debug_fname_embed ?: debug_fname);
      off_t crc_offset = strlen (debug_basename) + 1;
      /* Align to 4 byte boundary */
      crc_offset = ((crc_offset - 1) & ~3) + 4;

      shdr_info[cnt].data->d_align = 4;
      shdr_info[cnt].shdr.sh_size = shdr_info[cnt].data->d_size
	= crc_offset + 4;
      debuglink_buf = xcalloc (1, shdr_info[cnt].data->d_size);
      shdr_info[cnt].data->d_buf = debuglink_buf;

      strcpy (shdr_info[cnt].data->d_buf, debug_basename);

      /* Cache this Elf_Data describing the CRC32 word in the section.
	 We'll fill this in when we have written the debug file.  */
      debuglink_crc_data = *shdr_info[cnt].data;
      debuglink_crc_data.d_buf = ((char *) debuglink_crc_data.d_buf
				  + crc_offset);
      debuglink_crc_data.d_size = 4;

      /* One more section done.  */
      ++cnt;
    }

  /* Index of the section header table in the shdr_info array.  */
  shdridx = cnt;

  /* Add the section header string table section name.  */
  shdr_info[cnt].se = ebl_strtabadd (shst, ".shstrtab", 10);
  shdr_info[cnt].idx = idx;

  /* Create the section header.  */
  shdr_info[cnt].shdr.sh_type = SHT_STRTAB;
  shdr_info[cnt].shdr.sh_flags = 0;
  shdr_info[cnt].shdr.sh_addr = 0;
  shdr_info[cnt].shdr.sh_link = SHN_UNDEF;
  shdr_info[cnt].shdr.sh_info = SHN_UNDEF;
  shdr_info[cnt].shdr.sh_entsize = 0;
  /* We set the offset to zero here.  Before we write the ELF file the
     field must have the correct value.  This is done in the final
     loop over all section.  Then we have all the information needed.  */
  shdr_info[cnt].shdr.sh_offset = 0;
  shdr_info[cnt].shdr.sh_addralign = 1;

  /* Create the section.  */
  shdr_info[cnt].newscn = elf_newscn (newelf);
  if (shdr_info[cnt].newscn == NULL)
    error (EXIT_FAILURE, 0,
	   gettext ("while create section header section: %s"),
	   elf_errmsg (-1));
  assert (elf_ndxscn (shdr_info[cnt].newscn) == idx);

  /* Finalize the string table and fill in the correct indices in the
     section headers.  */
  shstrtab_data = elf_newdata (shdr_info[cnt].newscn);
  if (shstrtab_data == NULL)
    error (EXIT_FAILURE, 0,
	   gettext ("while create section header string table: %s"),
	   elf_errmsg (-1));
  ebl_strtabfinalize (shst, shstrtab_data);

  /* We have to set the section size.  */
  shdr_info[cnt].shdr.sh_size = shstrtab_data->d_size;

  /* Update the section information.  */
  GElf_Off lastoffset = 0;
  for (cnt = 1; cnt <= shdridx; ++cnt)
    if (shdr_info[cnt].idx > 0)
      {
	Elf_Data *newdata;

	scn = elf_getscn (newelf, shdr_info[cnt].idx);
	assert (scn != NULL);

	/* Update the name.  */
	shdr_info[cnt].shdr.sh_name = ebl_strtaboffset (shdr_info[cnt].se);

	/* Update the section header from the input file.  Some fields
	   might be section indeces which now have to be adjusted.  */
	if (shdr_info[cnt].shdr.sh_link != 0)
	  shdr_info[cnt].shdr.sh_link =
	    shdr_info[shdr_info[cnt].shdr.sh_link].idx;

	if (shdr_info[cnt].shdr.sh_type == SHT_GROUP)
	  {
	    assert (shdr_info[cnt].data != NULL);

	    Elf32_Word *grpref = (Elf32_Word *) shdr_info[cnt].data->d_buf;
	    for (size_t inner = 0;
		 inner < shdr_info[cnt].data->d_size / sizeof (Elf32_Word);
		 ++inner)
	      grpref[inner] = shdr_info[grpref[inner]].idx;
	  }

	/* Handle the SHT_REL, SHT_RELA, and SHF_INFO_LINK flag.  */
	if (SH_INFO_LINK_P (&shdr_info[cnt].shdr))
	  shdr_info[cnt].shdr.sh_info =
	    shdr_info[shdr_info[cnt].shdr.sh_info].idx;

	/* Get the data from the old file if necessary.  We already
	   created the data for the section header string table.  */
	if (cnt < shnum)
	  {
	    if (shdr_info[cnt].data == NULL)
	      {
		shdr_info[cnt].data = elf_getdata (shdr_info[cnt].scn, NULL);
		if (shdr_info[cnt].data == NULL)
		  INTERNAL_ERROR (fname);
	      }

	    /* Set the data.  This is done by copying from the old file.  */
	    newdata = elf_newdata (scn);
	    if (newdata == NULL)
	      INTERNAL_ERROR (fname);

	    /* Copy the structure.  */
	    *newdata = *shdr_info[cnt].data;

	    /* We know the size.  */
	    shdr_info[cnt].shdr.sh_size = shdr_info[cnt].data->d_size;

	    /* We have to adjust symbol tables.  The st_shndx member might
	       have to be updated.  */
	    if (shdr_info[cnt].shdr.sh_type == SHT_DYNSYM
		|| shdr_info[cnt].shdr.sh_type == SHT_SYMTAB)
	      {
		Elf_Data *versiondata = NULL;
		Elf_Data *shndxdata = NULL;

		size_t elsize = gelf_fsize (elf, ELF_T_SYM, 1,
					    ehdr->e_version);

		if (shdr_info[cnt].symtab_idx != 0)
		  {
		    assert (shdr_info[cnt].shdr.sh_type == SHT_SYMTAB_SHNDX);
		    /* This section has extended section information.
		       We have to modify that information, too.  */
		    shndxdata = elf_getdata (shdr_info[shdr_info[cnt].symtab_idx].scn,
					     NULL);

		    assert ((versiondata->d_size / sizeof (Elf32_Word))
			    >= shdr_info[cnt].data->d_size / elsize);
		  }

		if (shdr_info[cnt].version_idx != 0)
		  {
		    assert (shdr_info[cnt].shdr.sh_type == SHT_DYNSYM);
		    /* This section has associated version
		       information.  We have to modify that
		       information, too.  */
		    versiondata = elf_getdata (shdr_info[shdr_info[cnt].version_idx].scn,
					       NULL);

		    assert ((versiondata->d_size / sizeof (GElf_Versym))
			    >= shdr_info[cnt].data->d_size / elsize);
		  }

		shdr_info[cnt].newsymidx
		  = (Elf32_Word *) xcalloc (shdr_info[cnt].data->d_size
					    / elsize, sizeof (Elf32_Word));

		bool last_was_local = true;
		size_t destidx;
		size_t inner;
		for (destidx = inner = 1;
		     inner < shdr_info[cnt].data->d_size / elsize;
		     ++inner)
		  {
		    Elf32_Word sec;
		    GElf_Sym sym_mem;
		    Elf32_Word xshndx;
		    GElf_Sym *sym = gelf_getsymshndx (shdr_info[cnt].data,
						      shndxdata, inner,
						      &sym_mem, &xshndx);
		    if (sym == NULL)
		      INTERNAL_ERROR (fname);

		    if (sym->st_shndx == SHN_UNDEF
			|| (sym->st_shndx >= shnum
			    && sym->st_shndx != SHN_XINDEX))
		      {
			/* This is no section index, leave it alone
			   unless it is moved.  */
			if (destidx != inner
			    && gelf_update_symshndx (shdr_info[cnt].data,
						     shndxdata,
						     destidx, sym,
						     xshndx) == 0)
			  INTERNAL_ERROR (fname);

			shdr_info[cnt].newsymidx[inner] = destidx++;

			if (last_was_local
			    && GELF_ST_BIND (sym->st_info) != STB_LOCAL)
			  {
			    last_was_local = false;
			    shdr_info[cnt].shdr.sh_info = destidx - 1;
			  }

			continue;
		      }

		    /* Get the full section index, if necessary from the
		       XINDEX table.  */
		    if (sym->st_shndx != SHN_XINDEX)
		      sec = shdr_info[sym->st_shndx].idx;
		    else
		      {
			assert (shndxdata != NULL);

			sec = shdr_info[xshndx].idx;
		      }

		    if (sec != 0)
		      {
			GElf_Section nshndx;
			Elf32_Word nxshndx;

			if (sec < SHN_LORESERVE)
			  {
			    nshndx = sec;
			    nxshndx = 0;
			  }
			else
			  {
			    nshndx = SHN_XINDEX;
			    nxshndx = sec;
			  }

			assert (sec < SHN_LORESERVE || shndxdata != NULL);

			if ((inner != destidx || nshndx != sym->st_shndx
			     || (shndxdata != NULL && nxshndx != xshndx))
			    && (sym->st_shndx = nshndx,
				gelf_update_symshndx (shdr_info[cnt].data,
						      shndxdata,
						      destidx, sym,
						      nxshndx) == 0))
			  INTERNAL_ERROR (fname);

			shdr_info[cnt].newsymidx[inner] = destidx++;

			if (last_was_local
			    && GELF_ST_BIND (sym->st_info) != STB_LOCAL)
			  {
			    last_was_local = false;
			    shdr_info[cnt].shdr.sh_info = destidx - 1;
			  }
		      }
		    else if (debug_fname == NULL
			     || shdr_info[cnt].debug_data == NULL)
		      /* This is a section or group signature symbol
			 for a section which has been removed.  */
		      {
			size_t sidx = (sym->st_shndx != SHN_XINDEX
					? sym->st_shndx : xshndx);
			assert (GELF_ST_TYPE (sym->st_info) == STT_SECTION
				|| (shdr_info[sidx].shdr.sh_type == SHT_GROUP
				    && shdr_info[sidx].shdr.sh_info == inner));
		      }
		  }

		if (destidx != inner)
		  {
		    /* The size of the symbol table changed.  */
		    shdr_info[cnt].shdr.sh_size = newdata->d_size
		      = destidx * elsize;
		    any_symtab_changes = true;
		  }
		else
		  {
		    /* The symbol table didn't really change.  */
		    free (shdr_info[cnt].newsymidx);
		    shdr_info[cnt].newsymidx = NULL;
		  }
	      }
	  }

	/* If we have to, compute the offset of the section.  */
	if (shdr_info[cnt].shdr.sh_offset == 0)
	  shdr_info[cnt].shdr.sh_offset
	    = ((lastoffset + shdr_info[cnt].shdr.sh_addralign - 1)
	       & ~((GElf_Off) (shdr_info[cnt].shdr.sh_addralign - 1)));

	/* Set the section header in the new file.  */
	if (unlikely (gelf_update_shdr (scn, &shdr_info[cnt].shdr) == 0))
	  /* There cannot be any overflows.  */
	  INTERNAL_ERROR (fname);

	/* Remember the last section written so far.  */
	GElf_Off filesz = (shdr_info[cnt].shdr.sh_type != SHT_NOBITS
			   ? shdr_info[cnt].shdr.sh_size : 0);
	if (lastoffset < shdr_info[cnt].shdr.sh_offset + filesz)
	  lastoffset = shdr_info[cnt].shdr.sh_offset + filesz;
      }

  /* Adjust symbol references if symbol tables changed.  */
  if (any_symtab_changes)
    /* Find all relocation sections which use this symbol table.  */
    for (cnt = 1; cnt <= shdridx; ++cnt)
      {
	/* Update section headers when the data size has changed.
	   We also update the SHT_NOBITS section in the debug
	   file so that the section headers match in sh_size.  */
	inline void update_section_size (const Elf_Data *newdata)
	{
	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	  shdr->sh_size = newdata->d_size;
	  (void) gelf_update_shdr (scn, shdr);
	  if (debugelf != NULL)
	    {
	      /* libelf will use d_size to set sh_size.  */
	      Elf_Data *debugdata = elf_getdata (elf_getscn (debugelf,
							     cnt), NULL);
	      debugdata->d_size = newdata->d_size;
	    }
	}

	if (shdr_info[cnt].idx == 0 && debug_fname == NULL)
	  /* Ignore sections which are discarded.  When we are saving a
	     relocation section in a separate debug file, we must fix up
	     the symbol table references.  */
	  continue;

	const Elf32_Word symtabidx = shdr_info[cnt].old_sh_link;
	const Elf32_Word *const newsymidx = shdr_info[symtabidx].newsymidx;
	switch (shdr_info[cnt].shdr.sh_type)
	  {
	    inline bool no_symtab_updates (void)
	    {
	      /* If the symbol table hasn't changed, do not do anything.  */
	      if (shdr_info[symtabidx].newsymidx == NULL)
		return true;

	      /* If the symbol table is not discarded, but additionally
		 duplicated in the separate debug file and this section
		 is discarded, don't adjust anything.  */
	      return (shdr_info[cnt].idx == 0
		      && shdr_info[symtabidx].debug_data != NULL);
	    }

	  case SHT_REL:
	  case SHT_RELA:
	    if (no_symtab_updates ())
	      break;

	    Elf_Data *d = elf_getdata (shdr_info[cnt].idx == 0
				       ? elf_getscn (debugelf, cnt)
				       : elf_getscn (newelf,
						     shdr_info[cnt].idx),
				       NULL);
	    assert (d != NULL);
	    size_t nrels = (shdr_info[cnt].shdr.sh_size
			    / shdr_info[cnt].shdr.sh_entsize);

	    if (shdr_info[cnt].shdr.sh_type == SHT_REL)
	      for (size_t relidx = 0; relidx < nrels; ++relidx)
		{
		  GElf_Rel rel_mem;
		  if (gelf_getrel (d, relidx, &rel_mem) == NULL)
		    INTERNAL_ERROR (fname);

		  size_t symidx = GELF_R_SYM (rel_mem.r_info);
		  if (newsymidx[symidx] != symidx)
		    {
		      rel_mem.r_info
			= GELF_R_INFO (newsymidx[symidx],
				       GELF_R_TYPE (rel_mem.r_info));

		      if (gelf_update_rel (d, relidx, &rel_mem) == 0)
			INTERNAL_ERROR (fname);
		    }
		}
	    else
	      for (size_t relidx = 0; relidx < nrels; ++relidx)
		{
		  GElf_Rela rel_mem;
		  if (gelf_getrela (d, relidx, &rel_mem) == NULL)
		    INTERNAL_ERROR (fname);

		  size_t symidx = GELF_R_SYM (rel_mem.r_info);
		  if (newsymidx[symidx] != symidx)
		    {
		      rel_mem.r_info
			= GELF_R_INFO (newsymidx[symidx],
				       GELF_R_TYPE (rel_mem.r_info));

		      if (gelf_update_rela (d, relidx, &rel_mem) == 0)
			INTERNAL_ERROR (fname);
		    }
		}
	    break;

	  case SHT_HASH:
	    if (no_symtab_updates ())
	      break;

	    /* We have to recompute the hash table.  */

	    assert (shdr_info[cnt].idx > 0);

	    /* The hash section in the new file.  */
	    scn = elf_getscn (newelf, shdr_info[cnt].idx);

	    /* The symbol table data.  */
	    Elf_Data *symd = elf_getdata (elf_getscn (newelf,
						      shdr_info[symtabidx].idx),
					  NULL);
	    assert (symd != NULL);

	    /* The hash table data.  */
	    Elf_Data *hashd = elf_getdata (scn, NULL);
	    assert (hashd != NULL);

	    if (shdr_info[cnt].shdr.sh_entsize == sizeof (Elf32_Word))
	      {
		/* Sane arches first.  */
		Elf32_Word *bucket = (Elf32_Word *) hashd->d_buf;

		size_t strshndx = shdr_info[symtabidx].old_sh_link;
		size_t elsize = gelf_fsize (elf, ELF_T_SYM, 1,
					    ehdr->e_version);

		/* Adjust the nchain value.  The symbol table size
		   changed.  We keep the same size for the bucket array.  */
		bucket[1] = symd->d_size / elsize;
		Elf32_Word nbucket = bucket[0];
		bucket += 2;
		Elf32_Word *chain = bucket + nbucket;

		/* New size of the section.  */
		hashd->d_size = ((2 + symd->d_size / elsize + nbucket)
				 * sizeof (Elf32_Word));
		update_section_size (hashd);

		/* Clear the arrays.  */
		memset (bucket, '\0',
			(symd->d_size / elsize + nbucket)
			* sizeof (Elf32_Word));

		for (size_t inner = shdr_info[symtabidx].shdr.sh_info;
		     inner < symd->d_size / elsize; ++inner)
		  {
		    GElf_Sym sym_mem;
		    GElf_Sym *sym = gelf_getsym (symd, inner, &sym_mem);
		    assert (sym != NULL);

		    const char *name = elf_strptr (elf, strshndx,
						   sym->st_name);
		    assert (name != NULL);
		    size_t hidx = elf_hash (name) % nbucket;

		    if (bucket[hidx] == 0)
		      bucket[hidx] = inner;
		    else
		      {
			hidx = bucket[hidx];

			while (chain[hidx] != 0)
			  hidx = chain[hidx];

			chain[hidx] = inner;
		      }
		  }
	      }
	    else
	      {
		/* Alpha and S390 64-bit use 64-bit SHT_HASH entries.  */
		assert (shdr_info[cnt].shdr.sh_entsize
			== sizeof (Elf64_Xword));

		Elf64_Xword *bucket = (Elf64_Xword *) hashd->d_buf;

		size_t strshndx = shdr_info[symtabidx].old_sh_link;
		size_t elsize = gelf_fsize (elf, ELF_T_SYM, 1,
					    ehdr->e_version);

		/* Adjust the nchain value.  The symbol table size
		   changed.  We keep the same size for the bucket array.  */
		bucket[1] = symd->d_size / elsize;
		Elf64_Xword nbucket = bucket[0];
		bucket += 2;
		Elf64_Xword *chain = bucket + nbucket;

		/* New size of the section.  */
		hashd->d_size = ((2 + symd->d_size / elsize + nbucket)
				 * sizeof (Elf64_Xword));
		update_section_size (hashd);

		/* Clear the arrays.  */
		memset (bucket, '\0',
			(symd->d_size / elsize + nbucket)
			* sizeof (Elf64_Xword));

		for (size_t inner = shdr_info[symtabidx].shdr.sh_info;
		     inner < symd->d_size / elsize; ++inner)
		  {
		    GElf_Sym sym_mem;
		    GElf_Sym *sym = gelf_getsym (symd, inner, &sym_mem);
		    assert (sym != NULL);

		    const char *name = elf_strptr (elf, strshndx,
						   sym->st_name);
		    assert (name != NULL);
		    size_t hidx = elf_hash (name) % nbucket;

		    if (bucket[hidx] == 0)
		      bucket[hidx] = inner;
		    else
		      {
			hidx = bucket[hidx];

			while (chain[hidx] != 0)
			  hidx = chain[hidx];

			chain[hidx] = inner;
		      }
		  }
	      }
	    break;

	  case SHT_GNU_versym:
	    /* If the symbol table changed we have to adjust the entries.  */
	    if (no_symtab_updates ())
	      break;

	    assert (shdr_info[cnt].idx > 0);

	    /* The symbol version section in the new file.  */
	    scn = elf_getscn (newelf, shdr_info[cnt].idx);

	    /* The symbol table data.  */
	    symd = elf_getdata (elf_getscn (newelf, shdr_info[symtabidx].idx),
				NULL);
	    assert (symd != NULL);

	    /* The version symbol data.  */
	    Elf_Data *verd = elf_getdata (scn, NULL);
	    assert (verd != NULL);

	    /* The symbol version array.  */
	    GElf_Half *verstab = (GElf_Half *) verd->d_buf;

	    /* Walk through the list and */
	    size_t elsize = gelf_fsize (elf, verd->d_type, 1,
					ehdr->e_version);
	    for (size_t inner = 1; inner < verd->d_size / elsize; ++inner)
	      if (newsymidx[inner] != 0)
		/* Overwriting the same array works since the
		   reordering can only move entries to lower indices
		   in the array.  */
		verstab[newsymidx[inner]] = verstab[inner];

	    /* New size of the section.  */
	    verd->d_size = gelf_fsize (newelf, verd->d_type,
				       symd->d_size
				       / gelf_fsize (elf, symd->d_type, 1,
						     ehdr->e_version),
				       ehdr->e_version);
	    update_section_size (verd);
	    break;

	  case SHT_GROUP:
	    if (no_symtab_updates ())
	      break;

	    /* Yes, the symbol table changed.
	       Update the section header of the section group.  */
	    scn = elf_getscn (newelf, shdr_info[cnt].idx);
	    GElf_Shdr shdr_mem;
	    GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	    assert (shdr != NULL);

	    shdr->sh_info = newsymidx[shdr->sh_info];

	    (void) gelf_update_shdr (scn, shdr);
	    break;
	  }
      }

  /* Remove any relocations between debug sections in ET_REL
     for the debug file when requested.  These relocations are always
     zero based between the unallocated sections.  */
  if (debug_fname != NULL && reloc_debug && ehdr->e_type == ET_REL)
    {
      scn = NULL;
      cnt = 0;
      while ((scn = elf_nextscn (debugelf, scn)) != NULL)
	{
	  cnt++;
	  /* We need the actual section and header from the debugelf
	     not just the cached original in shdr_info because we
	     might want to change the size.  */
	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	  if (shdr->sh_type == SHT_REL || shdr->sh_type == SHT_RELA)
	    {
	      /* Make sure that this relocation section points to a
		 section to relocate with contents, that isn't
		 allocated and that is a debug section.  */
	      Elf_Scn *tscn = elf_getscn (debugelf, shdr->sh_info);
	      GElf_Shdr tshdr_mem;
	      GElf_Shdr *tshdr = gelf_getshdr (tscn, &tshdr_mem);
	      if (tshdr->sh_type == SHT_NOBITS
		  || tshdr->sh_size == 0
		  || (tshdr->sh_flags & SHF_ALLOC) != 0)
		continue;

	      const char *tname =  elf_strptr (debugelf, shstrndx,
					       tshdr->sh_name);
	      if (! tname || ! ebl_debugscn_p (ebl, tname))
		continue;

	      /* OK, lets relocate all trivial cross debug section
		 relocations. */
	      Elf_Data *reldata = elf_getdata (scn, NULL);
	      /* We actually wanted the rawdata, but since we already
		 accessed it earlier as elf_getdata () that won't
		 work. But debug sections are all ELF_T_BYTE, so it
		 doesn't really matter.  */
	      Elf_Data *tdata = elf_getdata (tscn, NULL);
	      if (tdata->d_type != ELF_T_BYTE)
		INTERNAL_ERROR (fname);

	      /* Pick up the symbol table and shndx table to
		 resolve relocation symbol indexes.  */
	      Elf64_Word symt = shdr->sh_link;
	      Elf_Data *symdata, *xndxdata;
	      symdata = (shdr_info[symt].debug_data
			 ?: shdr_info[symt].data);
	      xndxdata = (shdr_info[shdr_info[symt].symtab_idx].debug_data
			  ?: shdr_info[shdr_info[symt].symtab_idx].data);

	      /* Apply one relocation.  Returns true when trivial
		 relocation actually done.  */
	      bool relocate (GElf_Addr offset, const GElf_Sxword addend,
			     bool is_rela, int rtype, int symndx)
	      {
		/* R_*_NONE relocs can always just be removed.  */
		if (rtype == 0)
		  return true;

		/* We only do simple absolute relocations.  */
		Elf_Type type = ebl_reloc_simple_type (ebl, rtype);
		if (type == ELF_T_NUM)
		  return false;

		/* These are the types we can relocate.  */
#define TYPES   DO_TYPE (BYTE, Byte); DO_TYPE (HALF, Half);		\
		DO_TYPE (WORD, Word); DO_TYPE (SWORD, Sword);		\
		DO_TYPE (XWORD, Xword); DO_TYPE (SXWORD, Sxword)

		/* And only for relocations against other debug sections.  */
		GElf_Sym sym_mem;
		Elf32_Word xndx;
		GElf_Sym *sym = gelf_getsymshndx (symdata, xndxdata,
						  symndx, &sym_mem,
						  &xndx);
		Elf32_Word sec = (sym->st_shndx == SHN_XINDEX
				  ? xndx : sym->st_shndx);
		if (ebl_debugscn_p (ebl, shdr_info[sec].name))
		  {
		    size_t size;

#define DO_TYPE(NAME, Name) GElf_##Name Name;
		    union { TYPES; } tmpbuf;
#undef DO_TYPE

		    switch (type)
		      {
#define DO_TYPE(NAME, Name)				\
			case ELF_T_##NAME:		\
			  size = sizeof (GElf_##Name);	\
			  tmpbuf.Name = 0;		\
			  break;
			TYPES;
#undef DO_TYPE
		      default:
			return false;
		      }

		    if (offset > tdata->d_size
			|| tdata->d_size - offset < size)
		      error (0, 0, gettext ("bad relocation"));

		    /* When the symbol value is zero then for SHT_REL
		       sections this is all that needs to be checked.
		       The addend is contained in the original data at
		       the offset already.  So if the (section) symbol
		       address is zero and the given addend is zero
		       just remove the relocation, it isn't needed
		       anymore.  */
		    if (addend == 0 && sym->st_value == 0)
		      return true;

		    Elf_Data tmpdata =
		      {
			.d_type = type,
			.d_buf = &tmpbuf,
			.d_size = size,
			.d_version = EV_CURRENT,
		      };
		    Elf_Data rdata =
		      {
			.d_type = type,
			.d_buf = tdata->d_buf + offset,
			.d_size = size,
			.d_version = EV_CURRENT,
		      };

		    GElf_Addr value = sym->st_value;
		    if (is_rela)
		      {
			/* For SHT_RELA sections we just take the
			   given addend and add it to the value.  */
			value += addend;
		      }
		    else
		      {
			/* For SHT_REL sections we have to peek at
			   what is already in the section at the given
			   offset to get the addend.  */
			Elf_Data *d = gelf_xlatetom (debugelf, &tmpdata,
						     &rdata,
						     ehdr->e_ident[EI_DATA]);
			if (d == NULL)
			  INTERNAL_ERROR (fname);
			assert (d == &tmpdata);
		      }

		    switch (type)
		      {
#define DO_TYPE(NAME, Name)					\
			case ELF_T_##NAME:			\
			  tmpbuf.Name += (GElf_##Name) value;	\
			  break;
			TYPES;
#undef DO_TYPE
		      default:
			abort ();
		      }

		    /* Now finally put in the new value.  */
		    Elf_Data *s = gelf_xlatetof (debugelf, &rdata,
						 &tmpdata,
						 ehdr->e_ident[EI_DATA]);
		    if (s == NULL)
		      INTERNAL_ERROR (fname);
		    assert (s == &rdata);

		    return true;
		  }
		return false;
	      }

	      size_t nrels = shdr->sh_size / shdr->sh_entsize;
	      size_t next = 0;
	      if (shdr->sh_type == SHT_REL)
		for (size_t relidx = 0; relidx < nrels; ++relidx)
		  {
		    GElf_Rel rel_mem;
		    GElf_Rel *r = gelf_getrel (reldata, relidx, &rel_mem);
		    if (! relocate (r->r_offset, 0, false,
				    GELF_R_TYPE (r->r_info),
				    GELF_R_SYM (r->r_info)))
		      {
			if (relidx != next)
			  gelf_update_rel (reldata, next, r);
			++next;
		      }
		  }
	      else
		for (size_t relidx = 0; relidx < nrels; ++relidx)
		  {
		    GElf_Rela rela_mem;
		    GElf_Rela *r = gelf_getrela (reldata, relidx, &rela_mem);
		    if (! relocate (r->r_offset, r->r_addend, true,
				    GELF_R_TYPE (r->r_info),
				    GELF_R_SYM (r->r_info)))
		      {
			if (relidx != next)
			  gelf_update_rela (reldata, next, r);
			++next;
		      }
		  }

	      nrels = next;
	      shdr->sh_size = reldata->d_size = nrels * shdr->sh_entsize;
	      gelf_update_shdr (scn, shdr);
	    }
	}
    }

  /* Now that we have done all adjustments to the data,
     we can actually write out the debug file.  */
  if (debug_fname != NULL)
    {
      /* Finally write the file.  */
      if (unlikely (elf_update (debugelf, ELF_C_WRITE) == -1))
	{
	  error (0, 0, gettext ("while writing '%s': %s"),
		 debug_fname, elf_errmsg (-1));
	  result = 1;
	  goto fail_close;
	}

      /* Create the real output file.  First rename, then change the
	 mode.  */
      if (rename (tmp_debug_fname, debug_fname) != 0
	  || fchmod (debug_fd, mode) != 0)
	{
	  error (0, errno, gettext ("while creating '%s'"), debug_fname);
	  result = 1;
	  goto fail_close;
	}

      /* The temporary file does not exist anymore.  */
      tmp_debug_fname = NULL;

      if (!remove_shdrs)
	{
	  uint32_t debug_crc;
	  Elf_Data debug_crc_data =
	    {
	      .d_type = ELF_T_WORD,
	      .d_buf = &debug_crc,
	      .d_size = sizeof (debug_crc),
	      .d_version = EV_CURRENT
	    };

	  /* Compute the checksum which we will add to the executable.  */
	  if (crc32_file (debug_fd, &debug_crc) != 0)
	    {
	      error (0, errno, gettext ("\
while computing checksum for debug information"));
	      unlink (debug_fname);
	      result = 1;
	      goto fail_close;
	    }

	  /* Store it in the debuglink section data.  */
	  if (unlikely (gelf_xlatetof (newelf, &debuglink_crc_data,
				       &debug_crc_data, ehdr->e_ident[EI_DATA])
			!= &debuglink_crc_data))
	    INTERNAL_ERROR (fname);
	}
    }

  /* Finally finish the ELF header.  Fill in the fields not handled by
     libelf from the old file.  */
  newehdr = gelf_getehdr (newelf, &newehdr_mem);
  if (newehdr == NULL)
    INTERNAL_ERROR (fname);

  memcpy (newehdr->e_ident, ehdr->e_ident, EI_NIDENT);
  newehdr->e_type = ehdr->e_type;
  newehdr->e_machine = ehdr->e_machine;
  newehdr->e_version = ehdr->e_version;
  newehdr->e_entry = ehdr->e_entry;
  newehdr->e_flags = ehdr->e_flags;
  newehdr->e_phoff = ehdr->e_phoff;

  /* We need to position the section header table.  */
  const size_t offsize = gelf_fsize (elf, ELF_T_OFF, 1, EV_CURRENT);
  newehdr->e_shoff = ((shdr_info[shdridx].shdr.sh_offset
		       + shdr_info[shdridx].shdr.sh_size + offsize - 1)
		      & ~((GElf_Off) (offsize - 1)));
  newehdr->e_shentsize = gelf_fsize (elf, ELF_T_SHDR, 1, EV_CURRENT);

  /* The new section header string table index.  */
  if (likely (idx < SHN_HIRESERVE) && likely (idx != SHN_XINDEX))
    newehdr->e_shstrndx = idx;
  else
    {
      /* The index does not fit in the ELF header field.  */
      shdr_info[0].scn = elf_getscn (elf, 0);

      if (gelf_getshdr (shdr_info[0].scn, &shdr_info[0].shdr) == NULL)
	INTERNAL_ERROR (fname);

      shdr_info[0].shdr.sh_link = idx;
      (void) gelf_update_shdr (shdr_info[0].scn, &shdr_info[0].shdr);

      newehdr->e_shstrndx = SHN_XINDEX;
    }

  if (gelf_update_ehdr (newelf, newehdr) == 0)
    {
      error (0, 0, gettext ("%s: error while creating ELF header: %s"),
	     fname, elf_errmsg (-1));
      return 1;
    }

  /* We have everything from the old file.  */
  if (elf_cntl (elf, ELF_C_FDDONE) != 0)
    {
      error (0, 0, gettext ("%s: error while reading the file: %s"),
	     fname, elf_errmsg (-1));
      return 1;
    }

  /* The ELF library better follows our layout when this is not a
     relocatable object file.  */
  elf_flagelf (newelf, ELF_C_SET,
	       (ehdr->e_type != ET_REL ? ELF_F_LAYOUT : 0)
	       | (permissive ? ELF_F_PERMISSIVE : 0));

  /* Finally write the file.  */
  if (elf_update (newelf, ELF_C_WRITE) == -1)
    {
      error (0, 0, gettext ("while writing '%s': %s"),
	     fname, elf_errmsg (-1));
      result = 1;
    }

  if (remove_shdrs)
    {
      /* libelf can't cope without the section headers being properly intact.
	 So we just let it write them normally, and then we nuke them later.  */

      if (newehdr->e_ident[EI_CLASS] == ELFCLASS32)
	{
	  assert (offsetof (Elf32_Ehdr, e_shentsize) + sizeof (Elf32_Half)
		  == offsetof (Elf32_Ehdr, e_shnum));
	  assert (offsetof (Elf32_Ehdr, e_shnum) + sizeof (Elf32_Half)
		  == offsetof (Elf32_Ehdr, e_shstrndx));
	  const Elf32_Off zero_off = 0;
	  const Elf32_Half zero[3] = { 0, 0, SHN_UNDEF };
	  if (pwrite_retry (fd, &zero_off, sizeof zero_off,
			    offsetof (Elf32_Ehdr, e_shoff)) != sizeof zero_off
	      || (pwrite_retry (fd, zero, sizeof zero,
				offsetof (Elf32_Ehdr, e_shentsize))
		  != sizeof zero)
	      || ftruncate64 (fd, shdr_info[shdridx].shdr.sh_offset) < 0)
	    {
	      error (0, errno, gettext ("while writing '%s'"),
		     fname);
	      result = 1;
	    }
	}
      else
	{
	  assert (offsetof (Elf64_Ehdr, e_shentsize) + sizeof (Elf64_Half)
		  == offsetof (Elf64_Ehdr, e_shnum));
	  assert (offsetof (Elf64_Ehdr, e_shnum) + sizeof (Elf64_Half)
		  == offsetof (Elf64_Ehdr, e_shstrndx));
	  const Elf64_Off zero_off = 0;
	  const Elf64_Half zero[3] = { 0, 0, SHN_UNDEF };
	  if (pwrite_retry (fd, &zero_off, sizeof zero_off,
			    offsetof (Elf64_Ehdr, e_shoff)) != sizeof zero_off
	      || (pwrite_retry (fd, zero, sizeof zero,
				offsetof (Elf64_Ehdr, e_shentsize))
		  != sizeof zero)
	      || ftruncate64 (fd, shdr_info[shdridx].shdr.sh_offset) < 0)
	    {
	      error (0, errno, gettext ("while writing '%s'"),
		     fname);
	      result = 1;
	    }
	}
    }

 fail_close:
  if (shdr_info != NULL)
    {
      /* For some sections we might have created an table to map symbol
	 table indices.  */
      if (any_symtab_changes)
	for (cnt = 1; cnt <= shdridx; ++cnt)
	  {
	    free (shdr_info[cnt].newsymidx);
	    if (shdr_info[cnt].debug_data != NULL)
	      free (shdr_info[cnt].debug_data->d_buf);
	  }

      /* Free data we allocated for the .gnu_debuglink section. */
      free (debuglink_buf);

      /* Free the memory.  */
      if ((shnum + 2) * sizeof (struct shdr_info) > MAX_STACK_ALLOC)
	free (shdr_info);
    }

  /* Free other resources.  */
  if (shstrtab_data != NULL)
    free (shstrtab_data->d_buf);
  if (shst != NULL)
    ebl_strtabfree (shst);

  /* That was it.  Close the descriptors.  */
  if (elf_end (newelf) != 0)
    {
      error (0, 0, gettext ("error while finishing '%s': %s"), fname,
	     elf_errmsg (-1));
      result = 1;
    }

  if (debugelf != NULL && elf_end (debugelf) != 0)
    {
      error (0, 0, gettext ("error while finishing '%s': %s"), debug_fname,
	     elf_errmsg (-1));
      result = 1;
    }

 fail:
  /* Close the EBL backend.  */
  if (ebl != NULL)
    ebl_closebackend (ebl);

  /* Close debug file descriptor, if opened */
  if (debug_fd >= 0)
    {
      if (tmp_debug_fname != NULL)
	unlink (tmp_debug_fname);
      close (debug_fd);
    }

  /* If requested, preserve the timestamp.  */
  if (tvp != NULL)
    {
      if (futimes (fd, tvp) != 0)
	{
	  error (0, errno, gettext ("\
cannot set access and modification date of '%s'"),
		 output_fname ?: fname);
	  result = 1;
	}
    }

  /* Close the file descriptor if we created a new file.  */
  if (output_fname != NULL)
    close (fd);

  return result;
}


static int
handle_ar (int fd, Elf *elf, const char *prefix, const char *fname,
	   struct timeval tvp[2])
{
  size_t prefix_len = prefix == NULL ? 0 : strlen (prefix);
  size_t fname_len = strlen (fname) + 1;
  char new_prefix[prefix_len + 1 + fname_len];
  char *cp = new_prefix;

  /* Create the full name of the file.  */
  if (prefix != NULL)
    {
      cp = mempcpy (cp, prefix, prefix_len);
      *cp++ = ':';
    }
  memcpy (cp, fname, fname_len);


  /* Process all the files contained in the archive.  */
  Elf *subelf;
  Elf_Cmd cmd = ELF_C_RDWR;
  int result = 0;
  while ((subelf = elf_begin (fd, cmd, elf)) != NULL)
    {
      /* The the header for this element.  */
      Elf_Arhdr *arhdr = elf_getarhdr (subelf);

      if (elf_kind (subelf) == ELF_K_ELF)
	result |= handle_elf (fd, subelf, new_prefix, arhdr->ar_name, 0, NULL);
      else if (elf_kind (subelf) == ELF_K_AR)
	result |= handle_ar (fd, subelf, new_prefix, arhdr->ar_name, NULL);

      /* Get next archive element.  */
      cmd = elf_next (subelf);
      if (unlikely (elf_end (subelf) != 0))
	INTERNAL_ERROR (fname);
    }

  if (tvp != NULL)
    {
      if (unlikely (futimes (fd, tvp) != 0))
	{
	  error (0, errno, gettext ("\
cannot set access and modification date of '%s'"), fname);
	  result = 1;
	}
    }

  if (unlikely (close (fd) != 0))
    error (EXIT_FAILURE, errno, gettext ("while closing '%s'"), fname);

  return result;
}


#include "debugpred.h"
