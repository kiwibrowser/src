/* Copyright (C) 2001-2011 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2001.

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

#include <assert.h>
#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <gelf.h>
#include <inttypes.h>
#include <libintl.h>
#include <stdbool.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <elf-knowledge.h>
#include "ld.h"
#include "list.h"
#include <md5.h>
#include <sha1.h>
#include <system.h>


/* Header of .eh_frame_hdr section.  */
struct unw_eh_frame_hdr
{
  unsigned char version;
  unsigned char eh_frame_ptr_enc;
  unsigned char fde_count_enc;
  unsigned char table_enc;
};
#define EH_FRAME_HDR_VERSION 1


/* Prototypes for local functions.  */
static const char **ld_generic_lib_extensions (struct ld_state *)
     __attribute__ ((__const__));
static int ld_generic_file_close (struct usedfiles *fileinfo,
				  struct ld_state *statep);
static int ld_generic_file_process (int fd, struct usedfiles *fileinfo,
				    struct ld_state *statep,
				    struct usedfiles **nextp);
static void ld_generic_generate_sections (struct ld_state *statep);
static void ld_generic_create_sections (struct ld_state *statep);
static int ld_generic_flag_unresolved (struct ld_state *statep);
static int ld_generic_open_outfile (struct ld_state *statep, int machine,
				    int class, int data);
static int ld_generic_create_outfile (struct ld_state *statep);
static void ld_generic_relocate_section (struct ld_state *statep,
					 Elf_Scn *outscn,
					 struct scninfo *firstp,
					 const Elf32_Word *dblindirect);
static int ld_generic_finalize (struct ld_state *statep);
static bool ld_generic_special_section_number_p (struct ld_state *statep,
						 size_t number);
static bool ld_generic_section_type_p (struct ld_state *statep,
				       XElf_Word type);
static XElf_Xword ld_generic_dynamic_section_flags (struct ld_state *statep);
static void ld_generic_initialize_plt (struct ld_state *statep, Elf_Scn *scn);
static void ld_generic_initialize_pltrel (struct ld_state *statep,
					  Elf_Scn *scn);
static void ld_generic_initialize_got (struct ld_state *statep, Elf_Scn *scn);
static void ld_generic_initialize_gotplt (struct ld_state *statep,
					  Elf_Scn *scn);
static void ld_generic_finalize_plt (struct ld_state *statep, size_t nsym,
				     size_t nsym_dyn,
				     struct symbol **ndxtosymp);
static int ld_generic_rel_type (struct ld_state *statep);
static void ld_generic_count_relocations (struct ld_state *statep,
					  struct scninfo *scninfo);
static void ld_generic_create_relocations (struct ld_state *statep,
					   const Elf32_Word *dblindirect);

static int file_process2 (struct usedfiles *fileinfo);
static void mark_section_used (struct scninfo *scninfo, Elf32_Word shndx,
			       struct scninfo **grpscnp);


/* Map symbol index to struct symbol record.  */
static struct symbol **ndxtosym;

/* String table reference to all symbols in the symbol table.  */
static struct Ebl_Strent **symstrent;


/* Check whether file associated with FD is a DSO.  */
static bool
is_dso_p (int fd)
{
  /* We have to read the 'e_type' field.  It has the same size (16
     bits) in 32- and 64-bit ELF.  */
  XElf_Half e_type;

  return (pread (fd, &e_type, sizeof (e_type), offsetof (XElf_Ehdr, e_type))
	  == sizeof (e_type)
	  && e_type == ET_DYN);
}


/* Print the complete name of a file, including the archive it is
   contained in.  */
static int
print_file_name (FILE *s, struct usedfiles *fileinfo, int first_level,
		 int newline)
{
  int npar = 0;

  if (fileinfo->archive_file != NULL)
    {
      npar = print_file_name (s, fileinfo->archive_file, 0, 0) + 1;
      fputc_unlocked ('(', s);
      fputs_unlocked (fileinfo->rfname, s);

      if (first_level)
	while (npar-- > 0)
	  fputc_unlocked (')', s);
    }
  else
    fputs_unlocked (fileinfo->rfname, s);

  if (first_level && newline)
    fputc_unlocked ('\n', s);

  return npar;
}


/* Function to determine whether an object will be dynamically linked.  */
bool
dynamically_linked_p (void)
{
  return (ld_state.file_type == dso_file_type || ld_state.nplt > 0
	  || ld_state.ngot > 0);
}


bool
linked_from_dso_p (struct scninfo *scninfo, size_t symidx)
{
  struct usedfiles *file = scninfo->fileinfo;

  /* If this symbol is not undefined in this file it cannot come from
     a DSO.  */
  if (symidx < file->nlocalsymbols)
    return false;

  struct symbol *sym = file->symref[symidx];

  return sym->defined && sym->in_dso;
}


/* Initialize state object.  This callback function is called after the
   parameters are parsed but before any file is searched for.  */
int
ld_prepare_state (const char *emulation)
{
  /* When generating DSO we normally allow undefined symbols.  */
  ld_state.nodefs = true;

  /* To be able to detect problems we add a .comment section entry by
     default.  */
  ld_state.add_ld_comment = true;

  /* XXX We probably should find a better place for this.  The index
     of the first user-defined version is 2.  */
  ld_state.nextveridx = 2;

  /* Pick an not too small number for the initial size of the tables.  */
  ld_symbol_tab_init (&ld_state.symbol_tab, 1027);
  ld_section_tab_init (&ld_state.section_tab, 67);
  ld_version_str_tab_init (&ld_state.version_str_tab, 67);

  /* Initialize the section header string table.  */
  ld_state.shstrtab = ebl_strtabinit (true);
  if (ld_state.shstrtab == NULL)
    error (EXIT_FAILURE, errno, gettext ("cannot create string table"));

  /* Initialize the callbacks.  These are the defaults, the appropriate
     backend can later install its own callbacks.  */
  ld_state.callbacks.lib_extensions = ld_generic_lib_extensions;
  ld_state.callbacks.file_process = ld_generic_file_process;
  ld_state.callbacks.file_close = ld_generic_file_close;
  ld_state.callbacks.generate_sections = ld_generic_generate_sections;
  ld_state.callbacks.create_sections = ld_generic_create_sections;
  ld_state.callbacks.flag_unresolved = ld_generic_flag_unresolved;
  ld_state.callbacks.open_outfile = ld_generic_open_outfile;
  ld_state.callbacks.create_outfile = ld_generic_create_outfile;
  ld_state.callbacks.relocate_section = ld_generic_relocate_section;
  ld_state.callbacks.finalize = ld_generic_finalize;
  ld_state.callbacks.special_section_number_p =
    ld_generic_special_section_number_p;
  ld_state.callbacks.section_type_p = ld_generic_section_type_p;
  ld_state.callbacks.dynamic_section_flags = ld_generic_dynamic_section_flags;
  ld_state.callbacks.initialize_plt = ld_generic_initialize_plt;
  ld_state.callbacks.initialize_pltrel = ld_generic_initialize_pltrel;
  ld_state.callbacks.initialize_got = ld_generic_initialize_got;
  ld_state.callbacks.initialize_gotplt = ld_generic_initialize_gotplt;
  ld_state.callbacks.finalize_plt = ld_generic_finalize_plt;
  ld_state.callbacks.rel_type = ld_generic_rel_type;
  ld_state.callbacks.count_relocations = ld_generic_count_relocations;
  ld_state.callbacks.create_relocations = ld_generic_create_relocations;

#ifndef BASE_ELF_NAME
  /* Find the ld backend library.  Use EBL to determine the name if
     the user hasn't provided one on the command line.  */
  if (emulation == NULL)
    {
      emulation = ebl_backend_name (ld_state.ebl);
      assert (emulation != NULL);
    }
  size_t emulation_len = strlen (emulation);

  /* Construct the file name.  */
  char *fname = (char *) alloca (sizeof "libld_" - 1 + emulation_len
				 + sizeof ".so");
  strcpy (mempcpy (stpcpy (fname, "libld_"), emulation, emulation_len), ".so");

  /* Try loading.  */
  void *h = dlopen (fname, RTLD_LAZY);
  if (h == NULL)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot load ld backend library '%s': %s"),
	   fname, dlerror ());

  /* Find the initializer.  It must be present.  */
  char *initname = (char *) alloca (emulation_len + sizeof "_ld_init");
  strcpy (mempcpy (initname, emulation, emulation_len), "_ld_init");
  int (*initfct) (struct ld_state *)
    = (int (*) (struct ld_state *)) dlsym (h, initname);

  if (initfct == NULL)
    error (EXIT_FAILURE, 0, gettext ("\
cannot find init function in ld backend library '%s': %s"),
	   fname, dlerror ());

  /* Store the handle.  */
  ld_state.ldlib = h;

  /* Call the init function.  */
  return initfct (&ld_state);
#else
# define INIT_FCT_NAME(base) _INIT_FCT_NAME(base)
# define _INIT_FCT_NAME(base) base##_ld_init
  /* Declare and call the initialization function.  */
  extern int INIT_FCT_NAME(BASE_ELF_NAME) (struct ld_state *);
  return INIT_FCT_NAME(BASE_ELF_NAME) (&ld_state);
#endif
}


static int
check_for_duplicate2 (struct usedfiles *newp, struct usedfiles *list)
{
  struct usedfiles *first;

  if (list == NULL)
    return 0;

  list = first = list->next;
  do
    {
      /* When searching the needed list we might come across entries
	 for files which are not yet opened.  Stop then, there is
	 nothing more to test.  */
      if (likely (list->status == not_opened))
	break;

      if (unlikely (list->ino == newp->ino)
	  && unlikely (list->dev == newp->dev))
	{
	  close (newp->fd);
	  newp->fd = -1;
	  newp->status = closed;
	  if (newp->file_type == relocatable_file_type)
	    error (0, 0, gettext ("%s listed more than once as input"),
		   newp->rfname);

	  return 1;
	}
      list = list->next;
    }
  while (likely (list != first));

  return 0;
}


static int
check_for_duplicate (struct usedfiles *newp)
{
  struct stat st;

  if (unlikely (fstat (newp->fd, &st) < 0))
    {
      close (newp->fd);
      return errno;
    }

  newp->dev = st.st_dev;
  newp->ino = st.st_ino;

  return (check_for_duplicate2 (newp, ld_state.relfiles)
	  || check_for_duplicate2 (newp, ld_state.dsofiles)
	  || check_for_duplicate2 (newp, ld_state.needed));
}


/* Find a file along the path described in the state.  */
static int
open_along_path2 (struct usedfiles *fileinfo, struct pathelement *path)
{
  const char *fname = fileinfo->fname;
  size_t fnamelen = strlen (fname);
  int err = ENOENT;
  struct pathelement *firstp = path;

  if (path == NULL)
    /* Cannot find anything since we have no path.  */
    return ENOENT;

  do
    {
      if (likely (path->exist >= 0))
	{
	  /* Create the file name.  */
	  char *rfname = NULL;
	  size_t dirlen = strlen (path->pname);
	  int fd = -1;

	  if (fileinfo->file_type == archive_file_type)
	    {
	      const char **exts = (ld_state.statically
				   ? (const char *[2]) { ".a", NULL }
				   : LIB_EXTENSION (&ld_state));

	      /* We have to create the actual file name.  We prepend "lib"
		 and add one of the extensions the platform has.  */
	      while (*exts != NULL)
		{
		  size_t extlen = strlen (*exts);
		  rfname = (char *) alloca (dirlen + 5 + fnamelen + extlen);
		  memcpy (mempcpy (stpcpy (mempcpy (rfname, path->pname,
						    dirlen),
					   "/lib"),
				   fname, fnamelen),
			  *exts, extlen + 1);

		  fd = open (rfname, O_RDONLY);
		  if (likely (fd != -1) || errno != ENOENT)
		    {
		      err = fd == -1 ? errno : 0;
		      break;
		    }

		  /* Next extension.  */
		  ++exts;
		}
	    }
	  else
	    {
	      assert (fileinfo->file_type == dso_file_type
		      || fileinfo->file_type == dso_needed_file_type);

	      rfname = (char *) alloca (dirlen + 1 + fnamelen + 1);
	      memcpy (stpcpy (mempcpy (rfname, path->pname, dirlen), "/"),
		      fname, fnamelen + 1);

	      fd = open (rfname, O_RDONLY);
	      if (unlikely (fd == -1))
		err = errno;
	    }

	  if (likely (fd != -1))
	    {
	      /* We found the file.  This also means the directory
		 exists.  */
	      fileinfo->fd = fd;
	      path->exist = 1;

	      /* Check whether we have this file already loaded.  */
	      if (unlikely (check_for_duplicate (fileinfo) != 0))
		return EAGAIN;

	      /* Make a copy of the name.  */
	      fileinfo->rfname = obstack_strdup (&ld_state.smem, rfname);

	      if (unlikely (ld_state.trace_files))
		printf (fileinfo->file_type == archive_file_type
			? gettext ("%s (for -l%s)\n")
			: gettext ("%s (for DT_NEEDED %s)\n"),
			rfname, fname);

	      return 0;
	    }

	  /* The file does not exist.  Maybe the whole directory doesn't.
	     Check it unless we know it exists.  */
	  if (unlikely (path->exist == 0))
	    {
	      struct stat st;

	      /* Keep only the directory name.  Note that the path
		 might be relative.  This doesn't matter here.  We do
		 the test in any case even if there is the chance that
		 somebody wants to change the programs working
		 directory at some point which would make the result
		 of this test void.  Since changing the working
		 directory is completely wrong we are not taking this
		 case into account.  */
	      rfname[dirlen] = '\0';
	      if (unlikely (stat (rfname, &st) < 0) || ! S_ISDIR (st.st_mode))
		/* The directory does not exist or the named file is no
		   directory.  */
		path->exist = -1;
	      else
		path->exist = 1;
	    }
	}

      /* Next path element.  */
      path = path->next;
    }
  while (likely (err == ENOENT && path != firstp));

  return err;
}


static int
open_along_path (struct usedfiles *fileinfo)
{
  const char *fname = fileinfo->fname;
  int err = ENOENT;

  if (fileinfo->file_type == relocatable_file_type)
    {
      /* Only libraries are searched along the path.  */
      fileinfo->fd = open (fname, O_RDONLY);

      if (likely (fileinfo->fd != -1))
	{
	  /* We found the file.  */
	  if (unlikely (ld_state.trace_files))
	    print_file_name (stdout, fileinfo, 1, 1);

	  return check_for_duplicate (fileinfo);
	}

      /* If the name is an absolute path we are done.  */
      err = errno;
    }
  else
    {
      /* If the user specified two parts to the LD_LIBRARY_PATH variable
	 try the first part now.  */
      err = open_along_path2 (fileinfo, ld_state.ld_library_path1);

      /* Try the user-specified path next.  */
      if (err == ENOENT)
	err = open_along_path2 (fileinfo,
				fileinfo->file_type == archive_file_type
				? ld_state.paths : ld_state.rpath_link);

      /* Then the second part of the LD_LIBRARY_PATH value.  */
      if (unlikely (err == ENOENT))
	{
	  err = open_along_path2 (fileinfo, ld_state.ld_library_path2);

	  /* In case we look for a DSO handle now the RUNPATH.  */
	  if (err == ENOENT)
	    {
	      if (fileinfo->file_type == dso_file_type)
		err = open_along_path2 (fileinfo, ld_state.runpath_link);

	      /* Finally the path from the default linker script.  */
	      if (err == ENOENT)
		err = open_along_path2 (fileinfo, ld_state.default_paths);
	    }
	}
    }

  if (unlikely (err != 0)
      && (err != EAGAIN || fileinfo->file_type == relocatable_file_type))
    error (0, err, gettext ("cannot open %s"), fileinfo->fname);

  return err;
}


static int
matching_group_comdat_scn (const XElf_Sym *sym, size_t shndx,
			   struct usedfiles *fileinfo, struct symbol *oldp)
{
  if ((shndx >= SHN_LORESERVE && shndx <= SHN_HIRESERVE)
      || (oldp->scndx >= SHN_LORESERVE && oldp->scndx <= SHN_HIRESERVE))
    /* Cannot be a group COMDAT section.  */
    return 0;

  size_t newgrpid = fileinfo->scninfo[shndx].grpid;
  size_t oldgrpid = oldp->file->scninfo[oldp->scndx].grpid;
  if (newgrpid == 0 || oldgrpid == 0)
    return 0;

  assert (SCNINFO_SHDR (fileinfo->scninfo[newgrpid].shdr).sh_type
	  == SHT_GROUP);
  assert (SCNINFO_SHDR (oldp->file->scninfo[oldgrpid].shdr).sh_type
	  == SHT_GROUP);

  if (! fileinfo->scninfo[newgrpid].comdat_group
      || ! oldp->file->scninfo[oldgrpid].comdat_group)
    return 0;

  if (strcmp (fileinfo->scninfo[newgrpid].symbols->name,
	      oldp->file->scninfo[oldgrpid].symbols->name) != 0)
    return 0;

  /* This is a matching, duplicate COMDAT group section.  Ignore it.  */
  return 1;
}


static void
check_type_and_size (const XElf_Sym *sym, struct usedfiles *fileinfo,
		     struct symbol *oldp)
{
  /* We check the type and size of the symbols.  In both cases the
     information can be missing (size is zero, type is STT_NOTYPE) in
     which case we issue no warnings.  Otherwise everything must
     match.  If the type does not match there is no point in checking
     the size.  */

  if (XELF_ST_TYPE (sym->st_info) != STT_NOTYPE && oldp->type != STT_NOTYPE
      && unlikely (oldp->type != XELF_ST_TYPE (sym->st_info)))
    {
      char buf1[64];
      char buf2[64];

      error (0, 0, gettext ("\
Warning: type of `%s' changed from %s in %s to %s in %s"),
	     oldp->name,
	     ebl_symbol_type_name (ld_state.ebl, oldp->type,
				   buf1, sizeof (buf1)),
	     oldp->file->rfname,
	     ebl_symbol_type_name (ld_state.ebl, XELF_ST_TYPE (sym->st_info),
				   buf2, sizeof (buf2)),
	     fileinfo->rfname);
    }
  else if (XELF_ST_TYPE (sym->st_info) == STT_OBJECT
	   && oldp->size != 0
	   && unlikely (oldp->size != sym->st_size))
    error (0, 0, gettext ("\
Warning: size of `%s' changed from %" PRIu64 " in %s to %" PRIu64 " in %s"),
	   oldp->name, (uint64_t) oldp->size, oldp->file->rfname,
	   (uint64_t) sym->st_size, fileinfo->rfname);
}


static int
check_definition (const XElf_Sym *sym, size_t shndx, size_t symidx,
		  struct usedfiles *fileinfo, struct symbol *oldp)
{
  int result = 0;
  bool old_in_dso = FILEINFO_EHDR (oldp->file->ehdr).e_type == ET_DYN;
  bool new_in_dso = FILEINFO_EHDR (fileinfo->ehdr).e_type == ET_DYN;
  bool use_new_def = false;

  if (shndx != SHN_UNDEF
      && (! oldp->defined
	  || (shndx != SHN_COMMON && oldp->common && ! new_in_dso)
	  || (old_in_dso && ! new_in_dso)))
    {
      /* We found a definition for a previously undefined symbol or a
	 real definition for a previous common-only definition or a
	 redefinition of a symbol definition in an object file
	 previously defined in a DSO.  First perform some tests which
	 will show whether the common is really matching the
	 definition.  */
      check_type_and_size (sym, fileinfo, oldp);

      /* We leave the next element intact to not interrupt the list
	 with the unresolved symbols.  Whoever walks the list will
	 have to check the `defined' flag.  But we remember that this
	 list element is not unresolved anymore.  */
      if (! oldp->defined)
	{
	  /* Remove from the list.  */
	  --ld_state.nunresolved;
	  if (! oldp->weak)
	    --ld_state.nunresolved_nonweak;
	  CDBL_LIST_DEL (ld_state.unresolved, oldp);
	}
      else if (oldp->common)
	/* Remove from the list.  */
	CDBL_LIST_DEL (ld_state.common_syms, oldp);

      /* Use the values of the definition from now on.  */
      use_new_def = true;
    }
  else if (shndx != SHN_UNDEF
	   && oldp->defined
	   && matching_group_comdat_scn (sym, shndx, fileinfo, oldp))
    /* The duplicate symbol is in a group COMDAT section with the same
       signature as the one containing the original definition.
       Just ignore the second definition.  */
    /* nothing */;
  else if (shndx != SHN_UNDEF
	   && unlikely (! oldp->common)
	   && oldp->defined
	   && shndx != SHN_COMMON
	   /* Multiple definitions are no fatal errors if the -z muldefs flag
	      is used.  We don't warn about the multiple definition unless we
	      are told to be verbose.  */
	   && (!ld_state.muldefs || verbose)
	   && ! old_in_dso && fileinfo->file_type == relocatable_file_type)
    {
      /* We have a double definition.  This is a problem.  */
      char buf[64];
      XElf_Sym_vardef (oldsym);
      struct usedfiles *oldfile;
      const char *scnname;
      Elf32_Word xndx;
      size_t shnum;

      if (elf_getshdrnum (fileinfo->elf, &shnum) < 0)
	error (EXIT_FAILURE, 0,
	       gettext ("cannot determine number of sections: %s"),
	       elf_errmsg (-1));

      /* XXX Use only ebl_section_name.  */
      if (shndx < SHN_LORESERVE || (shndx > SHN_HIRESERVE && shndx < shnum))
	scnname = elf_strptr (fileinfo->elf,
			      fileinfo->shstrndx,
			      SCNINFO_SHDR (fileinfo->scninfo[shndx].shdr).sh_name);
      else
	// XXX extended section
	scnname = ebl_section_name (ld_state.ebl, shndx, 0, buf, sizeof (buf),
				    NULL, shnum);

      /* XXX Print source file and line number.  */
      print_file_name (stderr, fileinfo, 1, 0);
      fprintf (stderr,
	       gettext ("(%s+%#" PRIx64 "): multiple definition of %s `%s'\n"),
	       scnname,
	       (uint64_t) sym->st_value,
	       ebl_symbol_type_name (ld_state.ebl, XELF_ST_TYPE (sym->st_info),
				     buf, sizeof (buf)),
	       oldp->name);

      oldfile = oldp->file;
      xelf_getsymshndx (oldfile->symtabdata, oldfile->xndxdata, oldp->symidx,
			oldsym, xndx);
      assert (oldsym != NULL);

      /* XXX Use only ebl_section_name.  */
      if (oldp->scndx < SHN_LORESERVE || oldp->scndx > SHN_HIRESERVE)
	scnname = elf_strptr (oldfile->elf,
			      oldfile->shstrndx,
			      SCNINFO_SHDR (oldfile->scninfo[shndx].shdr).sh_name);
      else
	scnname = ebl_section_name (ld_state.ebl, oldp->scndx, oldp->scndx,
				    buf, sizeof (buf), NULL, shnum);

      /* XXX Print source file and line number.  */
      print_file_name (stderr, oldfile, 1, 0);
      fprintf (stderr, gettext ("(%s+%#" PRIx64 "): first defined here\n"),
	       scnname, (uint64_t) oldsym->st_value);

      if (likely (!ld_state.muldefs))
	result = 1;
    }
  else if (old_in_dso && fileinfo->file_type == relocatable_file_type
	   && shndx != SHN_UNDEF)
    /* We use the definition from a normal relocatable file over the
       definition in a DSO.  This is what the dynamic linker would
       do, too.  */
    use_new_def = true;
  else if (old_in_dso && !new_in_dso && oldp->defined && !oldp->on_dsolist)
    {
      CDBL_LIST_ADD_REAR (ld_state.from_dso, oldp);
      ++ld_state.nfrom_dso;

      /* If the object is a function we allocate a PLT entry,
	 otherwise only a GOT entry.  */
      if (oldp->type == STT_FUNC)
	++ld_state.nplt;
      else
	++ld_state.ngot;

      oldp->on_dsolist = 1;
    }
  else if (oldp->common && shndx == SHN_COMMON)
    {
      /* The symbol size is the largest of all common definitions.  */
      oldp->size = MAX (oldp->size, sym->st_size);
      /* Similarly for the alignment.  */
      oldp->merge.value = MAX (oldp->merge.value, sym->st_value);
    }

  if (unlikely (use_new_def))
    {
      /* Adjust the symbol record appropriately and remove
	 the symbol from the list of symbols which are taken from DSOs.  */
      if (old_in_dso && fileinfo->file_type == relocatable_file_type)
	{
	  CDBL_LIST_DEL (ld_state.from_dso, oldp);
	  --ld_state.nfrom_dso;

	  if (likely (oldp->type == STT_FUNC))
	    --ld_state.nplt;
	  else
	    --ld_state.ngot;

	  oldp->on_dsolist = 0;
	}

      /* Use the values of the definition from now on.  */
      oldp->size = sym->st_size;
      oldp->type = XELF_ST_TYPE (sym->st_info);
      oldp->symidx = symidx;
      oldp->scndx = shndx;
      //oldp->symscndx = THESYMSCNDX must be passed;
      oldp->file = fileinfo;
      oldp->defined = 1;
      oldp->in_dso = new_in_dso;
      oldp->common = shndx == SHN_COMMON;
      if (likely (fileinfo->file_type == relocatable_file_type))
	{
	  /* If the definition comes from a DSO we pertain the weak flag
	     and it's indicating whether the reference is weak or not.  */
	  oldp->weak = XELF_ST_BIND (sym->st_info) == STB_WEAK;

	  // XXX Really exclude SHN_ABS?
	  if (shndx != SHN_COMMON && shndx != SHN_ABS)
	    {
	      struct scninfo *ignore;
	      mark_section_used (&fileinfo->scninfo[shndx], shndx, &ignore);
	    }
	}

      /* Add to the list of symbols used from DSOs if necessary.  */
      if (new_in_dso && !old_in_dso)
	{
	  CDBL_LIST_ADD_REAR (ld_state.from_dso, oldp);
	  ++ld_state.nfrom_dso;

	  /* If the object is a function we allocate a PLT entry,
	     otherwise only a GOT entry.  */
	  if (oldp->type == STT_FUNC)
	    ++ld_state.nplt;
	  else
	    ++ld_state.ngot;

	  oldp->on_dsolist = 1;
	}
      else if (shndx == SHN_COMMON)
	{
	  /* Store the alignment.  */
	  oldp->merge.value = sym->st_value;

	  CDBL_LIST_ADD_REAR (ld_state.common_syms, oldp);
	}
    }

  return result;
}


static struct scninfo *
find_section_group (struct usedfiles *fileinfo, Elf32_Word shndx,
		    Elf_Data **datap)
{
  struct scninfo *runp;

  for (runp = fileinfo->groups; runp != NULL; runp = runp->next)
    if (!runp->used)
      {
	Elf32_Word *grpref;
	size_t cnt;
	Elf_Data *data;

	data = elf_getdata (runp->scn, NULL);
	if (data == NULL)
	  error (EXIT_FAILURE, 0,
		 gettext ("%s: cannot get section group data: %s"),
		 fileinfo->fname, elf_errmsg (-1));

	/* There cannot be another data block.  */
	assert (elf_getdata (runp->scn, data) == NULL);

	grpref = (Elf32_Word *) data->d_buf;
	cnt = data->d_size / sizeof (Elf32_Word);
	/* Note that we stop after looking at index 1 since index 0
	   contains the flags for the section group.  */
	while (cnt > 1)
	  if (grpref[--cnt] == shndx)
	    {
	      *datap = data;
	      return runp;
	    }
      }

  /* If we come here no section group contained the given section
     despite the SHF_GROUP flag.  This is an error in the input
     file.  */
  error (EXIT_FAILURE, 0, gettext ("\
%s: section '%s' with group flag set does not belong to any group"),
	 fileinfo->fname,
	 elf_strptr (fileinfo->elf, fileinfo->shstrndx,
		     SCNINFO_SHDR (fileinfo->scninfo[shndx].shdr).sh_name));
  return NULL;
}


/* Mark all sections which belong to the same group as section SHNDX
   as used.  */
static void
mark_section_group (struct usedfiles *fileinfo, Elf32_Word shndx,
		    struct scninfo **grpscnp)
{
  /* First locate the section group.  There can be several (many) of
     them.  */
  size_t cnt;
  Elf32_Word *grpref;
  Elf_Data *data;
  struct scninfo *grpscn = find_section_group (fileinfo, shndx, &data);
  *grpscnp = grpscn;

  /* Mark all the sections as used.

     XXX Two possible problems here:

     - the gABI says "The section must be referenced by a section of type
       SHT_GROUP".  I hope everybody reads this as "exactly one section".

     - section groups are also useful to mark the debugging section which
       belongs to a text section.  Unconditionally adding debugging sections
       is therefore probably not what is wanted if stripping is required.  */

  /* Mark the section group as handled.  */
  grpscn->used = true;

  grpref = (Elf32_Word *) data->d_buf;
  cnt = data->d_size / sizeof (Elf32_Word);
  while (cnt > 1)
    {
      Elf32_Word idx = grpref[--cnt];
      XElf_Shdr *shdr = &SCNINFO_SHDR (fileinfo->scninfo[idx].shdr);

      if (fileinfo->scninfo[idx].grpid != grpscn->grpid)
	error (EXIT_FAILURE, 0, gettext ("\
%s: section [%2d] '%s' is not in the correct section group"),
	       fileinfo->fname, (int) idx,
	       elf_strptr (fileinfo->elf, fileinfo->shstrndx, shdr->sh_name));

      if (ld_state.strip == strip_none
	  /* If we are stripping, remove debug sections.  */
	  || (!ebl_debugscn_p (ld_state.ebl,
			       elf_strptr (fileinfo->elf, fileinfo->shstrndx,
					   shdr->sh_name))
	      /* And the relocation sections for the debug sections.  */
	      && ((shdr->sh_type != SHT_RELA && shdr->sh_type != SHT_REL)
		  || !ebl_debugscn_p (ld_state.ebl,
				      elf_strptr (fileinfo->elf,
						  fileinfo->shstrndx,
						  SCNINFO_SHDR (fileinfo->scninfo[shdr->sh_info].shdr).sh_name)))))
	{
	  struct scninfo *ignore;

	  mark_section_used (&fileinfo->scninfo[idx], idx, &ignore);
	}
    }
}


static void
mark_section_used (struct scninfo *scninfo, Elf32_Word shndx,
		   struct scninfo **grpscnp)
{
  if (likely (scninfo->used))
    /* Nothing to be done.  */
    return;

  /* We need this section.  */
  scninfo->used = true;

  /* Make sure the section header has been read from the file.  */
  XElf_Shdr *shdr = &SCNINFO_SHDR (scninfo->shdr);
#if NATIVE_ELF
  if (unlikely (scninfo->shdr == NULL))
#else
  if (unlikely (scninfo->shdr.sh_type == SHT_NULL))
#endif
    {
#if NATIVE_ELF != 0
      shdr = xelf_getshdr (scninfo->scn, scninfo->shdr);
#else
      xelf_getshdr_copy (scninfo->scn, shdr, scninfo->shdr);
#endif
      if (unlikely (shdr == NULL))
	/* Something is very wrong.  The calling code will notice it
	   soon and print a message.  */
	return;
    }

  /* Handle section linked by 'sh_link'.  */
  if (unlikely (shdr->sh_link != 0))
    {
      struct scninfo *ignore;
      mark_section_used (&scninfo->fileinfo->scninfo[shdr->sh_link],
			 shdr->sh_link, &ignore);
    }

  /* Handle section linked by 'sh_info'.  */
  if (unlikely (shdr->sh_info != 0) && (shdr->sh_flags & SHF_INFO_LINK))
    {
      struct scninfo *ignore;
      mark_section_used (&scninfo->fileinfo->scninfo[shdr->sh_info],
			 shdr->sh_info, &ignore);
    }

  if (unlikely (shdr->sh_flags & SHF_GROUP) && ld_state.gc_sections)
    /* Find the section group which contains this section.  */
    mark_section_group (scninfo->fileinfo, shndx, grpscnp);
}


/* We collect all sections in a hashing table.  All sections with the
   same name are collected in a list.  Note that we do not determine
   which sections are finally collected in the same output section
   here.  This would be terribly inefficient.  It will be done later.  */
static void
add_section (struct usedfiles *fileinfo, struct scninfo *scninfo)
{
  struct scnhead *queued;
  struct scnhead search;
  unsigned long int hval;
  XElf_Shdr *shdr = &SCNINFO_SHDR (scninfo->shdr);
  struct scninfo *grpscn = NULL;
  Elf_Data *grpscndata = NULL;

  /* See whether we can determine right away whether we need this
     section in the output.

     XXX I assume here that --gc-sections only affects extraction
     from an archive.  If it also affects objects files given on
     the command line then somebody must explain to me how the
     dependency analysis should work.  Should the entry point be
     the root?  What if it is a numeric value?  */
  if (!scninfo->used
      && (ld_state.strip == strip_none
	  || (shdr->sh_flags & SHF_ALLOC) != 0
	  || shdr->sh_type == SHT_NOTE
	  || (shdr->sh_type == SHT_PROGBITS
	      && strcmp (elf_strptr (fileinfo->elf,
				     fileinfo->shstrndx,
				     shdr->sh_name), ".comment") == 0))
      && (fileinfo->status != in_archive || !ld_state.gc_sections))
    /* Mark as used and handle reference recursively if necessary.  */
    mark_section_used (scninfo, elf_ndxscn (scninfo->scn), &grpscn);

  if ((shdr->sh_flags & SHF_GROUP) && grpscn == NULL)
    /* Determine the symbol which name constitutes the signature
       for the section group.  */
    grpscn = find_section_group (fileinfo, elf_ndxscn (scninfo->scn),
				 &grpscndata);
  assert (grpscn == NULL || grpscn->symbols->name != NULL);

  /* Determine the section name.  */
  search.name = elf_strptr (fileinfo->elf, fileinfo->shstrndx, shdr->sh_name);
  search.type = shdr->sh_type;
  search.flags = shdr->sh_flags;
  search.entsize = shdr->sh_entsize;
  search.grp_signature = grpscn != NULL ? grpscn->symbols->name : NULL;
  search.kind = scn_normal;
  hval = elf_hash (search.name);

  /* Find already queued sections.  */
  queued = ld_section_tab_find (&ld_state.section_tab, hval, &search);
  if (queued != NULL)
    {
      bool is_comdat = false;

      /* If this section is part of a COMDAT section group we simply
	 ignore it since we already have a copy.  */
      if (unlikely (shdr->sh_flags & SHF_GROUP))
	{
	  /* Get the data of the section group section.  */
	  if (grpscndata == NULL)
	    {
	      grpscndata = elf_getdata (grpscn->scn, NULL);
	      assert (grpscndata != NULL);
	    }

	  /* XXX Possibly unaligned memory access.  */
	  if ((((Elf32_Word *) grpscndata->d_buf)[0] & GRP_COMDAT) != 0)
	    {
	      /* We have to compare the group signatures.  There might
		 be sections with the same name but belonging to
		 groups with different signatures.  This means we have
		 to compare the new group signature with all those
		 already collected.  There might also be some
		 non-group sections in the mix.  */
	      struct scninfo *runp = queued->last;
	      do
		{
		  if (SCNINFO_SHDR (runp->shdr).sh_flags & SHF_GROUP)
		    {
		      struct scninfo *grpscn2
			= find_section_group (runp->fileinfo,
					      elf_ndxscn (runp->scn),
					      &grpscndata);

		      if (strcmp (grpscn->symbols->name,
				  grpscn2->symbols->name) == 0)
			{
			  scninfo->unused_comdat = is_comdat = true;
			  break;
			}
		    }

		  runp = runp->next;
		}
	      while (runp != queued->last);
	    }
	}

      if (!is_comdat)
	{
	  /* No COMDAT section, we use the data.  */
	  scninfo->next = queued->last->next;
	  queued->last = queued->last->next = scninfo;

	  queued->flags = ebl_sh_flags_combine (ld_state.ebl, queued->flags,
						shdr->sh_flags);
	  queued->align = MAX (queued->align, shdr->sh_addralign);
	}
    }
  else
    {
      /* We do not use obstacks here since the memory might be
	 deallocated.  */
      queued = (struct scnhead *) xcalloc (sizeof (struct scnhead), 1);
      queued->kind = scn_normal;
      queued->name = search.name;
      queued->type = shdr->sh_type;
      queued->flags = shdr->sh_flags;
      queued->align = shdr->sh_addralign;
      queued->entsize = shdr->sh_entsize;
      queued->grp_signature = grpscn != NULL ? grpscn->symbols->name : NULL;
      queued->segment_nr = ~0;
      queued->last = scninfo->next = scninfo;

      /* Check whether we need a TLS segment.  */
      ld_state.need_tls |= (shdr->sh_flags & SHF_TLS) != 0;

      /* Add to the hash table and possibly overwrite existing value.  */
      ld_section_tab_insert (&ld_state.section_tab, hval, queued);
    }
}


static int
add_relocatable_file (struct usedfiles *fileinfo, GElf_Word secttype)
{
  size_t scncnt;
  size_t cnt;
  Elf_Data *symtabdata = NULL;
  Elf_Data *xndxdata = NULL;
  Elf_Data *versymdata = NULL;
  Elf_Data *verdefdata = NULL;
  Elf_Data *verneeddata = NULL;
  size_t symstridx = 0;
  size_t nsymbols = 0;
  size_t nlocalsymbols = 0;
  bool has_merge_sections = false;
  bool has_tls_symbols = false;
  /* Unless we have different information we assume the code needs
     an executable stack.  */
  enum execstack execstack = execstack_true;

  /* Prerequisites.  */
  assert (fileinfo->elf != NULL);

  /* Allocate memory for the sections.  */
  if (unlikely (elf_getshdrnum (fileinfo->elf, &scncnt) < 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot determine number of sections: %s"),
	   elf_errmsg (-1));

  fileinfo->scninfo = (struct scninfo *)
    obstack_calloc (&ld_state.smem, scncnt * sizeof (struct scninfo));

  /* Read all the section headers and find the symbol table.  Note
     that we don't skip the section with index zero.  Even though the
     section itself is always empty the section header contains
     informaton for the case when the section index for the section
     header string table is too large to fit in the ELF header.  */
  for (cnt = 0; cnt < scncnt; ++cnt)
    {
      /* Store the handle for the section.  */
      fileinfo->scninfo[cnt].scn = elf_getscn (fileinfo->elf, cnt);

      /* Get the ELF section header and data.  */
      XElf_Shdr *shdr;
#if NATIVE_ELF != 0
      if (fileinfo->scninfo[cnt].shdr == NULL)
#else
      if (fileinfo->scninfo[cnt].shdr.sh_type == SHT_NULL)
#endif
	{
#if NATIVE_ELF != 0
	  shdr = xelf_getshdr (fileinfo->scninfo[cnt].scn,
			       fileinfo->scninfo[cnt].shdr);
#else
	  xelf_getshdr_copy (fileinfo->scninfo[cnt].scn, shdr,
			     fileinfo->scninfo[cnt].shdr);
#endif
	  if (shdr == NULL)
	    {
	      /* This should never happen.  */
	      fprintf (stderr, gettext ("%s: invalid ELF file (%s:%d)\n"),
		       fileinfo->rfname, __FILE__, __LINE__);
	      return 1;
	    }
	}
      else
	shdr = &SCNINFO_SHDR (fileinfo->scninfo[cnt].shdr);

      Elf_Data *data = elf_getdata (fileinfo->scninfo[cnt].scn, NULL);

      /* Check whether this section is marked as merge-able.  */
      has_merge_sections |= (shdr->sh_flags & SHF_MERGE) != 0;
      has_tls_symbols |= (shdr->sh_flags & SHF_TLS) != 0;

      /* Get the ELF section header and data.  */
      /* Make the file structure available.  */
      fileinfo->scninfo[cnt].fileinfo = fileinfo;

      if (unlikely (shdr->sh_type == SHT_SYMTAB)
	  || unlikely (shdr->sh_type == SHT_DYNSYM))
	{
	  if (shdr->sh_type == SHT_SYMTAB)
	    {
	      assert (fileinfo->symtabdata == NULL);
	      fileinfo->symtabdata = data;
	      fileinfo->nsymtab = shdr->sh_size / shdr->sh_entsize;
	      fileinfo->nlocalsymbols = shdr->sh_info;
	      fileinfo->symstridx = shdr->sh_link;
	    }
	  else
	    {
	      assert (fileinfo->dynsymtabdata == NULL);
	      fileinfo->dynsymtabdata = data;
	      fileinfo->ndynsymtab = shdr->sh_size / shdr->sh_entsize;
	      fileinfo->dynsymstridx = shdr->sh_link;
	    }

	  /* If we are looking for the normal symbol table we just
	     found it.  */
	  if (secttype == shdr->sh_type)
	    {
	      assert (symtabdata == NULL);
	      symtabdata = data;
	      symstridx = shdr->sh_link;
	      nsymbols = shdr->sh_size / shdr->sh_entsize;
	      nlocalsymbols = shdr->sh_info;
	    }
	}
      else if (unlikely (shdr->sh_type == SHT_SYMTAB_SHNDX))
	{
	  assert (xndxdata == NULL);
	  fileinfo->xndxdata = xndxdata = data;
	}
      else if (unlikely (shdr->sh_type == SHT_GNU_versym))
	{
	  assert (versymdata == 0);
	  fileinfo->versymdata = versymdata = data;
	}
      else if (unlikely (shdr->sh_type == SHT_GNU_verdef))
	{
	  size_t nversions;

	  assert (verdefdata == 0);
	  fileinfo->verdefdata = verdefdata = data;

	  /* Allocate the arrays flagging the use of the version and
	     to track of allocated names.  */
	  fileinfo->nverdef = nversions = shdr->sh_info;
	  /* We have NVERSIONS + 1 because the indeces used to access the
	     sectino start with one; zero represents local binding.  */
	  fileinfo->verdefused = (XElf_Versym *)
	    obstack_calloc (&ld_state.smem,
			    sizeof (XElf_Versym) * (nversions + 1));
	  fileinfo->verdefent = (struct Ebl_Strent **)
	    obstack_alloc (&ld_state.smem,
			   sizeof (struct Ebl_Strent *) * (nversions + 1));
	}
      else if (unlikely (shdr->sh_type == SHT_GNU_verneed))
	{
	  assert (verneeddata == 0);
	  fileinfo->verneeddata = verneeddata = data;
	}
      else if (unlikely (shdr->sh_type == SHT_DYNAMIC))
	{
	  assert (fileinfo->dynscn == NULL);
	  fileinfo->dynscn = fileinfo->scninfo[cnt].scn;
	}
      else if (unlikely (shdr->sh_type == SHT_GROUP))
	{
	  Elf_Scn *symscn;
	  XElf_Shdr_vardef (symshdr);
	  Elf_Data *symdata;

	  if (FILEINFO_EHDR (fileinfo->ehdr).e_type != ET_REL)
	    error (EXIT_FAILURE, 0, gettext ("\
%s: only files of type ET_REL might contain section groups"),
		   fileinfo->fname);

	  fileinfo->scninfo[cnt].next = fileinfo->groups;
	  fileinfo->scninfo[cnt].grpid = cnt;
	  fileinfo->groups = &fileinfo->scninfo[cnt];

	  /* Determine the signature.  We create a symbol record for
	     it.  Only the name element is important.  */
	  fileinfo->scninfo[cnt].symbols = (struct symbol *)
	    obstack_calloc (&ld_state.smem, sizeof (struct symbol));

	  symscn = elf_getscn (fileinfo->elf, shdr->sh_link);
	  xelf_getshdr (symscn, symshdr);
	  symdata = elf_getdata (symscn, NULL);

	  if (symshdr != NULL)
	    {
	      XElf_Sym_vardef (sym);

	      /* We don't need the section index and therefore we don't
		 have to use 'xelf_getsymshndx'.  */
	      xelf_getsym (symdata, shdr->sh_info, sym);
	      if (sym != NULL)
		{
		  struct symbol *symbol = fileinfo->scninfo[cnt].symbols;

#ifndef NO_HACKS
		  if (XELF_ST_TYPE (sym->st_info) == STT_SECTION)
		    {
		      XElf_Shdr_vardef (buggyshdr);
		      xelf_getshdr (elf_getscn (fileinfo->elf, sym->st_shndx),
				    buggyshdr);

		      symbol->name = elf_strptr (fileinfo->elf,
						 FILEINFO_EHDR (fileinfo->ehdr).e_shstrndx,
						 buggyshdr->sh_name);
		      symbol->symidx = -1;
		    }
		  else
#endif
		    {
		      symbol->name = elf_strptr (fileinfo->elf,
						 symshdr->sh_link,
						 sym->st_name);
		      symbol->symidx = shdr->sh_info;
		    }
		  symbol->file = fileinfo;
		}
	    }
	  if (fileinfo->scninfo[cnt].symbols->name == NULL)
	    error (EXIT_FAILURE, 0, gettext ("\
%s: cannot determine signature of section group [%2zd] '%s': %s"),
		   fileinfo->fname,
		   elf_ndxscn (fileinfo->scninfo[cnt].scn),
		   elf_strptr (fileinfo->elf, fileinfo->shstrndx,
			       shdr->sh_name),
		   elf_errmsg (-1));


	  /* For all the sections which are part of this group, add
	     the reference.  */
	  if (data == NULL)
	    error (EXIT_FAILURE, 0, gettext ("\
%s: cannot get content of section group [%2zd] '%s': %s'"),
		   fileinfo->fname, elf_ndxscn (fileinfo->scninfo[cnt].scn),
		   elf_strptr (fileinfo->elf, fileinfo->shstrndx,
			       shdr->sh_name),
		   elf_errmsg (-1));

	  Elf32_Word *grpdata = (Elf32_Word *) data->d_buf;
	  if (grpdata[0] & GRP_COMDAT)
	    fileinfo->scninfo[cnt].comdat_group = true;
	  for (size_t inner = 1; inner < data->d_size / sizeof (Elf32_Word);
	       ++inner)
	    {
	      if (grpdata[inner] >= scncnt)
		error (EXIT_FAILURE, 0, gettext ("\
%s: group member %zu of section group [%2zd] '%s' has too high index: %" PRIu32),
		       fileinfo->fname,
		       inner, elf_ndxscn (fileinfo->scninfo[cnt].scn),
		       elf_strptr (fileinfo->elf, fileinfo->shstrndx,
				   shdr->sh_name),
		       grpdata[inner]);

	      fileinfo->scninfo[grpdata[inner]].grpid = cnt;
	    }

	  /* The 'used' flag is used to indicate when the information
	     in the section group is used to mark all other sections
	     as used.  So it must not be true yet.  */
	  assert (fileinfo->scninfo[cnt].used == false);
	}
      else if (! SECTION_TYPE_P (&ld_state, shdr->sh_type)
	       && unlikely ((shdr->sh_flags & SHF_OS_NONCONFORMING) != 0))
	/* According to the gABI it is a fatal error if the file contains
	   a section with unknown type and the SHF_OS_NONCONFORMING flag
	   set.  */
	error (EXIT_FAILURE, 0,
	       gettext ("%s: section '%s' has unknown type: %d"),
	       fileinfo->fname,
	       elf_strptr (fileinfo->elf, fileinfo->shstrndx,
			   shdr->sh_name),
	       (int) shdr->sh_type);
      /* We don't have to add a few section types here.  These will be
	 generated from scratch for the new output file.  We also
	 don't add the sections of DSOs here since these sections are
	 not used in the resulting object file.  */
      else if (likely (fileinfo->file_type == relocatable_file_type)
	       && likely (cnt > 0)
	       && likely (shdr->sh_type == SHT_PROGBITS
			  || shdr->sh_type == SHT_RELA
			  || shdr->sh_type == SHT_REL
			  || shdr->sh_type == SHT_NOTE
			  || shdr->sh_type == SHT_NOBITS
			  || shdr->sh_type == SHT_INIT_ARRAY
			  || shdr->sh_type == SHT_FINI_ARRAY
			  || shdr->sh_type == SHT_PREINIT_ARRAY))
	{
	  /* Check whether the section needs to be executable.  */
	  if (shdr->sh_type == SHT_PROGBITS
	      && (shdr->sh_flags & SHF_EXECINSTR) == 0
	      && strcmp (elf_strptr (fileinfo->elf, fileinfo->shstrndx,
				     shdr->sh_name),
			 ".note.GNU-stack") == 0)
	    execstack = execstack_false;

	  add_section (fileinfo, &fileinfo->scninfo[cnt]);
	}
    }

  /* Now we know more about the requirements for an executable stack
     of the result.  */
  if (fileinfo->file_type == relocatable_file_type
      && execstack == execstack_true
      && ld_state.execstack != execstack_false_force)
    ld_state.execstack = execstack_true;

  /* Handle the symbols.  Record defined and undefined symbols in the
     hash table.  In theory there can be a file without any symbol
     table.  */
  if (likely (symtabdata != NULL))
    {
      /* In case this file contains merge-able sections we have to
	 locate the symbols which are in these sections.  */
      fileinfo->has_merge_sections = has_merge_sections;
      if (likely (has_merge_sections || has_tls_symbols))
	{
	  fileinfo->symref = (struct symbol **)
	    obstack_calloc (&ld_state.smem,
			    nsymbols * sizeof (struct symbol *));

	  /* Only handle the local symbols here.  */
	  for (cnt = 0; cnt < nlocalsymbols; ++cnt)
	    {
	      Elf32_Word shndx;
	      XElf_Sym_vardef (sym);

	      xelf_getsymshndx (symtabdata, xndxdata, cnt, sym, shndx);
	      if (sym == NULL)
		{
		  /* This should never happen.  */
		  fprintf (stderr, gettext ("%s: invalid ELF file (%s:%d)\n"),
			   fileinfo->rfname, __FILE__, __LINE__);
		  return 1;
		}

	      if (likely (shndx != SHN_XINDEX))
		shndx = sym->st_shndx;
	      else if (unlikely (shndx == 0))
		{
		  fprintf (stderr, gettext ("%s: invalid ELF file (%s:%d)\n"),
			   fileinfo->rfname, __FILE__, __LINE__);
		  return 1;
		}

	      if (XELF_ST_TYPE (sym->st_info) != STT_SECTION
		  && (shndx < SHN_LORESERVE || shndx > SHN_HIRESERVE)
		  && ((SCNINFO_SHDR (fileinfo->scninfo[shndx].shdr).sh_flags
		       & SHF_MERGE)
		      || XELF_ST_TYPE (sym->st_info) == STT_TLS))
		{
		  /* Create a symbol record for this symbol and add it
		     to the list for this section.  */
		  struct symbol *newp;

		  newp = (struct symbol *)
		    obstack_calloc (&ld_state.smem, sizeof (struct symbol));

		  newp->symidx = cnt;
		  newp->scndx = shndx;
		  newp->file = fileinfo;
		  newp->defined = 1;
		  fileinfo->symref[cnt] = newp;

		  if (fileinfo->scninfo[shndx].symbols == NULL)
		    fileinfo->scninfo[shndx].symbols = newp->next_in_scn
		      = newp;
		  else
		    {
		      newp->next_in_scn
			= fileinfo->scninfo[shndx].symbols->next_in_scn;
		      fileinfo->scninfo[shndx].symbols
			= fileinfo->scninfo[shndx].symbols->next_in_scn = newp;
		    }
		}
	    }
	}
      else
	/* Create array with pointers to the symbol definitions.  Note
	   that we only allocate memory for the non-local symbols
	   since we have no merge-able sections.  But we store the
	   pointer as if it was for the whole symbol table.  This
	   saves some memory.  */
	fileinfo->symref = (struct symbol **)
	  obstack_calloc (&ld_state.smem, ((nsymbols - nlocalsymbols)
					   * sizeof (struct symbol *)))
	  - nlocalsymbols;

      /* Don't handle local symbols here.  It's either not necessary
	 at all or has already happened.  */
      for (cnt = nlocalsymbols; cnt < nsymbols; ++cnt)
	{
	  XElf_Sym_vardef (sym);
	  Elf32_Word shndx;
	  xelf_getsymshndx (symtabdata, xndxdata, cnt, sym, shndx);

	  if (sym == NULL)
	    {
	      /* This should never happen.  */
	      fprintf (stderr, gettext ("%s: invalid ELF file (%s:%d)\n"),
		       fileinfo->rfname, __FILE__, __LINE__);
	      return 1;
	    }

	  if (likely (shndx != SHN_XINDEX))
	    shndx = sym->st_shndx;
	  else if (unlikely (shndx == 0))
	    {
	      fprintf (stderr, gettext ("%s: invalid ELF file (%s:%d)\n"),
		       fileinfo->rfname, __FILE__, __LINE__);
	      return 1;
	    }

	  /* We ignore ABS symbols from DSOs.  */
	  // XXX Is this correct?
	  if (unlikely (shndx == SHN_ABS) && secttype == SHT_DYNSYM)
	    continue;

	  if ((shndx < SHN_LORESERVE || shndx > SHN_HIRESERVE)
	      && fileinfo->scninfo[shndx].unused_comdat)
	    /* The symbol is not used.  */
	    continue;

	  /* If the DSO uses symbol versions determine whether this is
	     the default version.  Otherwise we'll ignore the symbol.  */
	  if (versymdata != NULL)
	    {
	      XElf_Versym versym;

	      if (xelf_getversym_copy (versymdata, cnt, versym) == NULL)
		/* XXX Should we handle faulty input files more graceful?  */
		assert (! "xelf_getversym failed");

	      if ((versym & 0x8000) != 0)
		/* Ignore the symbol, it's not the default version.  */
		continue;
	    }

	  /* See whether we know anything about this symbol.  */
	  struct symbol search;
	  search.name = elf_strptr (fileinfo->elf, symstridx, sym->st_name);
	  unsigned long int hval = elf_hash (search.name);

	  /* We ignore the symbols the linker generates.  This are
	     _GLOBAL_OFFSET_TABLE_, _DYNAMIC.  */
	  // XXX This loop is hot and the following tests hardly ever match.
	  // XXX Maybe move the tests somewhere they are executed less often.
	  if (((unlikely (hval == 165832675ul)
		&& strcmp (search.name, "_DYNAMIC") == 0)
	       || (unlikely (hval == 102264335ul)
		   && strcmp (search.name, "_GLOBAL_OFFSET_TABLE_") == 0))
	      && sym->st_shndx != SHN_UNDEF
	      /* If somebody defines such a variable in a relocatable we
		 don't ignore it.  Let the user get what s/he deserves.  */
	      && fileinfo->file_type != relocatable_file_type)
	    continue;

	  struct symbol *oldp = ld_symbol_tab_find (&ld_state.symbol_tab,
						    hval, &search);
	  struct symbol *newp;
	  if (likely (oldp == NULL))
	    {
	      /* No symbol of this name known.  Add it.  */
	      newp = (struct symbol *) obstack_alloc (&ld_state.smem,
						      sizeof (*newp));
	      newp->name = search.name;
	      newp->size = sym->st_size;
	      newp->type = XELF_ST_TYPE (sym->st_info);
	      newp->symidx = cnt;
	      newp->outsymidx = 0;
	      newp->outdynsymidx = 0;
	      newp->scndx = shndx;
	      newp->file = fileinfo;
	      newp->defined = newp->scndx != SHN_UNDEF;
	      newp->common = newp->scndx == SHN_COMMON;
	      newp->weak = XELF_ST_BIND (sym->st_info) == STB_WEAK;
	      newp->added = 0;
	      newp->merged = 0;
	      newp->local = 0;
	      newp->hidden = 0;
	      newp->need_copy = 0;
	      newp->on_dsolist = 0;
	      newp->in_dso = secttype == SHT_DYNSYM;
	      newp->next_in_scn = NULL;
#ifndef NDEBUG
	      newp->next = NULL;
	      newp->previous = NULL;
#endif

	      if (newp->scndx == SHN_UNDEF)
		{
		  CDBL_LIST_ADD_REAR (ld_state.unresolved, newp);
		  ++ld_state.nunresolved;
		  if (! newp->weak)
		    ++ld_state.nunresolved_nonweak;
		}
	      else if (newp->scndx == SHN_COMMON)
		{
		  /* Store the alignment requirement.  */
		  newp->merge.value = sym->st_value;

		  CDBL_LIST_ADD_REAR (ld_state.common_syms, newp);
		}

	      /* Insert the new symbol.  */
	      if (unlikely (ld_symbol_tab_insert (&ld_state.symbol_tab,
						  hval, newp) != 0))
		/* This cannot happen.  */
		abort ();

	      fileinfo->symref[cnt] = newp;

	      /* We have a few special symbols to recognize.  The symbols
		 _init and _fini are the initialization and finalization
		 functions respectively.  They have to be made known in
		 the dynamic section and therefore we have to find out
		 now whether these functions exist or not.  */
	      if (hval == 6685956 && strcmp (newp->name, "_init") == 0)
		ld_state.init_symbol = newp;
	      else if (hval == 6672457 && strcmp (newp->name, "_fini") == 0)
		ld_state.fini_symbol = newp;
	    }
	  else if (unlikely (check_definition (sym, shndx, cnt, fileinfo, oldp)
			     != 0))
	    /* A fatal error (multiple definition of a symbol)
	       occurred, no need to continue.  */
	    return 1;
	  else
	    /* Use the previously allocated symbol record.  It has
	       been updated in check_definition(), if necessary.  */
	    newp = fileinfo->symref[cnt] = oldp;

	  /* Mark the section the symbol we need comes from as used.  */
	  if (shndx != SHN_UNDEF
	      && (shndx < SHN_LORESERVE || shndx > SHN_HIRESERVE))
	    {
	      struct scninfo *ignore;

#ifndef NDEBUG
	      size_t shnum;
	      assert (elf_getshdrnum (fileinfo->elf, &shnum) == 0);
	      assert (shndx < shnum);
#endif

	      /* Mark section (and all dependencies) as used.  */
	      mark_section_used (&fileinfo->scninfo[shndx], shndx, &ignore);

	      /* Check whether the section is merge-able.  In this case we
		 have to record the symbol.  */
	      if (SCNINFO_SHDR (fileinfo->scninfo[shndx].shdr).sh_flags
		  & SHF_MERGE)
		{
		  if (fileinfo->scninfo[shndx].symbols == NULL)
		    fileinfo->scninfo[shndx].symbols = newp->next_in_scn
		      = newp;
		  else
		    {
		      newp->next_in_scn
			= fileinfo->scninfo[shndx].symbols->next_in_scn;
		      fileinfo->scninfo[shndx].symbols
			= fileinfo->scninfo[shndx].symbols->next_in_scn = newp;
		    }
		}
	    }
	}

      /* This file is used.  */
      if (likely (fileinfo->file_type == relocatable_file_type))
	{
	  if (unlikely (ld_state.relfiles == NULL))
	    ld_state.relfiles = fileinfo->next = fileinfo;
	  else
	    {
	      fileinfo->next = ld_state.relfiles->next;
	      ld_state.relfiles = ld_state.relfiles->next = fileinfo;
	    }

	  /* Update some summary information in the state structure.  */
	  ld_state.nsymtab += fileinfo->nsymtab;
	  ld_state.nlocalsymbols += fileinfo->nlocalsymbols;
	}
      else if (likely (fileinfo->file_type == dso_file_type))
	{
	  CSNGL_LIST_ADD_REAR (ld_state.dsofiles, fileinfo);
	  ++ld_state.ndsofiles;

	  if (fileinfo->lazyload)
	    /* We have to create another dynamic section entry for the
	       DT_POSFLAG_1 entry.

	       XXX Once more functionality than the lazyloading flag
	       are suppported the test must be extended.  */
	    ++ld_state.ndsofiles;
	}
    }

  return 0;
}


int
ld_handle_filename_list (struct filename_list *fnames)
{
  struct filename_list *runp;
  int res = 0;

  for (runp = fnames; runp != NULL; runp = runp->next)
    {
      struct usedfiles *curp;

      /* Create a record for the new file.  */
      curp = runp->real = ld_new_inputfile (runp->name, relocatable_file_type);

      /* Set flags for group handling.  */
      curp->group_start = runp->group_start;
      curp->group_end = runp->group_end;

      /* Set as-needed flag from the file, not the command line.  */
      curp->as_needed = runp->as_needed;

      /* Read the file and everything else which comes up, including
	 handling groups.  */
      do
	res |= FILE_PROCESS (-1, curp, &ld_state, &curp);
      while (curp != NULL);
    }

  /* Free the list.  */
  while (fnames != NULL)
    {
      runp = fnames;
      fnames = fnames->next;
      free (runp);
    }

  return res;
}


/* Handle opening of the given file with ELF descriptor.  */
static int
open_elf (struct usedfiles *fileinfo, Elf *elf)
{
  int res = 0;

  if (elf == NULL)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get descriptor for ELF file (%s:%d): %s\n"),
	   __FILE__, __LINE__, elf_errmsg (-1));

  if (unlikely (elf_kind (elf) == ELF_K_NONE))
    {
      struct filename_list *fnames;

      /* We don't have to look at this file again.  */
      fileinfo->status = closed;

      /* Let's see whether this is a linker script.  */
      if (fileinfo->fd != -1)
	/* Create a stream from the file handle we know.  */
	ldin = fdopen (fileinfo->fd, "r");
      else
	{
	  /* Get the memory for the archive member.  */
	  char *content;
	  size_t contentsize;

	  /* Get the content of the file.  */
	  content = elf_rawfile (elf, &contentsize);
	  if (content == NULL)
	    {
	      fprintf (stderr, gettext ("%s: invalid ELF file (%s:%d)\n"),
		       fileinfo->rfname, __FILE__, __LINE__);
	      return 1;
	    }

	  /* The content of the file is available in memory.  Read the
	     memory region as a stream.  */
	  ldin = fmemopen (content, contentsize, "r");
	}

      /* No need for locking.  */
      __fsetlocking (ldin, FSETLOCKING_BYCALLER);

      if (ldin == NULL)
	error (EXIT_FAILURE, errno, gettext ("cannot open '%s'"),
	       fileinfo->rfname);

      /* Parse the file.  If it is a linker script no problems will be
	 reported.  */
      ld_state.srcfiles = NULL;
      ldlineno = 1;
      ld_scan_version_script = 0;
      ldin_fname = fileinfo->rfname;
      res = ldparse ();

      fclose (ldin);
      if (fileinfo->fd != -1 && !fileinfo->fd_passed)
	{
	  /* We won't need the file descriptor again.  */
	  close (fileinfo->fd);
	  fileinfo->fd = -1;
	}

      elf_end (elf);

      if (unlikely (res != 0))
	/* Something went wrong during parsing.  */
	return 1;

      /* This is no ELF file.  */
      fileinfo->elf = NULL;

      /* Now we have to handle eventual INPUT and GROUP statements in
	 the script.  Read the files mentioned.  */
      fnames = ld_state.srcfiles;
      if (fnames != NULL)
	{
	  struct filename_list *oldp;

	  /* Convert the list into a normal single-linked list.  */
	  oldp = fnames;
	  fnames = fnames->next;
	  oldp->next = NULL;

	  /* Remove the list from the state structure.  */
	  ld_state.srcfiles = NULL;

	  if (unlikely (ld_handle_filename_list (fnames) != 0))
	    return 1;
	}

      return 0;
    }

  /* Store the file info.  */
  fileinfo->elf = elf;

  /* The file is ready for action.  */
  fileinfo->status = opened;

  return 0;
}


static int
add_whole_archive (struct usedfiles *fileinfo)
{
  Elf *arelf;
  Elf_Cmd cmd = ELF_C_READ_MMAP_PRIVATE;
  int res = 0;

  while ((arelf = elf_begin (fileinfo->fd, cmd, fileinfo->elf)) != NULL)
    {
      Elf_Arhdr *arhdr = elf_getarhdr (arelf);
      struct usedfiles *newp;

      if (arhdr == NULL)
	abort ();

      /* Just to be sure; since these are no files in the archive
	 these names should never be returned.  */
      assert (strcmp (arhdr->ar_name, "/") != 0);
      assert (strcmp (arhdr->ar_name, "//") != 0);

      newp = ld_new_inputfile (arhdr->ar_name, relocatable_file_type);
      newp->archive_file = fileinfo;

      if (unlikely (ld_state.trace_files))
	print_file_name (stdout, newp, 1, 1);

      /* This shows that this file is contained in an archive.  */
      newp->fd = -1;
      /* Store the ELF descriptor.  */
      newp->elf = arelf;
      /* Show that we are open for business.  */
      newp->status = opened;

      /* Proces the file, add all the symbols etc.  */
      res = file_process2 (newp);
      if (unlikely (res != 0))
	    break;

      /* Advance to the next archive element.  */
      cmd = elf_next (arelf);
    }

  return res;
}


static int
extract_from_archive (struct usedfiles *fileinfo)
{
  static int archive_seq;
  int res = 0;

  if (fileinfo->archive_seq == 0)
    /* This is an archive we are not using completely.  Give it a
       unique number.  */
    fileinfo->archive_seq = ++archive_seq;

  /* If there are no unresolved symbols don't do anything.  */
  assert (ld_state.extract_rule == defaultextract
	  || ld_state.extract_rule == weakextract);
  if ((likely (ld_state.extract_rule == defaultextract)
       ? ld_state.nunresolved_nonweak : ld_state.nunresolved) == 0)
    return 0;

  Elf_Arsym *syms;
  size_t nsyms;

  /* Get all the symbols.  */
  syms = elf_getarsym (fileinfo->elf, &nsyms);
  if (syms == NULL)
    {
    cannot_read_archive:
      error (0, 0, gettext ("cannot read archive `%s': %s"),
	     fileinfo->rfname, elf_errmsg (-1));

      /* We cannot use this archive anymore.  */
      fileinfo->status = closed;

      return 1;
    }

  /* Now add all the symbols to the hash table.  Note that there
     can potentially be duplicate definitions.  We'll always use
     the first definition.  */
  // XXX Is this a compatible behavior?
  bool any_used;
  do
    {
      any_used = false;

      size_t cnt;
      for (cnt = 0; cnt < nsyms; ++cnt)
	{
	  struct symbol search = { .name = syms[cnt].as_name };
	  struct symbol *sym = ld_symbol_tab_find (&ld_state.symbol_tab,
						   syms[cnt].as_hash, &search);
	  if (sym != NULL && ! sym->defined)
	    {
	      /* The symbol is referenced and not defined.  */
	      Elf *arelf;
	      Elf_Arhdr *arhdr;
	      struct usedfiles *newp;

	      /* Find the archive member for this symbol.  */
	      if (unlikely (elf_rand (fileinfo->elf, syms[cnt].as_off)
			    != syms[cnt].as_off))
		goto cannot_read_archive;

	      /* Note: no test of a failing 'elf_begin' call.  That's fine
		 since 'elf'getarhdr' will report the problem.  */
	      arelf = elf_begin (fileinfo->fd, ELF_C_READ_MMAP_PRIVATE,
				 fileinfo->elf);
	      arhdr = elf_getarhdr (arelf);
	      if (arhdr == NULL)
		goto cannot_read_archive;

	      /* We have all the information and an ELF handle for the
		 archive member.  Create the normal data structure for
		 a file now.  */
	      newp = ld_new_inputfile (obstack_strdup (&ld_state.smem,
						       arhdr->ar_name),
				       relocatable_file_type);
	      newp->archive_file = fileinfo;

	      if (unlikely (ld_state.trace_files))
		print_file_name (stdout, newp, 1, 1);

	      /* This shows that this file is contained in an archive.  */
	      newp->fd = -1;
	      /* Store the ELF descriptor.  */
	      newp->elf = arelf;
	      /* Show that we are open for business.  */
	      newp->status = in_archive;

	      /* Now read the file and add all the symbols.  */
	      res = file_process2 (newp);
	      if (unlikely (res != 0))
		return res;

	      any_used = true;
	    }
	}

      if (any_used)
	{
	  /* This is an archive therefore it must have a number.  */
	  assert (fileinfo->archive_seq != 0);
	  ld_state.last_archive_used = fileinfo->archive_seq;
	}
    }
  while (any_used);

  return res;
}


static int
file_process2 (struct usedfiles *fileinfo)
{
  int res;

  if (likely (elf_kind (fileinfo->elf) == ELF_K_ELF))
    {
      /* The first time we get here we read the ELF header.  */
#if NATIVE_ELF != 0
      if (likely (fileinfo->ehdr == NULL))
#else
      if (likely (FILEINFO_EHDR (fileinfo->ehdr).e_type == ET_NONE))
#endif
	{
	  XElf_Ehdr *ehdr;
#if NATIVE_ELF != 0
	  ehdr = xelf_getehdr (fileinfo->elf, fileinfo->ehdr);
#else
	  xelf_getehdr_copy (fileinfo->elf, ehdr, fileinfo->ehdr);
#endif
	  if (ehdr == NULL)
	    {
	      fprintf (stderr, gettext ("%s: invalid ELF file (%s:%d)\n"),
		       fileinfo->rfname, __FILE__, __LINE__);
	      fileinfo->status = closed;
	      return 1;
	    }

	  if (FILEINFO_EHDR (fileinfo->ehdr).e_type != ET_REL
	      && unlikely (FILEINFO_EHDR (fileinfo->ehdr).e_type != ET_DYN))
	    /* XXX Add ebl* function to query types which are allowed
	       to link in.  */
	    {
	      char buf[64];

	      print_file_name (stderr, fileinfo, 1, 0);
	      fprintf (stderr,
		       gettext ("file of type %s cannot be linked in\n"),
		       ebl_object_type_name (ld_state.ebl,
					     FILEINFO_EHDR (fileinfo->ehdr).e_type,
					     buf, sizeof (buf)));
	      fileinfo->status = closed;
	      return 1;
	    }

	  /* Make sure the file type matches the backend.  */
	  if (FILEINFO_EHDR (fileinfo->ehdr).e_machine
	      != ebl_get_elfmachine (ld_state.ebl))
	    {
	      fprintf (stderr, gettext ("\
%s: input file incompatible with ELF machine type %s\n"),
		       fileinfo->rfname,
		       ebl_backend_name (ld_state.ebl));
	      fileinfo->status = closed;
	      return 1;
	    }

	  /* Determine the section header string table section index.  */
	  if (unlikely (elf_getshdrstrndx (fileinfo->elf, &fileinfo->shstrndx)
			< 0))
	    {
	      fprintf (stderr, gettext ("\
%s: cannot get section header string table index: %s\n"),
		       fileinfo->rfname, elf_errmsg (-1));
	      fileinfo->status = closed;
	      return 1;
	    }
	}

      /* Now handle the different types of files.  */
      if (FILEINFO_EHDR (fileinfo->ehdr).e_type == ET_REL)
	{
	  /* Add all the symbol.  Relocatable files have symbol
	     tables.  */
	  res = add_relocatable_file (fileinfo, SHT_SYMTAB);
	}
      else
	{
	  bool has_l_name = fileinfo->file_type == archive_file_type;

	  assert (FILEINFO_EHDR (fileinfo->ehdr).e_type == ET_DYN);

	  /* If the file is a DT_NEEDED dependency then the type is
	     already correctly specified.  */
	  if (fileinfo->file_type != dso_needed_file_type)
	    fileinfo->file_type = dso_file_type;

	  /* We cannot use DSOs when generating relocatable objects.  */
	  if (ld_state.file_type == relocatable_file_type)
	    {
	      error (0, 0, gettext ("\
cannot use DSO '%s' when generating relocatable object file"),
		     fileinfo->fname);
	      return 1;
	    }

	  /* Add all the symbols.  For DSOs we are looking at the
	     dynamic symbol table.  */
	  res = add_relocatable_file (fileinfo, SHT_DYNSYM);

	  /* We always have to have a dynamic section.  */
	  assert (fileinfo->dynscn != NULL);

	  /* We have to remember the dependencies for this object.  It
	     is necessary to look them up.  */
	  XElf_Shdr_vardef (dynshdr);
	  xelf_getshdr (fileinfo->dynscn, dynshdr);

	  Elf_Data *dyndata = elf_getdata (fileinfo->dynscn, NULL);
	  /* XXX Should we flag the failure to get the dynamic section?  */
	  if (dynshdr != NULL)
	    {
	      int cnt = dynshdr->sh_size / dynshdr->sh_entsize;
	      XElf_Dyn_vardef (dyn);

	      while (--cnt >= 0)
		{
		  xelf_getdyn (dyndata, cnt, dyn);
		  if (dyn != NULL)
		    {
		      if(dyn->d_tag == DT_NEEDED)
			{
			  struct usedfiles *newp;

			  newp = ld_new_inputfile (elf_strptr (fileinfo->elf,
							       dynshdr->sh_link,
							       dyn->d_un.d_val),
						   dso_needed_file_type);

			  /* Enqueue the newly found dependencies.  */
			  // XXX Check that there not already a file with the
			  // same name.
			  CSNGL_LIST_ADD_REAR (ld_state.needed, newp);
			}
		      else if (dyn->d_tag == DT_SONAME)
			{
			  /* We use the DT_SONAME (this is what's there
			     for).  */
			  fileinfo->soname = elf_strptr (fileinfo->elf,
							 dynshdr->sh_link,
							 dyn->d_un.d_val);
			  has_l_name = false;
			}
		    }
		}
	    }

	  /* Construct the file name if the DSO has no SONAME and the
	     file name comes from a -lXX parameter on the comment
	     line.  */
	  if (unlikely (has_l_name))
	    {
	      /* The FNAME is the parameter the user specified on the
		 command line.  We prepend "lib" and append ".so".  */
	      size_t len = strlen (fileinfo->fname) + 7;
	      char *newp;

	      newp = (char *) obstack_alloc (&ld_state.smem, len);
	      strcpy (stpcpy (stpcpy (newp, "lib"), fileinfo->fname), ".so");

	      fileinfo->soname = newp;
	    }
	}
    }
  else if (likely (elf_kind (fileinfo->elf) == ELF_K_AR))
    {
      if (unlikely (ld_state.extract_rule == allextract))
	/* Which this option enabled we have to add all the object
	   files in the archive.  */
	res = add_whole_archive (fileinfo);
      else if (ld_state.file_type == relocatable_file_type)
	{
	  /* When generating a relocatable object we don't find files
	     in archives.  */
	  if (verbose)
	    error (0, 0, gettext ("input file '%s' ignored"), fileinfo->fname);

	  res = 0;
	}
      else
	{
	  if (ld_state.group_start_requested
	      && ld_state.group_start_archive == NULL)
	    ld_state.group_start_archive = fileinfo;

	  if (ld_state.archives == NULL)
	    ld_state.archives = fileinfo;

	  if (ld_state.tailarchives != NULL)
	    ld_state.tailarchives->next = fileinfo;
	  ld_state.tailarchives = fileinfo;

	  /* Extract only the members from the archive which are
	     currently referenced by unresolved symbols.  */
	  res = extract_from_archive (fileinfo);
	}
    }
  else
    /* This should never happen, we know about no other types.  */
    abort ();

  return res;
}


/* Process a given file.  The first parameter is a file descriptor for
   the file which can be -1 to indicate the file has not yet been
   found.  The second parameter describes the file to be opened, the
   last one is the state of the linker which among other information
   contain the paths we look at.  */
static int
ld_generic_file_process (int fd, struct usedfiles *fileinfo,
			 struct ld_state *statep, struct usedfiles **nextp)
{
  int res = 0;

  /* By default we go to the next file in the list.  */
  *nextp = fileinfo->next;

  /* Set the flag to signal we are looking for a group start.  */
  if (unlikely (fileinfo->group_start))
    {
      ld_state.group_start_requested = true;
      fileinfo->group_start = false;
    }

  /* If the file isn't open yet, open it now.  */
  if (likely (fileinfo->status == not_opened))
    {
      bool fd_passed = true;

      if (likely (fd == -1))
	{
	  /* Find the file ourselves.  */
	  int err = open_along_path (fileinfo);
	  if (unlikely (err != 0))
	    /* We allow libraries and DSOs to be named more than once.
	       Don't report an error to the caller.  */
	    return err == EAGAIN ? 0 : err;

	  fd_passed = false;
	}
      else
	fileinfo->fd = fd;

      /* Remember where we got the descriptor from.  */
      fileinfo->fd_passed = fd_passed;

      /* We found the file.  Now test whether it is a file type we can
	 handle.

	 XXX Do we need to have the ability to start from a given
	 position in the search path again to look for another file if
	 the one found has not the right type?  */
      res = open_elf (fileinfo, elf_begin (fileinfo->fd,
					   is_dso_p (fileinfo->fd)
					   ? ELF_C_READ_MMAP
					   : ELF_C_READ_MMAP_PRIVATE, NULL));
      if (unlikely (res != 0))
	return res;
    }

  /* Now that we have opened the file start processing it.  */
  if (likely (fileinfo->status != closed))
    res = file_process2 (fileinfo);

  /* Determine which file to look at next.  */
  if (unlikely (fileinfo->group_backref != NULL))
    {
      /* We only go back if an archive other than the one we would go
	 back to has been used in the last round.  */
      if (ld_state.last_archive_used > fileinfo->group_backref->archive_seq)
	{
	  *nextp = fileinfo->group_backref;
	  ld_state.last_archive_used = 0;
	}
      else
	{
	  /* If we come here this means that the archives we read so
	     far are not needed anymore.  We can free some of the data
	     now.  */
	  struct usedfiles *runp = ld_state.archives;

	  do
	    {
	      /* We don't need the ELF descriptor anymore.  Unless there
		 are no files from the archive used this will not free
		 the whole file but only some data structures.  */
	      elf_end (runp->elf);
	      runp->elf = NULL;

	      runp = runp->next;
	    }
	  while (runp != fileinfo->next);

	  /* Do not do this again.  */
	  ld_state.archives = NULL;

	  /* Do not move on to the next archive.  */
	  *nextp = fileinfo->next = NULL;
	}
    }
  else if (unlikely (fileinfo->group_end))
    {
      /* This is the end of a group.  We possibly have to go back.
	 Determine which file we would go back to and see whether it
	 makes sense.  If there has not been an archive we don't have
	 to do anything.  */
      if (ld_state.group_start_requested)
	{
	  if (ld_state.group_start_archive != ld_state.tailarchives)
	    /* The loop includes more than one archive, add the pointer.  */
	    {
	      *nextp = ld_state.tailarchives->group_backref =
		ld_state.group_start_archive;
	      ld_state.last_archive_used = 0;
	    }
	  else
	    /* We might still have to go back to the beginning of the
	       group if since the last archive other files have been
	       added.  But we go back exactly once.  */
	    if (ld_state.tailarchives != fileinfo)
	      {
		*nextp = ld_state.group_start_archive;
		ld_state.last_archive_used = 0;
	      }
	}

      /* Clear the flags.  */
      ld_state.group_start_requested = false;
      ld_state.group_start_archive = NULL;
      fileinfo->group_end = false;
    }

  return res;
}


/* Library names passed to the linker as -lXX represent files named
   libXX.YY.  The YY part can have different forms, depending on the
   platform.  The generic set is .so and .a (in this order).  */
static const char **
ld_generic_lib_extensions (struct ld_state *statep __attribute__ ((__unused__)))
{
  static const char *exts[] =
    {
      ".so", ".a", NULL
    };

  return exts;
}


/* Flag unresolved symbols.  */
static int
ld_generic_flag_unresolved (struct ld_state *statep)
{
  int retval = 0;

  if (ld_state.nunresolved_nonweak > 0)
    {
      /* Go through the list and determine the unresolved symbols.  */
      struct symbol *first;
      struct symbol *s;

      s = first = ld_state.unresolved->next;
      do
	{
	  if (! s->defined && ! s->weak)
	    {
	      /* Two special symbol we recognize: the symbol for the
		 GOT and the dynamic section.  */
	      if (strcmp (s->name, "_GLOBAL_OFFSET_TABLE_") == 0
		  || strcmp (s->name, "_DYNAMIC") == 0)
		{
		  /* We will have to fill in more information later.  */
		  ld_state.need_got = true;

		  /* Remember that we found it.  */
		  if (s->name[1] == 'G')
		    ld_state.got_symbol = s;
		  else
		    ld_state.dyn_symbol = s;
		}
	      else if (ld_state.file_type != dso_file_type || !ld_state.nodefs)
		{
		  /* XXX The error message should get better.  It should use
		     the debugging information if present to tell where in the
		     sources the undefined reference is.  */
		  error (0, 0, gettext ("undefined symbol `%s' in %s"),
			 s->name, s->file->fname);

		  retval = 1;
		}
	    }

	  /* We cannot decide here what to do with undefined
	     references which will come from DSO since we do not know
	     what kind of symbol we expect.  Only when looking at the
	     relocations we can see whether we need a PLT entry or
	     only a GOT entry.  */

	  s = s->next;
	}
      while (s != first);
    }

  return retval;
}


/* Close the given file.  */
static int
ld_generic_file_close (struct usedfiles *fileinfo, struct ld_state *statep)
{
  /* Close the ELF descriptor.  */
  elf_end (fileinfo->elf);

  /* If we have opened the file descriptor close it.  But we might
     have done this already in which case FD is -1.  */
  if (!fileinfo->fd_passed && fileinfo->fd != -1)
    close (fileinfo->fd);

  /* We allocated the resolved file name.  */
  if (fileinfo->fname != fileinfo->rfname)
    free ((char *) fileinfo->rfname);

  return 0;
}


static void
new_generated_scn (enum scn_kind kind, const char *name, int type, int flags,
		   int entsize, int align)
{
  struct scnhead *newp;

  newp = (struct scnhead *) obstack_calloc (&ld_state.smem,
					    sizeof (struct scnhead));
  newp->kind = kind;
  newp->name = name;
  newp->nameent = ebl_strtabadd (ld_state.shstrtab, name, 0);
  newp->type = type;
  newp->flags = flags;
  newp->entsize = entsize;
  newp->align = align;
  newp->grp_signature = NULL;
  newp->used = true;

  /* All is well.  Create now the data for the section and insert it
     into the section table.  */
  ld_section_tab_insert (&ld_state.section_tab, elf_hash (name), newp);
}


/* Create the sections which are generated by the linker and are not
   present in the input file.  */
static void
ld_generic_generate_sections (struct ld_state *statep)
{
  /* The relocation section type.  */
  int rel_type = REL_TYPE (&ld_state) == DT_REL ? SHT_REL : SHT_RELA;

  /* When requested, every output file will have a build ID section.  */
  if (statep->build_id != NULL)
    new_generated_scn (scn_dot_note_gnu_build_id, ".note.gnu.build-id",
		       SHT_NOTE, SHF_ALLOC, 0, 4);

  /* When building dynamically linked object we have to include a
     section containing a string describing the interpreter.  This
     should be at the very beginning of the file together with the
     other information the ELF loader (kernel or wherever) has to look
     at.  We put it as the first section in the file.

     We also have to create the dynamic segment which is a special
     section the dynamic linker locates through an entry in the
     program header.  */
  if (dynamically_linked_p ())
    {
      /* Use any versioning (defined or required)?  */
      bool use_versioning = false;
      /* Use version requirements?  */
      bool need_version = false;

      /* First the .interp section.  */
      if (ld_state.interp != NULL || ld_state.file_type != dso_file_type)
	new_generated_scn (scn_dot_interp, ".interp", SHT_PROGBITS, SHF_ALLOC,
			   0, 1);

      /* Now the .dynamic section.  */
      new_generated_scn (scn_dot_dynamic, ".dynamic", SHT_DYNAMIC,
			 DYNAMIC_SECTION_FLAGS (&ld_state),
			 xelf_fsize (ld_state.outelf, ELF_T_DYN, 1),
			 xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1));

      /* We will need in any case the dynamic symbol table (even in
	 the unlikely case that no symbol is exported or referenced
	 from a DSO).  */
      ld_state.need_dynsym = true;
      new_generated_scn (scn_dot_dynsym, ".dynsym", SHT_DYNSYM, SHF_ALLOC,
			 xelf_fsize (ld_state.outelf, ELF_T_SYM, 1),
			 xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1));
      /* It comes with a string table.  */
      new_generated_scn (scn_dot_dynstr, ".dynstr", SHT_STRTAB, SHF_ALLOC,
			 0, 1);
      /* And a hashing table.  */
      // XXX For Linux/Alpha we need other sizes unless they change...
      if (GENERATE_SYSV_HASH)
	new_generated_scn (scn_dot_hash, ".hash", SHT_HASH, SHF_ALLOC,
			   sizeof (Elf32_Word), sizeof (Elf32_Word));
      if (GENERATE_GNU_HASH)
	new_generated_scn (scn_dot_gnu_hash, ".gnu.hash", SHT_GNU_HASH,
			   SHF_ALLOC, sizeof (Elf32_Word),
			   sizeof (Elf32_Word));

      /* Create the section associated with the PLT if necessary.  */
      if (ld_state.nplt > 0)
	{
	  /* Create the .plt section.  */
	  /* XXX We might need a function which returns the section flags.  */
	  new_generated_scn (scn_dot_plt, ".plt", SHT_PROGBITS,
			     SHF_ALLOC | SHF_EXECINSTR,
			     /* XXX Is the size correct?  */
			     xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1),
			     xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1));

	  /* Create the relocation section for the .plt.  This is always
	     separate even if the other relocation sections are combined.  */
	  new_generated_scn (scn_dot_pltrel, ".rel.plt", rel_type, SHF_ALLOC,
			     rel_type == SHT_REL
			     ? xelf_fsize (ld_state.outelf, ELF_T_REL, 1)
			     : xelf_fsize (ld_state.outelf, ELF_T_RELA, 1),
			     xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1));

	  /* XXX We might need a function which returns the section flags.  */
	  new_generated_scn (scn_dot_gotplt, ".got.plt", SHT_PROGBITS,
			     SHF_ALLOC | SHF_WRITE,
			     xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1),
			     xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1));

	  /* Mark all used DSOs as used.  Determine whether any referenced
	     object uses symbol versioning.  */
	  if (ld_state.from_dso != NULL)
	    {
	      struct symbol *srunp = ld_state.from_dso;

	      do
		{
		  srunp->file->used = true;

		  if (srunp->file->verdefdata != NULL)
		    {
		      XElf_Versym versym;

		      /* The input DSO uses versioning.  */
		      use_versioning = true;
		      /* We reference versions.  */
		      need_version = true;

		      if (xelf_getversym_copy (srunp->file->versymdata,
					       srunp->symidx, versym) == NULL)
			assert (! "xelf_getversym failed");

		      /* We cannot link explicitly with an older
			 version of a symbol.  */
		      assert ((versym & 0x8000) == 0);
		      /* We cannot reference local (index 0) or plain
			 global (index 1) versions.  */
		      assert (versym > 1);

		      /* Check whether we have already seen the
			 version and if not add it to the referenced
			 versions in the output file.  */
		      if (! srunp->file->verdefused[versym])
			{
			  srunp->file->verdefused[versym] = 1;

			  if (++srunp->file->nverdefused == 1)
			    /* Count the file if it is using versioning.  */
			    ++ld_state.nverdeffile;
			  ++ld_state.nverdefused;
			}
		    }
		}
	      while ((srunp = srunp->next) != ld_state.from_dso);
	    }

	  /* Create the sections used to record version dependencies.  */
	  if (need_version)
	    new_generated_scn (scn_dot_version_r, ".gnu.version_r",
			       SHT_GNU_verneed, SHF_ALLOC, 0,
			       xelf_fsize (ld_state.outelf, ELF_T_WORD, 1));
	}

      /* Now count the used DSOs since this is what the user
	 wants.  */
      int ndt_needed = 0;
      if (ld_state.ndsofiles > 0)
	{
	  struct usedfiles *frunp = ld_state.dsofiles;

	  do
	    if (! frunp->as_needed || frunp->used)
	      {
		++ndt_needed;
		if (frunp->lazyload)
		  /* We have to create another dynamic section
		     entry for the DT_POSFLAG_1 entry.

		     XXX Once more functionality than the lazyloading
		     flag are suppported the test must be
		     extended.  */
		  ++ndt_needed;
	      }
	  while ((frunp = frunp->next) != ld_state.dsofiles);
	}

      if (use_versioning)
	new_generated_scn (scn_dot_version, ".gnu.version", SHT_GNU_versym,
			   SHF_ALLOC,
			   xelf_fsize (ld_state.outelf, ELF_T_HALF, 1),
			   xelf_fsize (ld_state.outelf, ELF_T_HALF, 1));

      /* We need some entries all the time.  */
      ld_state.ndynamic = (7 + (ld_state.runpath != NULL
				|| ld_state.rpath != NULL)
			   + ndt_needed
			   + (ld_state.init_symbol != NULL ? 1 : 0)
			   + (ld_state.fini_symbol != NULL ? 1 : 0)
			   + (use_versioning ? 1 : 0)
			   + (need_version ? 2 : 0)
			   + (ld_state.nplt > 0 ? 4 : 0)
			   + (ld_state.relsize_total > 0 ? 3 : 0));
    }

  /* When creating a relocatable file or when we are not stripping the
     output file we create a symbol table.  */
  ld_state.need_symtab = (ld_state.file_type == relocatable_file_type
			  || ld_state.strip == strip_none);

  /* Add the .got section if needed.  */
  if (ld_state.need_got)
    /* XXX We might need a function which returns the section flags.  */
    new_generated_scn (scn_dot_got, ".got", SHT_PROGBITS,
		       SHF_ALLOC | SHF_WRITE,
		       xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1),
		       xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1));

  /* Add the .rel.dyn section.  */
  if (ld_state.relsize_total > 0)
    new_generated_scn (scn_dot_dynrel, ".rel.dyn", rel_type, SHF_ALLOC,
		       rel_type == SHT_REL
		       ? xelf_fsize (ld_state.outelf, ELF_T_REL, 1)
		       : xelf_fsize (ld_state.outelf, ELF_T_RELA, 1),
		       xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1));
}


/* Callback function registered with on_exit to make sure the temporary
   files gets removed if something goes wrong.  */
static void
remove_tempfile (int status, void *arg)
{
  if (status != 0 && ld_state.tempfname != NULL)
    unlink (ld_state.tempfname);
}


/* Create the output file.  The file name is given or "a.out".  We
   create as much of the ELF structure as possible.  */
static int
ld_generic_open_outfile (struct ld_state *statep, int machine, int klass,
			 int data)
{
  /* We do not create the new file right away with the final name.
     This would destroy an existing file with this name before a
     replacement is finalized.  We create instead a temporary file in
     the same directory.  */
  if (ld_state.outfname == NULL)
    ld_state.outfname = "a.out";

  size_t outfname_len = strlen (ld_state.outfname);
  char *tempfname = (char *) obstack_alloc (&ld_state.smem,
					    outfname_len + sizeof (".XXXXXX"));
  ld_state.tempfname = tempfname;

  int fd;
  int try = 0;
  while (1)
    {
      strcpy (mempcpy (tempfname, ld_state.outfname, outfname_len), ".XXXXXX");

      /* The use of mktemp() here is fine.  We do not want to use
	 mkstemp() since then the umask isn't used.  And the output
	 file will have these permissions anyhow.  Any intruder could
	 change the file later if it would be possible now.  */
      if (mktemp (tempfname) != NULL
	  && (fd = open (tempfname, O_RDWR | O_EXCL | O_CREAT | O_NOFOLLOW,
			 ld_state.file_type == relocatable_file_type
			 ? DEFFILEMODE : ACCESSPERMS)) != -1)
	break;

      /* Failed this round.  We keep trying a number of times.  */
      if (++try >= 10)
	error (EXIT_FAILURE, errno, gettext ("cannot create output file"));
    }
  ld_state.outfd = fd;

  /* Make sure we remove the temporary file in case something goes
     wrong.  */
  on_exit (remove_tempfile, NULL);

  /* Create the ELF file data for the output file.  */
  Elf *elf = ld_state.outelf = elf_begin (fd,
					  conserve_memory
					  ? ELF_C_WRITE : ELF_C_WRITE_MMAP,
					  NULL);
  if (elf == NULL)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot create ELF descriptor for output file: %s"),
	   elf_errmsg (-1));

  /* Create the basic data structures.  */
  if (! xelf_newehdr (elf, klass))
    /* Couldn't create the ELF header.  Very bad.  */
    error (EXIT_FAILURE, 0,
	   gettext ("could not create ELF header for output file: %s"),
	   elf_errmsg (-1));

  /* And get the current header so that we can modify it.  */
  XElf_Ehdr_vardef (ehdr);
  xelf_getehdr (elf, ehdr);
  assert (ehdr != NULL);

  /* Set the machine type.  */
  ehdr->e_machine = machine;

  /* Modify it according to the info we have here and now.  */
  if (ld_state.file_type == executable_file_type)
    ehdr->e_type = ET_EXEC;
  else if (ld_state.file_type == dso_file_type)
    ehdr->e_type = ET_DYN;
  else
    {
      assert (ld_state.file_type == relocatable_file_type);
      ehdr->e_type = ET_REL;
    }

  /* Set the ELF version.  */
  ehdr->e_version = EV_CURRENT;

  /* Set the endianness.  */
  ehdr->e_ident[EI_DATA] = data;

  /* Write the ELF header information back.  */
  (void) xelf_update_ehdr (elf, ehdr);

  return 0;
}


/* We compute the offsets of the various copied objects and the total
   size of the memory needed.  */
// XXX The method used here is simple: go from front to back and pack
// the objects in this order.  A more space efficient way would
// actually trying to pack the objects as dense as possible.  But this
// is more expensive.
static void
compute_copy_reloc_offset (XElf_Shdr *shdr)
{
  struct symbol *runp = ld_state.from_dso;
  assert (runp != NULL);

  XElf_Off maxalign = 1;
  XElf_Off offset = 0;

  do
    if (runp->need_copy)
      {
	/* Determine alignment for the symbol.  */
	// XXX The question is how?  The symbol record itself does not
	// have the information.  So we have to be conservative and
	// assume the alignment of the section the symbol is in.

	// XXX We can be more precise.  Use the offset from the beginning
	// of the section and determine the largest power of two with
	// module zero.
	XElf_Off symalign = MAX (SCNINFO_SHDR (runp->file->scninfo[runp->scndx].shdr).sh_addralign, 1);
	/* Keep track of the maximum alignment requirement.  */
	maxalign = MAX (maxalign, symalign);

	/* Align current position.  */
	offset = (offset + symalign - 1) & ~(symalign - 1);

	runp->merge.value = offset;

	offset += runp->size;
      }
  while ((runp = runp->next) != ld_state.from_dso);

  shdr->sh_type = SHT_NOBITS;
  shdr->sh_size = offset;
  shdr->sh_addralign = maxalign;
}


static void
compute_common_symbol_offset (XElf_Shdr *shdr)
{
  struct symbol *runp = ld_state.common_syms;
  assert (runp != NULL);

  XElf_Off maxalign = 1;
  XElf_Off offset = 0;

  do
    {
      /* Determine alignment for the symbol.  */
      XElf_Off symalign = runp->merge.value;

      /* Keep track of the maximum alignment requirement.  */
      maxalign = MAX (maxalign, symalign);

      /* Align current position.  */
      offset = (offset + symalign - 1) & ~(symalign - 1);

      runp->merge.value = offset;

      offset += runp->size;
    }
  while ((runp = runp->next) != ld_state.common_syms);

  shdr->sh_type = SHT_NOBITS;
  shdr->sh_size = offset;
  shdr->sh_addralign = maxalign;
}


static void
sort_sections_generic (void)
{
  /* XXX TBI */
  abort ();
}


static int
match_section (const char *osectname, struct filemask_section_name *sectmask,
	       struct scnhead **scnhead, bool new_section, size_t segment_nr)
{
  struct scninfo *prevp;
  struct scninfo *runp;
  struct scninfo *notused;

  if (fnmatch (sectmask->section_name->name, (*scnhead)->name, 0) != 0)
    /* The section name does not match.  */
    return new_section;

  /* If this is a section generated by the linker it doesn't contain
     the regular information (i.e., input section data etc) and must
     be handle special.  */
  if ((*scnhead)->kind != scn_normal)
    {
      (*scnhead)->name = osectname;
      (*scnhead)->segment_nr = segment_nr;

      /* We have to count note section since they get their own
	 program header entry.  */
      if ((*scnhead)->type == SHT_NOTE)
	++ld_state.nnotesections;

      ld_state.allsections[ld_state.nallsections++] = (*scnhead);
      return true;
    }

  /* Now we have to match the file names of the input files.  Some of
     the sections here might not match.    */
  runp = (*scnhead)->last->next;
  prevp = (*scnhead)->last;
  notused = NULL;

  do
    {
      /* Base of the file name the section comes from.  */
      const char *brfname = basename (runp->fileinfo->rfname);

      /* If the section isn't used, the name doesn't match the positive
	 inclusion list, or the name does match the negative inclusion
	 list, ignore the section.  */
      if (!runp->used
	  || (sectmask->filemask != NULL
	      && fnmatch (sectmask->filemask, brfname, 0) != 0)
	  || (sectmask->excludemask != NULL
	      && fnmatch (sectmask->excludemask, brfname, 0) == 0))
	{
	  /* This file does not match the file name masks.  */
	  if (notused == NULL)
	    notused = runp;

	  prevp = runp;
	  runp = runp->next;
	  if (runp == notused)
	    runp = NULL;
	}
      /* The section fulfills all requirements, add it to the output
	 file with the correct section name etc.  */
      else
	{
	  struct scninfo *found = runp;

	  /* Remove this input section data buffer from the list.  */
	  if (prevp != runp)
	    runp = prevp->next = runp->next;
	  else
	    {
	      free (*scnhead);
	      *scnhead = NULL;
	      runp = NULL;
	    }

	  /* Create a new section for the output file if the 'new_section'
	     flag says so.  Otherwise append the buffer to the last
	     section which we created in one of the last calls.  */
	  if (new_section)
	    {
	      struct scnhead *newp;

	      newp = (struct scnhead *) obstack_calloc (&ld_state.smem,
							sizeof (*newp));
	      newp->kind = scn_normal;
	      newp->name = osectname;
	      newp->type = SCNINFO_SHDR (found->shdr).sh_type;
	      /* Executable or DSO do not have section groups.  Drop that
		 information.  */
	      newp->flags = SCNINFO_SHDR (found->shdr).sh_flags & ~SHF_GROUP;
	      newp->segment_nr = segment_nr;
	      newp->last = found->next = found;
	      newp->used = true;
	      newp->relsize = found->relsize;
	      newp->entsize = SCNINFO_SHDR (found->shdr).sh_entsize;

	      /* We have to count note section since they get their own
		 program header entry.  */
	      if (newp->type == SHT_NOTE)
		++ld_state.nnotesections;

	      ld_state.allsections[ld_state.nallsections++] = newp;
	      new_section = false;
	    }
	  else
	    {
	      struct scnhead *queued;

	      queued = ld_state.allsections[ld_state.nallsections - 1];

	      found->next = queued->last->next;
	      queued->last = queued->last->next = found;

	      /* If the linker script forces us to add incompatible
		 sections together do so.  But reflect this in the
		 type and flags of the resulting file.  */
	      if (queued->type != SCNINFO_SHDR (found->shdr).sh_type)
		/* XXX Any better choice?  */
		queued->type = SHT_PROGBITS;
	      if (queued->flags != SCNINFO_SHDR (found->shdr).sh_flags)
		/* Executable or DSO do not have section groups.  Drop that
		   information.  */
		queued->flags = ebl_sh_flags_combine (ld_state.ebl,
						      queued->flags,
						      SCNINFO_SHDR (found->shdr).sh_flags
						      & ~SHF_GROUP);

	      /* Accumulate the relocation section size.  */
	      queued->relsize += found->relsize;
	    }
	}
    }
  while (runp != NULL);

  return new_section;
}


static void
sort_sections_lscript (void)
{
  struct scnhead *temp[ld_state.nallsections];

  /* Make a copy of the section head pointer array.  */
  memcpy (temp, ld_state.allsections,
	  ld_state.nallsections * sizeof (temp[0]));
  size_t nallsections = ld_state.nallsections;

  /* Convert the output segment list in a single-linked list.  */
  struct output_segment *segment = ld_state.output_segments->next;
  ld_state.output_segments->next = NULL;
  ld_state.output_segments = segment;

  /* Put the sections in the correct order in the array in the state
     structure.  This might involve merging of sections and also
     renaming the containing section in the output file.  */
  ld_state.nallsections = 0;
  size_t segment_nr;
  size_t last_writable = ~0ul;
  for (segment_nr = 0; segment != NULL; segment = segment->next, ++segment_nr)
    {
      struct output_rule *orule;

      for (orule = segment->output_rules; orule != NULL; orule = orule->next)
	if (orule->tag == output_section)
	  {
	    struct input_rule *irule;
	    bool new_section = true;

	    for (irule = orule->val.section.input; irule != NULL;
		 irule = irule->next)
	      if (irule->tag == input_section)
		{
		  size_t cnt;

		  for (cnt = 0; cnt < nallsections; ++cnt)
		    if (temp[cnt] != NULL)
		      new_section =
			match_section (orule->val.section.name,
				       irule->val.section, &temp[cnt],
				       new_section, segment_nr);
		}
	  }

      if ((segment->mode & PF_W) != 0)
	last_writable = ld_state.nallsections - 1;
    }

  /* In case we have to create copy relocations or we have common
     symbols, find the last writable segment and add one more data
     block.  It will be a NOBITS block and take up no disk space.
     This is why it is important to get the last block.  */
  if (ld_state.ncopy > 0 || ld_state.common_syms !=  NULL)
    {
      if (last_writable == ~0ul)
	error (EXIT_FAILURE, 0, "no writable segment");

      if (ld_state.allsections[last_writable]->type != SHT_NOBITS)
	{
	  /* Make room in the ALLSECTIONS array for a new section.
	     There is guaranteed room in the array.  We add the new
	     entry after the last writable section.  */
	  ++last_writable;
	  memmove (&ld_state.allsections[last_writable + 1],
		   &ld_state.allsections[last_writable],
		   (ld_state.nallsections - last_writable)
		   * sizeof (ld_state.allsections[0]));

	  ld_state.allsections[last_writable] = (struct scnhead *)
	    obstack_calloc (&ld_state.smem, sizeof (struct scnhead));

	  /* Name for the new section.  */
	  ld_state.allsections[last_writable]->name = ".bss";
	  /* Type: NOBITS.  */
	  ld_state.allsections[last_writable]->type = SHT_NOBITS;
	  /* Same segment as the last writable section.  */
	  ld_state.allsections[last_writable]->segment_nr
	    = ld_state.allsections[last_writable - 1]->segment_nr;
	}
    }

  /* Create common symbol data block.  */
  if (ld_state.ncopy > 0)
    {
#if NATIVE_ELF
      struct scninfo *si = (struct scninfo *)
	obstack_calloc (&ld_state.smem, sizeof (*si) + sizeof (XElf_Shdr));
      si->shdr = (XElf_Shdr *) (si + 1);
#else
      struct scninfo *si = (struct scninfo *) obstack_calloc (&ld_state.smem,
							      sizeof (*si));
#endif

      /* Get the information regarding the symbols with copy relocations.  */
      compute_copy_reloc_offset (&SCNINFO_SHDR (si->shdr));

      /* This section is needed.  */
      si->used = true;
      /* Remember for later the section data structure.  */
      ld_state.copy_section = si;

      if (likely (ld_state.allsections[last_writable]->last != NULL))
	{
	  si->next = ld_state.allsections[last_writable]->last->next;
	  ld_state.allsections[last_writable]->last->next = si;
	  ld_state.allsections[last_writable]->last = si;
	}
      else
	ld_state.allsections[last_writable]->last = si->next = si;
    }

  /* Create common symbol data block.  */
  if (ld_state.common_syms != NULL)
    {
#if NATIVE_ELF
      struct scninfo *si = (struct scninfo *)
	obstack_calloc (&ld_state.smem, sizeof (*si) + sizeof (XElf_Shdr));
      si->shdr = (XElf_Shdr *) (si + 1);
#else
      struct scninfo *si = (struct scninfo *) obstack_calloc (&ld_state.smem,
							      sizeof (*si));
#endif

      /* Get the information regarding the symbols with copy relocations.  */
      compute_common_symbol_offset (&SCNINFO_SHDR (si->shdr));

      /* This section is needed.  */
      si->used = true;
      /* Remember for later the section data structure.  */
      ld_state.common_section = si;

      if (likely (ld_state.allsections[last_writable]->last != NULL))
	{
	  si->next = ld_state.allsections[last_writable]->last->next;
	  ld_state.allsections[last_writable]->last->next = si;
	  ld_state.allsections[last_writable]->last = si;
	}
      else
	ld_state.allsections[last_writable]->last = si->next = si;
    }
}


/* Create the output sections now.  This requires knowledge about all
   the sections we will need.  It may be necessary to sort sections in
   the order they are supposed to appear in the executable.  The
   sorting use many different kinds of information to optimize the
   resulting binary.  Important is to respect segment boundaries and
   the needed alignment.  The mode of the segments will be determined
   afterwards automatically by the output routines.

   The generic sorting routines work in one of two possible ways:

   - if a linker script specifies the sections to be used in the
     output and assigns them to a segment this information is used;

   - otherwise the linker will order the sections based on permissions
     and some special knowledge about section names.*/
static void
ld_generic_create_sections (struct ld_state *statep)
{
  struct scngroup *groups;
  size_t cnt;

  /* For relocatable object we don't have to bother sorting the
     sections and we do want to preserve the relocation sections as
     they appear in the input files.  */
  if (ld_state.file_type != relocatable_file_type)
    {
      /* Collect all the relocation sections.  They are handled
	 separately.  */
      struct scninfo *list = NULL;
      for (cnt = 0; cnt < ld_state.nallsections; ++cnt)
	if ((ld_state.allsections[cnt]->type == SHT_REL
	     || ld_state.allsections[cnt]->type == SHT_RELA)
	    /* The generated relocation sections are not of any
	       interest here.  */
	    && ld_state.allsections[cnt]->last != NULL)
	  {
	    if (list == NULL)
	      list = ld_state.allsections[cnt]->last;
	    else
	      {
		/* Merge the sections list.  */
		struct scninfo *first = list->next;
		list->next = ld_state.allsections[cnt]->last->next;
		ld_state.allsections[cnt]->last->next = first;
		list = ld_state.allsections[cnt]->last;
	      }

	    /* Remove the entry from the section list.  */
	    ld_state.allsections[cnt] = NULL;
	  }
      ld_state.rellist = list;

      if (ld_state.output_segments == NULL)
	/* Sort using builtin rules.  */
	sort_sections_generic ();
      else
	sort_sections_lscript ();
    }

  /* Now iterate over the input sections and create the sections in the
     order they are required in the output file.  */
  for (cnt = 0; cnt < ld_state.nallsections; ++cnt)
    {
      struct scnhead *head = ld_state.allsections[cnt];
      Elf_Scn *scn;
      XElf_Shdr_vardef (shdr);

      /* Don't handle unused sections.  */
      if (!head->used)
	continue;

      /* We first have to create the section group if necessary.
	 Section group sections must come (in section index order)
	 before any of the section contained.  This all is necessary
	 only for relocatable object as other object types are not
	 allowed to contain section groups.  */
      if (ld_state.file_type == relocatable_file_type
	  && unlikely (head->flags & SHF_GROUP))
	{
	  /* There is at least one section which is contained in a
	     section group in the input file.  This means we must
	     create a section group here as well.  The only problem is
	     that not all input files have to have to same kind of
	     partitioning of the sections.  I.e., sections A and B in
	     one input file and sections B and C in another input file
	     can be in one group.  That will result in a group
	     containing the sections A, B, and C in the output
	     file.  */
	  struct scninfo *runp;
	  Elf32_Word here_groupidx = 0;
	  struct scngroup *here_group;
	  struct member *newp;

	  /* First check whether any section is already in a group.
	     In this case we have to add this output section, too.  */
	  runp = head->last;
	  do
	    {
	      assert (runp->grpid != 0);

	      here_groupidx = runp->fileinfo->scninfo[runp->grpid].outscnndx;
	      if (here_groupidx != 0)
		break;
	    }
	  while ((runp = runp->next) != head->last);

	  if (here_groupidx == 0)
	    {
	      /* We need a new section group section.  */
	      scn = elf_newscn (ld_state.outelf);
	      xelf_getshdr (scn, shdr);
	      if (shdr == NULL)
		error (EXIT_FAILURE, 0,
		       gettext ("cannot create section for output file: %s"),
		       elf_errmsg (-1));

	      here_group = (struct scngroup *) xmalloc (sizeof (*here_group));
	      here_group->outscnidx = here_groupidx = elf_ndxscn (scn);
	      here_group->nscns = 0;
	      here_group->member = NULL;
	      here_group->next = ld_state.groups;
	      /* Pick a name for the section.  To keep it meaningful
		 we use a name used in the input files.  If the
		 section group in the output file should contain
		 section which were in section groups of different
		 names in the input files this is the users
		 problem.  */
	      here_group->nameent
		= ebl_strtabadd (ld_state.shstrtab,
				 elf_strptr (runp->fileinfo->elf,
					     runp->fileinfo->shstrndx,
					     SCNINFO_SHDR (runp->shdr).sh_name),
				 0);
	      /* Signature symbol.  */
	      here_group->symbol
		= runp->fileinfo->scninfo[runp->grpid].symbols;

	      ld_state.groups = here_group;
	    }
	  else
	    {
	      /* Search for the group with this index.  */
	      here_group = ld_state.groups;
	      while (here_group->outscnidx != here_groupidx)
		here_group = here_group->next;
	    }

	  /* Add the new output section.  */
	  newp = (struct member *) alloca (sizeof (*newp));
	  newp->scn = head;
#ifndef NDT_NEEDED
	  newp->next = NULL;
#endif
	  CSNGL_LIST_ADD_REAR (here_group->member, newp);
	  ++here_group->nscns;

	  /* Store the section group index in all input files.  */
	  runp = head->last;
	  do
	    {
	      assert (runp->grpid != 0);

	      if (runp->fileinfo->scninfo[runp->grpid].outscnndx == 0)
		runp->fileinfo->scninfo[runp->grpid].outscnndx = here_groupidx;
	      else
		assert (runp->fileinfo->scninfo[runp->grpid].outscnndx
			== here_groupidx);
	    }
	  while ((runp = runp->next) != head->last);
	}

      /* We'll use this section so get it's name in the section header
	 string table.  */
      if (head->kind == scn_normal)
	head->nameent = ebl_strtabadd (ld_state.shstrtab, head->name, 0);

      /* Create a new section in the output file and add all data
	 from all the sections we read.  */
      scn = elf_newscn (ld_state.outelf);
      head->scnidx = elf_ndxscn (scn);
      xelf_getshdr (scn, shdr);
      if (shdr == NULL)
	error (EXIT_FAILURE, 0,
	       gettext ("cannot create section for output file: %s"),
	       elf_errmsg (-1));

      assert (head->type != SHT_NULL);
      assert (head->type != SHT_SYMTAB);
      assert (head->type != SHT_DYNSYM || head->kind != scn_normal);
      assert (head->type != SHT_STRTAB || head->kind != scn_normal);
      assert (head->type != SHT_GROUP);
      shdr->sh_type = head->type;
      shdr->sh_flags = head->flags;
      shdr->sh_addralign = head->align;
      shdr->sh_entsize = head->entsize;
      assert (shdr->sh_entsize != 0 || (shdr->sh_flags & SHF_MERGE) == 0);
      (void) xelf_update_shdr (scn, shdr);

      /* We have to know the section index of the dynamic symbol table
	 right away.  */
      if (head->kind == scn_dot_dynsym)
	ld_state.dynsymscnidx = elf_ndxscn (scn);
    }

  /* Actually create the section group sections.  */
  groups = ld_state.groups;
  while (groups != NULL)
    {
      Elf_Scn *scn;
      Elf_Data *data;
      Elf32_Word *grpdata;
      struct member *runp;

      scn = elf_getscn (ld_state.outelf, groups->outscnidx);
      assert (scn != NULL);

      data = elf_newdata (scn);
      if (data == NULL)
	error (EXIT_FAILURE, 0,
	       gettext ("cannot create section for output file: %s"),
	       elf_errmsg (-1));

      data->d_size = (groups->nscns + 1) * sizeof (Elf32_Word);
      data->d_buf = grpdata = (Elf32_Word *) xmalloc (data->d_size);
      data->d_type = ELF_T_WORD;
      data->d_version = EV_CURRENT;
      data->d_off = 0;
      /* XXX What better to use?  */
      data->d_align = sizeof (Elf32_Word);

      /* The first word in the section is the flag word.  */
      /* XXX Set COMDATA flag is necessary.  */
      grpdata[0] = 0;

      runp = groups->member->next;
      cnt = 1;
      do
	/* Fill in the index of the section.  */
	grpdata[cnt++] = runp->scn->scnidx;
      while ((runp = runp->next) != groups->member->next);

      groups = groups->next;
    }
}


static bool
reduce_symbol_p (XElf_Sym *sym, struct Ebl_Strent *strent)
{
  const char *str;
  const char *version;
  struct id_list search;
  struct id_list *verp;
  bool result = ld_state.default_bind_local;

  if (XELF_ST_BIND (sym->st_info) == STB_LOCAL || sym->st_shndx == SHN_UNDEF)
    /* We don't have to do anything to local symbols here.  */
    /* XXX Any section value in [SHN_LORESERVER,SHN_XINDEX) need
       special treatment?  */
    return false;

  /* XXX Handle other symbol bindings.  */
  assert (XELF_ST_BIND (sym->st_info) == STB_GLOBAL
	  || XELF_ST_BIND (sym->st_info) == STB_WEAK);

  str = ebl_string (strent);
  version = strchr (str, VER_CHR);
  if (version != NULL)
    {
      search.id = strndupa (str, version - str);
      if (*++version == VER_CHR)
	/* Skip the second '@' signaling a default definition.  */
	++version;
    }
  else
    {
      search.id = str;
      version = "";
    }

  verp = ld_version_str_tab_find (&ld_state.version_str_tab,
				  elf_hash (search.id), &search);
  while (verp != NULL)
    {
      /* We have this symbol in the version hash table.  Now match the
	 version name.  */
      if (strcmp (verp->u.s.versionname, version) == 0)
	/* Match!  */
	return verp->u.s.local;

      verp = verp->next;
    }

  /* XXX Add test for wildcard version symbols.  */

  return result;
}


static XElf_Addr
eval_expression (struct expression *expr, XElf_Addr addr)
{
  XElf_Addr val = ~((XElf_Addr) 0);

  switch (expr->tag)
    {
    case exp_num:
      val = expr->val.num;
      break;

    case exp_sizeof_headers:
      {
	/* The 'elf_update' call determine the offset of the first
	   section.  The the size of the header.  */
	XElf_Shdr_vardef (shdr);

	xelf_getshdr (elf_getscn (ld_state.outelf, 1), shdr);
	assert (shdr != NULL);

	val = shdr->sh_offset;
      }
      break;

    case exp_pagesize:
      val = ld_state.pagesize;
      break;

    case exp_id:
      /* We are here computing only address expressions.  It seems not
	 to be necessary to handle any variable but ".".  Let's avoid
	 the complication.  If it turns up to be needed we can add
	 it.  */
      if (strcmp (expr->val.str, ".") != 0)
	error (EXIT_FAILURE, 0, gettext ("\
address computation expression contains variable '%s'"),
	       expr->val.str);

      val = addr;
      break;

    case exp_mult:
      val = (eval_expression (expr->val.binary.left, addr)
	     * eval_expression (expr->val.binary.right, addr));
      break;

    case exp_div:
      val = (eval_expression (expr->val.binary.left, addr)
	     / eval_expression (expr->val.binary.right, addr));
      break;

    case exp_mod:
      val = (eval_expression (expr->val.binary.left, addr)
	     % eval_expression (expr->val.binary.right, addr));
      break;

    case exp_plus:
      val = (eval_expression (expr->val.binary.left, addr)
	     + eval_expression (expr->val.binary.right, addr));
      break;

    case exp_minus:
      val = (eval_expression (expr->val.binary.left, addr)
	     - eval_expression (expr->val.binary.right, addr));
      break;

    case exp_and:
      val = (eval_expression (expr->val.binary.left, addr)
	     & eval_expression (expr->val.binary.right, addr));
      break;

    case exp_or:
      val = (eval_expression (expr->val.binary.left, addr)
	     | eval_expression (expr->val.binary.right, addr));
      break;

    case exp_align:
      val = eval_expression (expr->val.child, addr);
      if ((val & (val - 1)) != 0)
	error (EXIT_FAILURE, 0, gettext ("argument '%" PRIuMAX "' of ALIGN in address computation expression is no power of two"),
	       (uintmax_t) val);
      val = (addr + val - 1) & ~(val - 1);
      break;
    }

  return val;
}


/* Find a good as possible size for the hash table so that all the
   non-zero entries in HASHCODES don't collide too much and the table
   isn't too large.  There is no exact formular for this so we use a
   heuristic.  Depending on the optimization level the search is
   longer or shorter.  */
static size_t
optimal_bucket_size (Elf32_Word *hashcodes, size_t maxcnt, int optlevel)
{
  size_t minsize;
  size_t maxsize;
  size_t bestsize;
  uint64_t bestcost;
  size_t size;
  uint32_t *counts;
  uint32_t *lengths;

  if (maxcnt == 0)
    return 0;

  /* When we are not optimizing we run only very few tests.  */
  if (optlevel <= 0)
    {
      minsize = maxcnt;
      maxsize = maxcnt + 10000 / maxcnt;
    }
  else
    {
      /* Does not make much sense to start with a smaller table than
	 one which has at least four collisions.  */
      minsize = MAX (1, maxcnt / 4);
      /* We look for a best fit in the range of up to eigth times the
	 number of elements.  */
      maxsize = 2 * maxcnt + (6 * MIN (optlevel, 100) * maxcnt) / 100;
    }
  bestsize = maxcnt;
  bestcost = UINT_MAX;

  /* Array for counting the collisions and chain lengths.  */
  counts = (uint32_t *) xmalloc ((maxcnt + 1 + maxsize) * sizeof (uint32_t));
  lengths = &counts[maxcnt + 1];

  for (size = minsize; size <= maxsize; ++size)
    {
      size_t inner;
      uint64_t cost;
      uint32_t maxlength;
      uint64_t success;
      uint32_t acc;
      double factor;

      memset (lengths, '\0', size * sizeof (uint32_t));
      memset (counts, '\0', (maxcnt + 1) * sizeof (uint32_t));

      /* Determine how often each hash bucket is used.  */
      assert (hashcodes[0] == 0);
      for (inner = 1; inner < maxcnt; ++inner)
	++lengths[hashcodes[inner] % size];

      /* Determine the lengths.  */
      maxlength = 0;
      for (inner = 0; inner < size; ++inner)
	{
	  ++counts[lengths[inner]];

	  if (lengths[inner] > maxlength)
	    maxlength = lengths[inner];
	}

      /* Determine successful lookup length.  */
      acc = 0;
      success = 0;
      for (inner = 0; inner <= maxlength; ++inner)
	{
	  acc += inner;
	  success += counts[inner] * acc;
	}

      /* We can compute two factors now: the average length of a
	 positive search and the average length of a negative search.
	 We count the number of comparisons which have to look at the
	 names themselves.  Recognizing that the chain ended is not
	 accounted for since it's almost for free.

	 Which lookup is more important depends on the kind of DSO.
	 If it is a system DSO like libc it is expected that most
	 lookups succeed.  Otherwise most lookups fail.  */
      if (ld_state.is_system_library)
	factor = (1.0 * (double) success / (double) maxcnt
		  + 0.3 * (double) maxcnt / (double) size);
      else
	factor = (0.3 * (double) success / (double) maxcnt
		  + 1.0 * (double) maxcnt / (double) size);

      /* Combine the lookup cost factor.  The 1/16th addend adds
	 penalties for too large table sizes.  */
      cost = (2 + maxcnt + size) * (factor + 1.0 / 16.0);

#if 0
      printf ("maxcnt = %d, size = %d, cost = %Ld, success = %g, fail = %g, factor = %g\n",
	      maxcnt, size, cost, (double) success / (double) maxcnt, (double) maxcnt / (double) size, factor);
#endif

      /* Compare with current best results.  */
      if (cost < bestcost)
	{
	  bestcost = cost;
	  bestsize = size;
	}
    }

  free (counts);

  return bestsize;
}


static void
optimal_gnu_hash_size (Elf32_Word *hashcodes, size_t maxcnt, int optlevel,
		       size_t *bitmask_nwords, size_t *shift, size_t *nbuckets)
{
  // XXX Implement something real
  *bitmask_nwords = 256;
  *shift = 6;
  *nbuckets = 3 * maxcnt / 2;
}


static XElf_Addr
find_entry_point (void)
{
  XElf_Addr result;

  if (ld_state.entry != NULL)
    {
      struct symbol search = { .name = ld_state.entry };
      struct symbol *syment;

      syment = ld_symbol_tab_find (&ld_state.symbol_tab,
				   elf_hash (ld_state.entry), &search);
      if (syment != NULL && syment->defined)
	{
	  /* We found the symbol.  */
	  Elf_Data *data = elf_getdata (elf_getscn (ld_state.outelf,
						    ld_state.symscnidx), NULL);

	  XElf_Sym_vardef (sym);

	  sym = NULL;
	  if (data != NULL)
	    xelf_getsym (data, ld_state.dblindirect[syment->outsymidx], sym);

	  if (sym == NULL && ld_state.need_dynsym && syment->outdynsymidx != 0)
	    {
	      /* Use the dynamic symbol table if available.  */
	      data = elf_getdata (elf_getscn (ld_state.outelf,
					      ld_state.dynsymscnidx), NULL);

	      sym = NULL;
	      if (data != NULL)
		xelf_getsym (data, syment->outdynsymidx, sym);
	    }

	  if (sym != NULL)
	    return sym->st_value;

	  /* XXX What to do if the output has no non-dynamic symbol
	     table and the dynamic symbol table does not contain the
	     symbol?  */
	  assert (ld_state.need_symtab);
	  assert (ld_state.symscnidx != 0);
	}
    }

  /* We couldn't find the symbol or none was given.  Use the first
     address of the ".text" section then.  */


  result = 0;

  /* In DSOs this is no fatal error.  They usually have no entry
     points.  In this case we set the entry point to zero, which makes
     sure it will always fail.  */
  if (ld_state.file_type == executable_file_type)
    {
      if (ld_state.entry != NULL)
	error (0, 0, gettext ("\
cannot find entry symbol '%s': defaulting to %#0*" PRIx64),
	       ld_state.entry,
	       xelf_getclass (ld_state.outelf) == ELFCLASS32 ? 10 : 18,
	       (uint64_t) result);
      else
	error (0, 0, gettext ("\
no entry symbol specified: defaulting to %#0*" PRIx64),
	       xelf_getclass (ld_state.outelf) == ELFCLASS32 ? 10 : 18,
	       (uint64_t) result);
    }

  return result;
}


static void
fillin_special_symbol (struct symbol *symst, size_t scnidx, size_t nsym,
		       Elf_Data *symdata, struct Ebl_Strtab *strtab)
{
  assert (ld_state.file_type != relocatable_file_type);

  XElf_Sym_vardef (sym);
  xelf_getsym_ptr (symdata, nsym, sym);

  /* The name offset will be filled in later.  */
  sym->st_name = 0;
  /* Traditionally: globally visible.  */
  sym->st_info = XELF_ST_INFO (symst->local ? STB_LOCAL : STB_GLOBAL,
			       symst->type);
  sym->st_other = symst->hidden ? STV_HIDDEN : STV_DEFAULT;
  /* Reference to the GOT or dynamic section.  Since the GOT and
     dynamic section are only created for executables and DSOs it
     cannot be that the section index is too large.  */
  assert (scnidx != 0);
  assert (scnidx < SHN_LORESERVE || scnidx == SHN_ABS);
  sym->st_shndx = scnidx;
  /* We want the beginning of the section.  */
  sym->st_value = 0;
  // XXX What size?
  sym->st_size = 0;

  /* Determine the size of the section.  */
  if (scnidx != SHN_ABS)
    {
      Elf_Data *data = elf_getdata (elf_getscn (ld_state.outelf, scnidx),
				    NULL);
      assert (data != NULL);
      sym->st_size = data->d_size;
      /* Make sure there is no second data block.  */
      assert (elf_getdata (elf_getscn (ld_state.outelf, scnidx), data)
	      == NULL);
    }

  /* Insert symbol into the symbol table.  Note that we do not have to
     use xelf_update_symshdx.  */
  (void) xelf_update_sym (symdata, nsym, sym);

  /* Cross-references.  */
  ndxtosym[nsym] = symst;
  symst->outsymidx = nsym;

  /* Add the name to the string table.  */
  symstrent[nsym] = ebl_strtabadd (strtab, symst->name, 0);
}


static void
new_dynamic_entry (Elf_Data *data, int idx, XElf_Sxword tag, XElf_Addr val)
{
  XElf_Dyn_vardef (dyn);
  xelf_getdyn_ptr (data, idx, dyn);
  dyn->d_tag = tag;
  dyn->d_un.d_ptr = val;
  (void) xelf_update_dyn (data, idx, dyn);
}


static void
allocate_version_names (struct usedfiles *runp, struct Ebl_Strtab *dynstrtab)
{
  /* If this DSO has no versions skip it.  */
  if (runp->status != opened || runp->verdefdata == NULL)
    return;

  /* Add the object name.  */
  int offset = 0;
  while (1)
    {
      XElf_Verdef_vardef (def);
      XElf_Verdaux_vardef (aux);

      /* Get data at the next offset.  */
      xelf_getverdef (runp->verdefdata, offset, def);
      assert (def != NULL);
      xelf_getverdaux (runp->verdefdata, offset + def->vd_aux, aux);
      assert (aux != NULL);

      assert (def->vd_ndx <= runp->nverdef);
      if (def->vd_ndx == 1 || runp->verdefused[def->vd_ndx] != 0)
	{
	  runp->verdefent[def->vd_ndx]
	    = ebl_strtabadd (dynstrtab, elf_strptr (runp->elf,
						    runp->dynsymstridx,
						    aux->vda_name), 0);

	  if (def->vd_ndx > 1)
	    runp->verdefused[def->vd_ndx] = ld_state.nextveridx++;
	}

      if (def->vd_next == 0)
	/* That were all versions.  */
	break;

      offset += def->vd_next;
    }
}


static XElf_Off
create_verneed_data (XElf_Off offset, Elf_Data *verneeddata,
		     struct usedfiles *runp, int *ntotal)
{
  size_t verneed_size = xelf_fsize (ld_state.outelf, ELF_T_VNEED, 1);
  size_t vernaux_size = xelf_fsize (ld_state.outelf, ELF_T_VNAUX, 1);
  int need_offset;
  bool filled = false;
  GElf_Verneed verneed;
  GElf_Vernaux vernaux;
  int ndef = 0;
  size_t cnt;

  /* If this DSO has no versions skip it.  */
  if (runp->nverdefused == 0)
    return offset;

  /* We fill in the Verneed record last.  Remember the offset.  */
  need_offset = offset;
  offset += verneed_size;

  for (cnt = 2; cnt <= runp->nverdef; ++cnt)
    if (runp->verdefused[cnt] != 0)
      {
	assert (runp->verdefent[cnt] != NULL);

	if (filled)
	  {
	    vernaux.vna_next = vernaux_size;
	    (void) gelf_update_vernaux (verneeddata, offset, &vernaux);
	    offset += vernaux_size;
	  }

	vernaux.vna_hash = elf_hash (ebl_string (runp->verdefent[cnt]));
	vernaux.vna_flags = 0;
	vernaux.vna_other = runp->verdefused[cnt];
	vernaux.vna_name = ebl_strtaboffset (runp->verdefent[cnt]);
	filled = true;
	++ndef;
      }

  assert (filled);
  vernaux.vna_next = 0;
  (void) gelf_update_vernaux (verneeddata, offset, &vernaux);
  offset += vernaux_size;

  verneed.vn_version = VER_NEED_CURRENT;
  verneed.vn_cnt = ndef;
  verneed.vn_file = ebl_strtaboffset (runp->verdefent[1]);
  /* The first auxiliary entry is always found directly
     after the verneed entry.  */
  verneed.vn_aux = verneed_size;
  verneed.vn_next = --*ntotal > 0 ? offset - need_offset : 0;
  (void) gelf_update_verneed (verneeddata, need_offset, &verneed);

  return offset;
}


/* Callback for qsort to sort dynamic string table.  */
static Elf32_Word *global_hashcodes;
static size_t global_nbuckets;
static int
sortfct_hashval (const void *p1, const void *p2)
{
  size_t idx1 = *(size_t *) p1;
  size_t idx2 = *(size_t *) p2;

  int def1 = ndxtosym[idx1]->defined && !ndxtosym[idx1]->in_dso;
  int def2 = ndxtosym[idx2]->defined && !ndxtosym[idx2]->in_dso;

  if (! def1 && def2)
    return -1;
  if (def1 && !def2)
    return 1;
  if (! def1)
    return 0;

  Elf32_Word hval1 = (global_hashcodes[ndxtosym[idx1]->outdynsymidx]
		      % global_nbuckets);
  Elf32_Word hval2 = (global_hashcodes[ndxtosym[idx2]->outdynsymidx]
		      % global_nbuckets);

  if (hval1 < hval2)
    return -1;
  if (hval1 > hval2)
    return 1;
  return 0;
}


/* Sort the dynamic symbol table.  The GNU hash table lookup assumes
   that all symbols with the same hash value module the bucket table
   size follow one another.  This avoids the extra hash chain table.
   There is no need (and no way) to perform this operation if we do
   not use the new hash table format.  */
static void
create_gnu_hash (size_t nsym_local, size_t nsym, size_t nsym_dyn,
		 Elf32_Word *gnuhashcodes)
{
  size_t gnu_bitmask_nwords = 0;
  size_t gnu_shift = 0;
  size_t gnu_nbuckets = 0;
  Elf32_Word *gnu_bitmask = NULL;
  Elf32_Word *gnu_buckets = NULL;
  Elf32_Word *gnu_chain = NULL;
  XElf_Shdr_vardef (shdr);

  /* Determine the "optimal" bucket size.  */
  optimal_gnu_hash_size (gnuhashcodes, nsym_dyn, ld_state.optlevel,
			 &gnu_bitmask_nwords, &gnu_shift, &gnu_nbuckets);

  /* Create the .gnu.hash section data structures.  */
  Elf_Scn *hashscn = elf_getscn (ld_state.outelf, ld_state.gnuhashscnidx);
  xelf_getshdr (hashscn, shdr);
  Elf_Data *hashdata = elf_newdata (hashscn);
  if (shdr == NULL || hashdata == NULL)
    error (EXIT_FAILURE, 0, gettext ("\
cannot create GNU hash table section for output file: %s"),
	   elf_errmsg (-1));

  shdr->sh_link = ld_state.dynsymscnidx;
  (void) xelf_update_shdr (hashscn, shdr);

  hashdata->d_size = (xelf_fsize (ld_state.outelf, ELF_T_ADDR,
				  gnu_bitmask_nwords)
		      + (4 + gnu_nbuckets + nsym_dyn) * sizeof (Elf32_Word));
  hashdata->d_buf = xcalloc (1, hashdata->d_size);
  hashdata->d_align = sizeof (Elf32_Word);
  hashdata->d_type = ELF_T_WORD;
  hashdata->d_off = 0;

  ((Elf32_Word *) hashdata->d_buf)[0] = gnu_nbuckets;
  ((Elf32_Word *) hashdata->d_buf)[2] = gnu_bitmask_nwords;
  ((Elf32_Word *) hashdata->d_buf)[3] = gnu_shift;
  gnu_bitmask = &((Elf32_Word *) hashdata->d_buf)[4];
  gnu_buckets = &gnu_bitmask[xelf_fsize (ld_state.outelf, ELF_T_ADDR,
					 gnu_bitmask_nwords)
			     / sizeof (*gnu_buckets)];
  gnu_chain = &gnu_buckets[gnu_nbuckets];
#ifndef NDEBUG
  void *endp = &gnu_chain[nsym_dyn];
#endif
  assert (endp == (void *) ((char *) hashdata->d_buf + hashdata->d_size));


  size_t *remap = xmalloc (nsym_dyn * sizeof (size_t));
#ifndef NDEBUG
  size_t nsym_dyn_cnt = 1;
#endif
  for (size_t cnt = nsym_local; cnt < nsym; ++cnt)
    if (symstrent[cnt] != NULL)
      {
	assert (ndxtosym[cnt]->outdynsymidx > 0);
	assert (ndxtosym[cnt]->outdynsymidx < nsym_dyn);
	remap[ndxtosym[cnt]->outdynsymidx] = cnt;
#ifndef NDEBUG
	++nsym_dyn_cnt;
#endif
      }
  assert (nsym_dyn_cnt == nsym_dyn);

  // XXX Until we can rely on qsort_r use global variables.
  global_hashcodes = gnuhashcodes;
  global_nbuckets = gnu_nbuckets;
  qsort (remap + 1, nsym_dyn - 1, sizeof (size_t), sortfct_hashval);

  bool bm32 = (xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1)
	       ==  sizeof (Elf32_Word));

  size_t first_defined = 0;
  Elf64_Word bitmask_idxbits = gnu_bitmask_nwords - 1;
  Elf32_Word last_bucket = 0;
  for (size_t cnt = 1; cnt < nsym_dyn; ++cnt)
    {
      if (first_defined == 0)
	{
	  if (! ndxtosym[remap[cnt]]->defined
	      || ndxtosym[remap[cnt]]->in_dso)
	    goto next;

	  ((Elf32_Word *) hashdata->d_buf)[1] = first_defined = cnt;
	}

      Elf32_Word hval = gnuhashcodes[ndxtosym[remap[cnt]]->outdynsymidx];

      if (bm32)
	{
	  Elf32_Word *bsw = &gnu_bitmask[(hval / 32) & bitmask_idxbits];
	  assert ((void *) gnu_bitmask <= (void *) bsw);
	  assert ((void *) bsw < (void *) gnu_buckets);
	  *bsw |= 1 << (hval & 31);
	  *bsw |= 1 << ((hval >> gnu_shift) & 31);
	}
      else
	{
	  Elf64_Word *bsw = &((Elf64_Word *) gnu_bitmask)[(hval / 64)
							  & bitmask_idxbits];
	  assert ((void *) gnu_bitmask <= (void *) bsw);
	  assert ((void *) bsw < (void *) gnu_buckets);
	  *bsw |= 1 << (hval & 63);
	  *bsw |= 1 << ((hval >> gnu_shift) & 63);
	}

      size_t this_bucket = hval % gnu_nbuckets;
      if (cnt == first_defined || this_bucket != last_bucket)
	{
	  if (cnt != first_defined)
	    {
	      /* Terminate the previous chain.  */
	      assert ((void *) &gnu_chain[cnt - first_defined - 1] < endp);
	      gnu_chain[cnt - first_defined - 1] |= 1;
	    }

	  assert (this_bucket < gnu_nbuckets);
	  gnu_buckets[this_bucket] = cnt;
	  last_bucket = this_bucket;
	}

      assert (cnt >= first_defined);
      assert (cnt - first_defined < nsym_dyn);
      gnu_chain[cnt - first_defined] = hval & ~1u;

    next:
      ndxtosym[remap[cnt]]->outdynsymidx = cnt;
    }

  /* Terminate the last chain.  */
  if (first_defined != 0)
    {
      assert (nsym_dyn > first_defined);
      assert (nsym_dyn - first_defined - 1 < nsym_dyn);
      gnu_chain[nsym_dyn - first_defined - 1] |= 1;

      hashdata->d_size -= first_defined * sizeof (Elf32_Word);
    }
  else
    /* We do not need any hash table.  */
    // XXX
    do { } while (0);

  free (remap);
}


/* Create the SysV-style hash table.  */
static void
create_hash (size_t nsym_local, size_t nsym, size_t nsym_dyn,
	     Elf32_Word *hashcodes)
{
  size_t nbucket = 0;
  Elf32_Word *bucket = NULL;
  Elf32_Word *chain = NULL;
  XElf_Shdr_vardef (shdr);

  /* Determine the "optimal" bucket size.  If we also generate the
     new-style hash function there is no need to waste effort and
     space on the old one which should not be used.  Make it as small
     as possible.  */
  if (GENERATE_GNU_HASH)
    nbucket = 1;
  else
    nbucket = optimal_bucket_size (hashcodes, nsym_dyn, ld_state.optlevel);
  /* Create the .hash section data structures.  */
  Elf_Scn *hashscn = elf_getscn (ld_state.outelf, ld_state.hashscnidx);
  xelf_getshdr (hashscn, shdr);
  Elf_Data *hashdata = elf_newdata (hashscn);
  if (shdr == NULL || hashdata == NULL)
    error (EXIT_FAILURE, 0, gettext ("\
cannot create hash table section for output file: %s"),
	   elf_errmsg (-1));

  shdr->sh_link = ld_state.dynsymscnidx;
  (void) xelf_update_shdr (hashscn, shdr);

  hashdata->d_size = (2 + nsym_dyn + nbucket) * sizeof (Elf32_Word);
  hashdata->d_buf = xcalloc (1, hashdata->d_size);
  hashdata->d_align = sizeof (Elf32_Word);
  hashdata->d_type = ELF_T_WORD;
  hashdata->d_off = 0;

  ((Elf32_Word *) hashdata->d_buf)[0] = nbucket;
  ((Elf32_Word *) hashdata->d_buf)[1] = nsym_dyn;
  bucket = &((Elf32_Word *) hashdata->d_buf)[2];
  chain = &((Elf32_Word *) hashdata->d_buf)[2 + nbucket];

  for (size_t cnt = nsym_local; cnt < nsym; ++cnt)
    if (symstrent[cnt] != NULL)
      {
	size_t dynidx = ndxtosym[cnt]->outdynsymidx;
	size_t hashidx = hashcodes[dynidx] % nbucket;
	if (bucket[hashidx] == 0)
	  bucket[hashidx] = dynidx;
	else
	  {
	    hashidx = bucket[hashidx];
	    while (chain[hashidx] != 0)
	      hashidx = chain[hashidx];

	    chain[hashidx] = dynidx;
	  }
      }
}


static void
create_build_id_section (Elf_Scn *scn)
{
  /* We know how large the section will be so we can create it now.  */
  Elf_Data *d = elf_newdata (scn);
  if (d == NULL)
    error (EXIT_FAILURE, 0, gettext ("cannot create build ID section: %s"),
	   elf_errmsg (-1));

  d->d_type = ELF_T_BYTE;
  d->d_version = EV_CURRENT;

  /* The note section header.  */
  assert (sizeof (Elf32_Nhdr) == sizeof (Elf64_Nhdr));
  d->d_size = sizeof (GElf_Nhdr);
  /* The string is four bytes long.  */
  d->d_size += sizeof (ELF_NOTE_GNU);
  assert (d->d_size % 4 == 0);

  if (strcmp (ld_state.build_id, "md5") == 0
      || strcmp (ld_state.build_id, "uuid") == 0)
    d->d_size += 16;
  else if (strcmp (ld_state.build_id, "sha1") == 0)
    d->d_size += 20;
  else
    {
      assert (ld_state.build_id[0] == '0' && ld_state.build_id[1] == 'x');
      /* Use an upper limit of the possible number of bytes generated
	 from the string.  */
      d->d_size += strlen (ld_state.build_id) / 2;
    }

  d->d_buf = xcalloc (d->d_size, 1);
  d->d_off = 0;
  d->d_align = 0;
}


static void
compute_hash_sum (void (*hashfct) (const void *, size_t, void *), void *ctx)
{
  /* The call cannot fail.  */
  size_t shstrndx;
  (void) elf_getshdrstrndx (ld_state.outelf, &shstrndx);

  const char *ident = elf_getident (ld_state.outelf, NULL);
  bool same_byte_order = ((ident[EI_DATA] == ELFDATA2LSB
			   && __BYTE_ORDER == __LITTLE_ENDIAN)
			  || (ident[EI_DATA] == ELFDATA2MSB
			      && __BYTE_ORDER == __BIG_ENDIAN));

  /* Iterate over all sections to find those which are not strippable.  */
  Elf_Scn *scn = NULL;
  while ((scn = elf_nextscn (ld_state.outelf, scn)) != NULL)
    {
      /* Get the section header.  */
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
      assert (shdr != NULL);

      if (SECTION_STRIP_P (shdr, elf_strptr (ld_state.outelf, shstrndx,
					     shdr->sh_name), true))
	/* The section can be stripped.  Don't use it.  */
	continue;

      /* Do not look at NOBITS sections.  */
      if (shdr->sh_type == SHT_NOBITS)
	continue;

      /* Iterate through the list of data blocks.  */
      Elf_Data *data = NULL;
      while ((data = INTUSE(elf_getdata) (scn, data)) != NULL)
	/* If the file byte order is the same as the host byte order
	   process the buffer directly.  If the data is just a stream
	   of bytes which the library will not convert we can use it
	   as well.  */
	if (likely (same_byte_order) || data->d_type == ELF_T_BYTE)
	  hashfct (data->d_buf, data->d_size, ctx);
	else
	  {
	    /* Convert the data to file byte order.  */
	    if (gelf_xlatetof (ld_state.outelf, data, data, ident[EI_DATA])
		== NULL)
	      error (EXIT_FAILURE, 0, gettext ("\
cannot convert section data to file format: %s"),
		     elf_errmsg (-1));

	    hashfct (data->d_buf, data->d_size, ctx);

	    /* And convert it back.  */
	    if (gelf_xlatetom (ld_state.outelf, data, data, ident[EI_DATA])
		== NULL)
	      error (EXIT_FAILURE, 0, gettext ("\
cannot convert section data to memory format: %s"),
		     elf_errmsg (-1));
	  }
    }
}


/* Iterate over the sections */
static void
compute_build_id (void)
{
  Elf_Data *d = elf_getdata (elf_getscn (ld_state.outelf,
					 ld_state.buildidscnidx), NULL);
  assert (d != NULL);

  GElf_Nhdr *hdr = d->d_buf;
  hdr->n_namesz = sizeof (ELF_NOTE_GNU);
  hdr->n_type = NT_GNU_BUILD_ID;
  char *dp = mempcpy (hdr + 1, ELF_NOTE_GNU, sizeof (ELF_NOTE_GNU));

  if (strcmp (ld_state.build_id, "sha1") == 0)
    {
      /* Compute the SHA1 sum of various parts of the generated file.
	 We compute the hash sum over the external representation.  */
      struct sha1_ctx ctx;
      sha1_init_ctx (&ctx);

      /* Compute the hash sum by running over all sections.  */
      compute_hash_sum ((void (*) (const void *, size_t, void *)) sha1_process_bytes,
			&ctx);

      /* We are done computing the checksum.  */
      (void) sha1_finish_ctx (&ctx, dp);

      hdr->n_descsz = SHA1_DIGEST_SIZE;
    }
  else if (strcmp (ld_state.build_id, "md5") == 0)
    {
      /* Compute the MD5 sum of various parts of the generated file.
	 We compute the hash sum over the external representation.  */
      struct md5_ctx ctx;
      md5_init_ctx (&ctx);

      /* Compute the hash sum by running over all sections.  */
      compute_hash_sum ((void (*) (const void *, size_t, void *)) md5_process_bytes,
			&ctx);

      /* We are done computing the checksum.  */
      (void) md5_finish_ctx (&ctx, dp);

      hdr->n_descsz = MD5_DIGEST_SIZE;
    }
  else if (strcmp (ld_state.build_id, "uuid") == 0)
    {
      int fd = open ("/dev/urandom", O_RDONLY);
      if (fd == -1)
	error (EXIT_FAILURE, errno, gettext ("cannot open '%s'"),
	       "/dev/urandom");

      if (TEMP_FAILURE_RETRY (read (fd, dp, 16)) != 16)
	error (EXIT_FAILURE, 0, gettext ("cannot read enough data for UUID"));

      close (fd);

      hdr->n_descsz = 16;
    }
  else
    {
      const char *cp = ld_state.build_id + 2;

      /* The form of the string has been verified before so here we can
	 simplify the scanning.  */
      do
	{
	  if (isxdigit (cp[0]))
	    {
	      char ch1 = tolower (cp[0]);
	      char ch2 = tolower (cp[1]);

	      *dp++ = (((isdigit (ch1) ? ch1 - '0' : ch1 - 'a' + 10) << 4)
		       | (isdigit (ch2) ? ch2 - '0' : ch2 - 'a' + 10));
	    }
	  else
	    ++cp;
	}
      while (*cp != '\0');
    }
}


/* Create the output file.

   For relocatable files what basically has to happen is that all
   sections from all input files are written into the output file.
   Sections with the same name are combined (offsets adjusted
   accordingly).  The symbol tables are combined in one single table.
   When stripping certain symbol table entries are omitted.

   For executables (shared or not) we have to create the program header,
   additional sections like the .interp, eventually (in addition) create
   a dynamic symbol table and a dynamic section.  Also the relocations
   have to be processed differently.  */
static int
ld_generic_create_outfile (struct ld_state *statep)
{
  struct scnlist
  {
    size_t scnidx;
    struct scninfo *scninfo;
    struct scnlist *next;
  };
  struct scnlist *rellist = NULL;
  size_t cnt;
  Elf_Scn *symscn = NULL;
  Elf_Scn *xndxscn = NULL;
  Elf_Scn *strscn = NULL;
  struct Ebl_Strtab *strtab = NULL;
  struct Ebl_Strtab *dynstrtab = NULL;
  XElf_Shdr_vardef (shdr);
  Elf_Data *data;
  Elf_Data *symdata = NULL;
  Elf_Data *xndxdata = NULL;
  struct usedfiles *file;
  size_t nsym;
  size_t nsym_local;
  size_t nsym_allocated;
  size_t nsym_dyn = 0;
  Elf32_Word *dblindirect = NULL;
#ifndef NDEBUG
  bool need_xndx;
#endif
  Elf_Scn *shstrtab_scn;
  size_t shstrtab_ndx;
  XElf_Ehdr_vardef (ehdr);
  struct Ebl_Strent *symtab_ent = NULL;
  struct Ebl_Strent *xndx_ent = NULL;
  struct Ebl_Strent *strtab_ent = NULL;
  struct Ebl_Strent *shstrtab_ent;
  struct scngroup *groups;
  Elf_Scn *dynsymscn = NULL;
  Elf_Data *dynsymdata = NULL;
  Elf_Data *dynstrdata = NULL;
  Elf32_Word *hashcodes = NULL;
  Elf32_Word *gnuhashcodes = NULL;
  size_t nsym_dyn_allocated = 0;
  Elf_Scn *versymscn = NULL;
  Elf_Data *versymdata = NULL;

  if (ld_state.need_symtab)
    {
      /* First create the symbol table.  We need the symbol section itself
	 and the string table for it.  */
      symscn = elf_newscn (ld_state.outelf);
      ld_state.symscnidx = elf_ndxscn (symscn);
      symdata = elf_newdata (symscn);
      if (symdata == NULL)
	error (EXIT_FAILURE, 0,
	       gettext ("cannot create symbol table for output file: %s"),
	       elf_errmsg (-1));

      symdata->d_type = ELF_T_SYM;
      /* This is an estimated size, but it will definitely cap the real value.
	 We might have to adjust the number later.  */
      nsym_allocated = (1 + ld_state.nsymtab + ld_state.nplt + ld_state.ngot
			+ ld_state.nusedsections + ld_state.nlscript_syms);
      symdata->d_size = xelf_fsize (ld_state.outelf, ELF_T_SYM,
				    nsym_allocated);

      /* Optionally the extended section table.  */
      /* XXX Is SHN_LORESERVE correct?  Do we need some other sections?  */
      if (unlikely (ld_state.nusedsections >= SHN_LORESERVE))
	{
	  xndxscn = elf_newscn (ld_state.outelf);
	  ld_state.xndxscnidx = elf_ndxscn (xndxscn);

	  xndxdata = elf_newdata (xndxscn);
	  if (xndxdata == NULL)
	    error (EXIT_FAILURE, 0,
		   gettext ("cannot create symbol table for output file: %s"),
		   elf_errmsg (-1));

	  /* The following relies on the fact that Elf32_Word and Elf64_Word
	     have the same size.  */
	  xndxdata->d_type = ELF_T_WORD;
	  /* This is an estimated size, but it will definitely cap the
	     real value.  we might have to adjust the number later.  */
	  xndxdata->d_size = xelf_fsize (ld_state.outelf, ELF_T_WORD,
					 nsym_allocated);
	  /* The first entry is left empty, clear it here and now.  */
	  xndxdata->d_buf = memset (xmalloc (xndxdata->d_size), '\0',
				    xelf_fsize (ld_state.outelf, ELF_T_WORD,
						1));
	  xndxdata->d_off = 0;
	  /* XXX Should use an ebl function.  */
	  xndxdata->d_align = sizeof (Elf32_Word);
	}
    }
  else
    {
      assert (ld_state.need_dynsym);

      /* First create the symbol table.  We need the symbol section itself
	 and the string table for it.  */
      symscn = elf_getscn (ld_state.outelf, ld_state.dynsymscnidx);
      symdata = elf_newdata (symscn);
      if (symdata == NULL)
	error (EXIT_FAILURE, 0,
	       gettext ("cannot create symbol table for output file: %s"),
	       elf_errmsg (-1));

      symdata->d_version = EV_CURRENT;
      symdata->d_type = ELF_T_SYM;
      /* This is an estimated size, but it will definitely cap the real value.
	 We might have to adjust the number later.  */
      nsym_allocated = (1 + ld_state.nsymtab + ld_state.nplt + ld_state.ngot
			- ld_state.nlocalsymbols + ld_state.nlscript_syms);
      symdata->d_size = xelf_fsize (ld_state.outelf, ELF_T_SYM,
				    nsym_allocated);
    }

  /* The first entry is left empty, clear it here and now.  */
  symdata->d_buf = memset (xmalloc (symdata->d_size), '\0',
			   xelf_fsize (ld_state.outelf, ELF_T_SYM, 1));
  symdata->d_off = 0;
  /* XXX This is ugly but how else can it be done.  */
  symdata->d_align = xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1);

  /* Allocate another array to keep track of the handles for the symbol
     names.  */
  symstrent = (struct Ebl_Strent **) xcalloc (nsym_allocated,
					      sizeof (struct Ebl_Strent *));

  /* By starting at 1 we effectively add a null entry.  */
  nsym = 1;

  /* Iteration over all sections.  */
  for (cnt = 0; cnt < ld_state.nallsections; ++cnt)
    {
      struct scnhead *head = ld_state.allsections[cnt];
      Elf_Scn *scn;
      struct scninfo *runp;
      XElf_Off offset;
      Elf32_Word xndx;

      /* Don't handle unused sections at all.  */
      if (!head->used)
	continue;

      /* Get the section handle.  */
      scn = elf_getscn (ld_state.outelf, head->scnidx);

      if (unlikely (head->kind == scn_dot_interp))
	{
	  Elf_Data *outdata = elf_newdata (scn);
	  if (outdata == NULL)
	    error (EXIT_FAILURE, 0,
		   gettext ("cannot create section for output file: %s"),
		   elf_errmsg (-1));

	  /* This is the string we'll put in the section.  */
	  const char *interp = ld_state.interp ?: "/lib/ld.so.1";

	  /* Create the section data.  */
	  outdata->d_buf = (void *) interp;
	  outdata->d_size = strlen (interp) + 1;
	  outdata->d_type = ELF_T_BYTE;
	  outdata->d_off = 0;
	  outdata->d_align = 1;
	  outdata->d_version = EV_CURRENT;

	  /* Remember the index of this section.  */
	  ld_state.interpscnidx = head->scnidx;

	  continue;
	}

      if (unlikely (head->kind == scn_dot_got))
	{
	  /* Remember the index of this section.  */
	  ld_state.gotscnidx = elf_ndxscn (scn);

	  /* Give the backend the change to initialize the section.  */
	  INITIALIZE_GOT (&ld_state, scn);

	  continue;
	}

      if (unlikely (head->kind == scn_dot_gotplt))
	{
	  /* Remember the index of this section.  */
	  ld_state.gotpltscnidx = elf_ndxscn (scn);

	  /* Give the backend the change to initialize the section.  */
	  INITIALIZE_GOTPLT (&ld_state, scn);

	  continue;
	}

      if (unlikely (head->kind == scn_dot_dynrel))
	{
	  Elf_Data *outdata;

	  outdata = elf_newdata (scn);
	  if (outdata == NULL)
	    error (EXIT_FAILURE, 0,
		   gettext ("cannot create section for output file: %s"),
		   elf_errmsg (-1));

	  outdata->d_size = ld_state.relsize_total;
	  outdata->d_buf = xmalloc (outdata->d_size);
	  outdata->d_type = (REL_TYPE (&ld_state) == DT_REL
			     ? ELF_T_REL : ELF_T_RELA);
	  outdata->d_off = 0;
	  outdata->d_align = xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1);

	  /* Remember the index of this section.  */
	  ld_state.reldynscnidx = elf_ndxscn (scn);

	  continue;
	}

      if (unlikely (head->kind == scn_dot_dynamic))
	{
	  /* Only create the data for now.  */
	  Elf_Data *outdata;

	  /* Account for a few more entries we have to add.  */
	  if (ld_state.dt_flags != 0)
	    ++ld_state.ndynamic;
	  if (ld_state.dt_flags_1 != 0)
	    ++ld_state.ndynamic;
	  if (ld_state.dt_feature_1 != 0)
	    ++ld_state.ndynamic;

	  outdata = elf_newdata (scn);
	  if (outdata == NULL)
	    error (EXIT_FAILURE, 0,
		   gettext ("cannot create section for output file: %s"),
		   elf_errmsg (-1));

	  /* Create the section data.  */
	  outdata->d_size = xelf_fsize (ld_state.outelf, ELF_T_DYN,
					ld_state.ndynamic);
	  outdata->d_buf = xcalloc (1, outdata->d_size);
	  outdata->d_type = ELF_T_DYN;
	  outdata->d_off = 0;
	  outdata->d_align = xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1);

	  /* Remember the index of this section.  */
	  ld_state.dynamicscnidx = elf_ndxscn (scn);

	  continue;
	}

      if (unlikely (head->kind == scn_dot_dynsym))
	{
	  /* We already know the section index.  */
	  assert (ld_state.dynsymscnidx == elf_ndxscn (scn));

	  continue;
	}

      if (unlikely (head->kind == scn_dot_dynstr))
	{
	  /* Remember the index of this section.  */
	  ld_state.dynstrscnidx = elf_ndxscn (scn);

	  /* Create the string table.  */
	  dynstrtab = ebl_strtabinit (true);

	  /* XXX TBI
	     We have to add all the strings which are needed in the
	     dynamic section here.  This means DT_FILTER,
	     DT_AUXILIARY, ... entries.  */
	  if (ld_state.ndsofiles > 0)
	    {
	      struct usedfiles *frunp = ld_state.dsofiles;

	      do
		if (! frunp->as_needed || frunp->used)
		  frunp->sonameent = ebl_strtabadd (dynstrtab, frunp->soname,
						    0);
	      while ((frunp = frunp->next) != ld_state.dsofiles);
	    }


	  /* Add the runtime path information.  The strings are stored
	     in the .dynstr section.  If both rpath and runpath are defined
	     the runpath information is used.  */
	  if (ld_state.runpath != NULL || ld_state.rpath != NULL)
	    {
	      struct pathelement *startp;
	      struct pathelement *prunp;
	      int tag;
	      size_t len;
	      char *str;
	      char *cp;

	      if (ld_state.runpath != NULL)
		{
		  startp = ld_state.runpath;
		  tag = DT_RUNPATH;
		}
	      else
		{
		  startp = ld_state.rpath;
		  tag = DT_RPATH;
		}

	      /* Determine how long the string will be.  */
	      for (len = 0, prunp = startp; prunp != NULL; prunp = prunp->next)
		len += strlen (prunp->pname) + 1;

	      cp = str = (char *) obstack_alloc (&ld_state.smem, len);
	      /* Copy the string.  */
	      for (prunp = startp; prunp != NULL; prunp = prunp->next)
		{
		  cp = stpcpy (cp, prunp->pname);
		  *cp++ = ':';
		}
	      /* Remove the last colon.  */
	      cp[-1] = '\0';

	      /* Remember the values until we can generate the dynamic
		 section.  */
	      ld_state.rxxpath_strent = ebl_strtabadd (dynstrtab, str, len);
	      ld_state.rxxpath_tag = tag;
	    }

	  continue;
	}

      if (unlikely (head->kind == scn_dot_hash))
	{
	  /* Remember the index of this section.  */
	  ld_state.hashscnidx = elf_ndxscn (scn);

	  continue;
	}

      if (unlikely (head->kind == scn_dot_gnu_hash))
	{
	  /* Remember the index of this section.  */
	  ld_state.gnuhashscnidx = elf_ndxscn (scn);

	  continue;
	}

      if (unlikely (head->kind == scn_dot_plt))
	{
	  /* Remember the index of this section.  */
	  ld_state.pltscnidx = elf_ndxscn (scn);

	  /* Give the backend the change to initialize the section.  */
	  INITIALIZE_PLT (&ld_state, scn);

	  continue;
	}

      if (unlikely (head->kind == scn_dot_pltrel))
	{
	  /* Remember the index of this section.  */
	  ld_state.pltrelscnidx = elf_ndxscn (scn);

	  /* Give the backend the change to initialize the section.  */
	  INITIALIZE_PLTREL (&ld_state, scn);

	  continue;
	}

      if (unlikely (head->kind == scn_dot_version))
	{
	  /* Remember the index of this section.  */
	  ld_state.versymscnidx = elf_ndxscn (scn);

	  continue;
	}

      if (unlikely (head->kind == scn_dot_version_r))
	{
	  /* Remember the index of this section.  */
	  ld_state.verneedscnidx = elf_ndxscn (scn);

	  continue;
	}

      if (unlikely (head->kind == scn_dot_note_gnu_build_id))
	{
	  /* Remember the index of this section.  */
	  ld_state.buildidscnidx = elf_ndxscn (scn);

	  create_build_id_section (scn);

	  continue;
	}

      /* If we come here we must be handling a normal section.  */
      assert (head->kind == scn_normal);

      /* Create an STT_SECTION entry in the symbol table.  But not for
	 the symbolic symbol table.  */
      if (ld_state.need_symtab)
	{
	  /* XXX Can we be cleverer and do this only if needed?  */
	  XElf_Sym_vardef (sym);

	  /* Optimization ahead: in the native linker we get a pointer
	     to the final location so that the following code writes
	     directly in the correct place.  Otherwise we write into
	     the local variable first.  */
	  xelf_getsym_ptr (symdata, nsym, sym);

	  /* Usual section symbol: local, no specific information,
	     except the section index.  The offset here is zero, the
	     start address will later be added.  */
	  sym->st_name = 0;
	  sym->st_info = XELF_ST_INFO (STB_LOCAL, STT_SECTION);
	  sym->st_other = 0;
	  sym->st_value = 0;
	  sym->st_size = 0;
	  /* In relocatable files the section index can be too big for
	     the ElfXX_Sym struct.  we have to deal with the extended
	     symbol table.  */
	  if (likely (head->scnidx < SHN_LORESERVE))
	    {
	      sym->st_shndx = head->scnidx;
	      xndx = 0;
	    }
	  else
	    {
	      sym->st_shndx = SHN_XINDEX;
	      xndx = head->scnidx;
	    }
	  /* Commit the change.  See the optimization above, this does
	     not change the symbol table entry.  But the extended
	     section index table entry is always written, if there is
	     such a table.  */
	  assert (nsym < nsym_allocated);
	  xelf_update_symshndx (symdata, xndxdata, nsym, sym, xndx, 0);

	  /* Remember the symbol's index in the symbol table.  */
	  head->scnsymidx = nsym++;
	}

      if (head->type == SHT_REL || head->type == SHT_RELA)
	{
	  /* Remember that we have to fill in the symbol table section
	     index.  */
	  if (ld_state.file_type == relocatable_file_type)
	    {
	      struct scnlist *newp;

	      newp = (struct scnlist *) alloca (sizeof (*newp));
	      newp->scnidx = head->scnidx;
	      newp->scninfo = head->last->next;
#ifndef NDEBUG
	      newp->next = NULL;
#endif
	      SNGL_LIST_PUSH (rellist, newp);
	    }
	  else
	    {
	      /* When we create an executable or a DSO we don't simply
		 copy the existing relocations.  Instead many will be
		 resolved, others will be converted.  Create a data buffer
		 large enough to contain the contents which we will fill
		 in later.  */
	      int type = head->type == SHT_REL ? ELF_T_REL : ELF_T_RELA;

	      data = elf_newdata (scn);
	      if (data == NULL)
		error (EXIT_FAILURE, 0,
		       gettext ("cannot create section for output file: %s"),
		       elf_errmsg (-1));

	      data->d_size = xelf_fsize (ld_state.outelf, type, head->relsize);
	      data->d_buf = xcalloc (data->d_size, 1);
	      data->d_type = type;
	      data->d_align = xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1);
	      data->d_off = 0;

	      continue;
	    }
	}

      /* Recognize string and merge flag and handle them.  */
      if (head->flags & SHF_MERGE)
	{
	  /* We merge the contents of the sections.  For this we do
	     not look at the contents of section directly.  Instead we
	     look at the symbols of the section.  */
	  Elf_Data *outdata;

	  /* Concatenate the lists of symbols for all sections.

	     XXX In case any input section has no symbols associated
	     (this happens for debug sections) we cannot use this
	     method.  Implement parsing the other debug sections and
	     find the string pointers.  For now we don't merge.  */
	  runp = head->last->next;
	  if (runp->symbols == NULL)
	    {
	      head->flags &= ~SHF_MERGE;
	      goto no_merge;
	    }
	  head->symbols = runp->symbols;

	  while ((runp = runp->next) != head->last->next)
	    {
	      if (runp->symbols == NULL)
		{
		  head->flags &= ~SHF_MERGE;
		  head->symbols = NULL;
		  goto no_merge;
		}

	      struct symbol *oldhead = head->symbols->next_in_scn;

	      head->symbols->next_in_scn = runp->symbols->next_in_scn;
	      runp->symbols->next_in_scn = oldhead;
	      head->symbols = runp->symbols;
	    }

	  /* Create the output section.  */
	  outdata = elf_newdata (scn);
	  if (outdata == NULL)
	    error (EXIT_FAILURE, 0,
		   gettext ("cannot create section for output file: %s"),
		   elf_errmsg (-1));

	  /* We use different merging algorithms for performance
	     reasons.  We can easily handle single-byte and
	     wchar_t-wide character strings.  All other cases (which
	     really should happen in real life) are handled by the
	     generic code.  */
	  if (SCNINFO_SHDR (head->last->shdr).sh_entsize == 1
	      && (head->flags & SHF_STRINGS))
	    {
	      /* Simple, single-byte string matching.  */
	      struct Ebl_Strtab *mergestrtab;
	      struct symbol *symrunp;
	      Elf_Data *locsymdata = NULL;
	      Elf_Data *locdata = NULL;

	      mergestrtab = ebl_strtabinit (false);

	      symrunp = head->symbols->next_in_scn;
	      file = NULL;
	      do
		{
		  /* Accelarate the loop.  We cache the file
		     information since it might very well be the case
		     that the previous entry was from the same
		     file.  */
		  if (symrunp->file != file)
		    {
		      /* Remember the file.  */
		      file = symrunp->file;
		      /* Symbol table data from that file.  */
		      locsymdata = file->symtabdata;
		      /* String section data.  */
		      locdata = elf_rawdata (file->scninfo[symrunp->scndx].scn,
					     NULL);
		      assert (locdata != NULL);
		      /* While we are at it, remember the output
			 section.  If we don't access the string data
			 section the section won't be in the output
			 file.  So it is sufficient to do the work
			 here.  */
		      file->scninfo[symrunp->scndx].outscnndx = head->scnidx;
		    }

		  /* Get the symbol information.  This provides us the
		     offset into the string data section.  */
		  XElf_Sym_vardef (sym);
		  xelf_getsym (locsymdata, symrunp->symidx, sym);
		  assert (sym != NULL);

		  /* Get the data from the file.  Note that we access
		     the raw section data; no endian-ness issues with
		     single-byte strings.  */
		  symrunp->merge.handle
		    = ebl_strtabadd (mergestrtab,
				     (char *) locdata->d_buf + sym->st_value,
				     0);
		}
	      while ((symrunp = symrunp->next_in_scn)
		     != head->symbols->next_in_scn);

	      /* All strings have been added.  Create the final table.  */
	      ebl_strtabfinalize (mergestrtab, outdata);

	      /* Compute the final offsets in the section.  */
	      symrunp = runp->symbols;
	      do
		{
		  symrunp->merge.value
		    = ebl_strtaboffset (symrunp->merge.handle);
		  symrunp->merged = 1;
		}
	      while ((symrunp = symrunp->next_in_scn) != runp->symbols);

	      /* We don't need the string table anymore.  */
	      ebl_strtabfree (mergestrtab);
	    }
	  else if (likely (SCNINFO_SHDR (head->last->shdr).sh_entsize
			   == sizeof (wchar_t))
		   && likely (head->flags & SHF_STRINGS))
	    {
	      /* Simple, wchar_t string merging.  */
	      struct Ebl_WStrtab *mergestrtab;
	      struct symbol *symrunp;
	      Elf_Data *locsymdata = NULL;
	      Elf_Data *locdata = NULL;

	      mergestrtab = ebl_wstrtabinit (false);

	      symrunp = runp->symbols;
	      file = NULL;
	      do
		{
		  /* Accelarate the loop.  We cache the file
		     information since it might very well be the case
		     that the previous entry was from the same
		     file.  */
		  if (symrunp->file != file)
		    {
		      /* Remember the file.  */
		      file = symrunp->file;
		      /* Symbol table data from that file.  */
		      locsymdata = file->symtabdata;
		      /* String section data.  */
		      locdata = elf_rawdata (file->scninfo[symrunp->scndx].scn,
					     NULL);
		      assert (locdata != NULL);

		      /* While we are at it, remember the output
			 section.  If we don't access the string data
			 section the section won't be in the output
			 file.  So it is sufficient to do the work
			 here.  */
		      file->scninfo[symrunp->scndx].outscnndx = head->scnidx;
		    }

		  /* Get the symbol information.  This provides us the
		     offset into the string data section.  */
		  XElf_Sym_vardef (sym);
		  xelf_getsym (locsymdata, symrunp->symidx, sym);
		  assert (sym != NULL);

		  /* Get the data from the file.  Using the raw
		     section data here is possible since we don't
		     interpret the string themselves except for
		     looking for the wide NUL character.  The NUL
		     character has fortunately the same representation
		     regardless of the byte order.  */
		  symrunp->merge.handle
		    = ebl_wstrtabadd (mergestrtab,
				      (wchar_t *) ((char *) locdata->d_buf
						   + sym->st_value), 0);
		}
	      while ((symrunp = symrunp->next_in_scn) != runp->symbols);

	      /* All strings have been added.  Create the final table.  */
	      ebl_wstrtabfinalize (mergestrtab, outdata);

	      /* Compute the final offsets in the section.  */
	      symrunp = runp->symbols;
	      do
		{
		  symrunp->merge.value
		    = ebl_wstrtaboffset (symrunp->merge.handle);
		  symrunp->merged = 1;
		}
	      while ((symrunp = symrunp->next_in_scn) != runp->symbols);

	      /* We don't need the string table anymore.  */
	      ebl_wstrtabfree (mergestrtab);
	    }
	  else
	    {
	      /* Non-standard merging.  */
	      struct Ebl_GStrtab *mergestrtab;
	      struct symbol *symrunp;
	      Elf_Data *locsymdata = NULL;
	      Elf_Data *locdata = NULL;
	      /* If this is no string section the length of each "string"
		 is always one.  */
	      unsigned int len = (head->flags & SHF_STRINGS) ? 0 : 1;

	      /* This is the generic string table functionality.  Much
		 slower than the specialized code.  */
	      mergestrtab
		= ebl_gstrtabinit (SCNINFO_SHDR (head->last->shdr).sh_entsize,
				   false);

	      symrunp = runp->symbols;
	      file = NULL;
	      do
		{
		  /* Accelarate the loop.  We cache the file
		     information since it might very well be the case
		     that the previous entry was from the same
		     file.  */
		  if (symrunp->file != file)
		    {
		      /* Remember the file.  */
		      file = symrunp->file;
		      /* Symbol table data from that file.  */
		      locsymdata = file->symtabdata;
		      /* String section data.  */
		      locdata = elf_rawdata (file->scninfo[symrunp->scndx].scn,
					     NULL);
		      assert (locdata != NULL);

		      /* While we are at it, remember the output
			 section.  If we don't access the string data
			 section the section won't be in the output
			 file.  So it is sufficient to do the work
			 here.  */
		      file->scninfo[symrunp->scndx].outscnndx = head->scnidx;
		    }

		  /* Get the symbol information.  This provides us the
		     offset into the string data section.  */
		  XElf_Sym_vardef (sym);
		  xelf_getsym (locsymdata, symrunp->symidx, sym);
		  assert (sym != NULL);

		  /* Get the data from the file.  Using the raw
		     section data here is possible since we don't
		     interpret the string themselves except for
		     looking for the wide NUL character.  The NUL
		     character has fortunately the same representation
		     regardless of the byte order.  */
		  symrunp->merge.handle
		    = ebl_gstrtabadd (mergestrtab,
				      (char *) locdata->d_buf + sym->st_value,
				      len);
		}
	      while ((symrunp = symrunp->next_in_scn) != runp->symbols);

	      /* Create the final table.  */
	      ebl_gstrtabfinalize (mergestrtab, outdata);

	      /* Compute the final offsets in the section.  */
	      symrunp = runp->symbols;
	      do
		{
		  symrunp->merge.value
		    = ebl_gstrtaboffset (symrunp->merge.handle);
		  symrunp->merged = 1;
		}
	      while ((symrunp = symrunp->next_in_scn) != runp->symbols);

	      /* We don't need the string table anymore.  */
	      ebl_gstrtabfree (mergestrtab);
	    }
	}
      else
	{
	no_merge:
	  assert (head->scnidx == elf_ndxscn (scn));

	  /* It is important to start with the first list entry (and
	     not just any one) to add the sections in the correct
	     order.  */
	  runp = head->last->next;
	  offset = 0;
	  do
	    {
	      Elf_Data *outdata = elf_newdata (scn);
	      if (outdata == NULL)
		error (EXIT_FAILURE, 0,
		       gettext ("cannot create section for output file: %s"),
		       elf_errmsg (-1));

	      /* Exceptional case: if we synthesize a data block SCN
		 is NULL and the sectio header info must be for a
		 SHT_NOBITS block and the size and alignment are
		 filled in.  */
	      if (likely (runp->scn != NULL))
		{
		  data = elf_getdata (runp->scn, NULL);
		  assert (data != NULL);

		  /* We reuse the data buffer in the input file.  */
		  *outdata = *data;

		  /* Given that we read the input file from disk we know there
		     cannot be another data part.  */
		  assert (elf_getdata (runp->scn, data) == NULL);
		}
	      else
		{
		  /* Must be a NOBITS section.  */
		  assert  (SCNINFO_SHDR (runp->shdr).sh_type == SHT_NOBITS);

		  outdata->d_buf = NULL;	/* Not needed.  */
		  outdata->d_type = ELF_T_BYTE;
		  outdata->d_version = EV_CURRENT;
		  outdata->d_size = SCNINFO_SHDR (runp->shdr).sh_size;
		  outdata->d_align = SCNINFO_SHDR (runp->shdr).sh_addralign;
		}

	      XElf_Off align =  MAX (1, outdata->d_align);
	      assert (powerof2 (align));
	      offset = ((offset + align - 1) & ~(align - 1));

	      runp->offset = offset;
	      runp->outscnndx = head->scnidx;
	      runp->allsectionsidx = cnt;

	      outdata->d_off = offset;

	      offset += outdata->d_size;
	    }
	  while ((runp = runp->next) != head->last->next);

	  /* If necessary add the additional line to the .comment section.  */
	  if (ld_state.add_ld_comment
	      && head->flags == 0
	      && head->type == SHT_PROGBITS
	      && strcmp (head->name, ".comment") == 0
	      && head->entsize == 0)
	    {
	      Elf_Data *outdata = elf_newdata (scn);

	      if (outdata == NULL)
		error (EXIT_FAILURE, 0,
		       gettext ("cannot create section for output file: %s"),
		       elf_errmsg (-1));

	      outdata->d_buf = (void *) "\0ld (" PACKAGE_NAME ") " PACKAGE_VERSION;
	      outdata->d_size = strlen ((char *) outdata->d_buf + 1) + 2;
	      outdata->d_off = offset;
	      outdata->d_type = ELF_T_BYTE;
	      outdata->d_align = 1;
	    }
	  /* XXX We should create a .comment section if none exists.
	     This requires that we early on detect that no such
	     section exists.  This should probably be implemented
	     together with some merging of the section contents.
	     Currently identical entries are not merged.  */
	}
    }

  /* The table we collect the strings in.  */
  strtab = ebl_strtabinit (true);
  if (strtab == NULL)
    error (EXIT_FAILURE, errno, gettext ("cannot create string table"));


#ifndef NDEBUG
  /* Keep track of the use of the XINDEX.  */
  need_xndx = false;
#endif

  /* We we generate a normal symbol table for an executable and the
     --export-dynamic option is not given, we need an extra table
     which keeps track of the symbol entry belonging to the symbol
     table entry.  Note that EXPORT_ALL_DYNAMIC is always set if we
     generate a DSO so we do not have to test this separately.  */
  ndxtosym = (struct symbol **) xcalloc (nsym_allocated,
					 sizeof (struct symbol));

  /* Create the special symbol for the GOT section.  */
  if (ld_state.got_symbol != NULL)
    {
      assert (nsym < nsym_allocated);
      // XXX Fix so that it works even if no PLT is needed.
      fillin_special_symbol (ld_state.got_symbol, ld_state.gotpltscnidx,
			     nsym++, symdata, strtab);
    }

  /* Similarly for the dynamic section symbol.  */
  if (ld_state.dyn_symbol != NULL)
    {
      assert (nsym < nsym_allocated);
      fillin_special_symbol (ld_state.dyn_symbol, ld_state.dynamicscnidx,
			     nsym++, symdata, strtab);
    }

  /* Create symbol table entries for the symbols defined in the linker
     script.  */
  if (ld_state.lscript_syms != NULL)
    {
      struct symbol *rsym = ld_state.lscript_syms;
      do
	{
	  assert (nsym < nsym_allocated);
	  fillin_special_symbol (rsym, SHN_ABS, nsym++, symdata, strtab);
	}
      while ((rsym = rsym->next) != NULL);
    }

  /* Iterate over all input files to collect the symbols.  */
  file = ld_state.relfiles->next;
  symdata = elf_getdata (elf_getscn (ld_state.outelf, ld_state.symscnidx),
			 NULL);

  do
    {
      size_t maxcnt;
      Elf_Data *insymdata;
      Elf_Data *inxndxdata;

      /* There must be no dynamic symbol table when creating
	 relocatable files.  */
      assert (ld_state.file_type != relocatable_file_type
	      || file->dynsymtabdata == NULL);

      insymdata = file->symtabdata;
      assert (insymdata != NULL);
      inxndxdata = file->xndxdata;

      maxcnt = file->nsymtab;

      file->symindirect = (Elf32_Word *) xcalloc (maxcnt, sizeof (Elf32_Word));

      /* The dynamic symbol table does not contain local symbols.  So
         we skip those entries.  */
      for (cnt = ld_state.need_symtab ? 1 : file->nlocalsymbols; cnt < maxcnt;
	   ++cnt)
	{
	  XElf_Sym_vardef (sym);
	  Elf32_Word xndx;
	  struct symbol *defp = NULL;

	  xelf_getsymshndx (insymdata, inxndxdata, cnt, sym, xndx);
	  assert (sym != NULL);

	  if (unlikely (XELF_ST_TYPE (sym->st_info) == STT_SECTION))
	    {
	      /* Section symbols should always be local but who knows...  */
	      if (ld_state.need_symtab)
		{
		  /* Determine the real section index in the source file.
		     Use the XINDEX section content if necessary.  We don't
		     add this information to the dynamic symbol table.  */
		  if (sym->st_shndx != SHN_XINDEX)
		    xndx = sym->st_shndx;

		  assert (file->scninfo[xndx].allsectionsidx
			  < ld_state.nallsections);
		  file->symindirect[cnt] = ld_state.allsections[file->scninfo[xndx].allsectionsidx]->scnsymidx;
		  /* Note that the resulting index can be zero here.  There is
		     no guarantee that the output file will contain all the
		     sections the input file did.  */
		}
	      continue;
	    }

	  if ((ld_state.strip >= strip_all || !ld_state.need_symtab)
	      /* XXX Do we need these entries?  */
	      && XELF_ST_TYPE (sym->st_info) == STT_FILE)
	    continue;

#if NATIVE_ELF != 0
	  /* Copy old data.  We create a temporary copy because the
	     symbol might still be discarded.  */
	  XElf_Sym sym_mem;
	  sym_mem = *sym;
	  sym = &sym_mem;
#endif

	  if (sym->st_shndx != SHN_UNDEF
	      && (sym->st_shndx < SHN_LORESERVE
		  || sym->st_shndx == SHN_XINDEX))
	    {
	      /* If we are creating an executable with no normal
		 symbol table and we do not export all symbols and
		 this symbol is not defined in a DSO as well, ignore
		 it.  */
	      if (!ld_state.export_all_dynamic && !ld_state.need_symtab)
		{
		  assert (cnt >= file->nlocalsymbols);
		  defp = file->symref[cnt];
		  assert (defp != NULL);

		  if (!defp->in_dso)
		    /* Ignore it.  */
		    continue;
		}

	      /* Determine the real section index in the source file.  Use
		 the XINDEX section content if necessary.  */
	      if (sym->st_shndx != SHN_XINDEX)
		xndx = sym->st_shndx;

	      sym->st_value += file->scninfo[xndx].offset;

	      assert (file->scninfo[xndx].outscnndx < SHN_LORESERVE
		      || file->scninfo[xndx].outscnndx > SHN_HIRESERVE);
	      if (unlikely (file->scninfo[xndx].outscnndx > SHN_LORESERVE))
		{
		  /* It is not possible to have an extended section index
		     table for the dynamic symbol table.  */
		  if (!ld_state.need_symtab)
		    error (EXIT_FAILURE, 0, gettext ("\
section index too large in dynamic symbol table"));

		  assert (xndxdata != NULL);
		  sym->st_shndx = SHN_XINDEX;
		  xndx = file->scninfo[xndx].outscnndx;
#ifndef NDEBUG
		  need_xndx = true;
#endif
		}
	      else
		{
		  sym->st_shndx = file->scninfo[xndx].outscnndx;
		  xndx = 0;
		}
	    }
	  else if (sym->st_shndx == SHN_COMMON || sym->st_shndx == SHN_UNDEF)
	    {
	      /* Check whether we have a (real) definition for this
		 symbol.  If this is the case we skip this symbol
		 table entry.  */
	      assert (cnt >= file->nlocalsymbols);
	      defp = file->symref[cnt];
	      assert (defp != NULL);

	      assert (sym->st_shndx != SHN_COMMON || defp->defined);

	      if ((sym->st_shndx == SHN_COMMON && !defp->common)
		  || (sym->st_shndx == SHN_UNDEF && defp->defined)
		  || defp->added)
		/* Ignore this symbol table entry, there is a
		   "better" one or we already added it.  */
		continue;

	      /* Remember that we already added this symbol.  */
	      defp->added = 1;

	      /* Adjust the section number for common symbols.  */
	      if (sym->st_shndx == SHN_COMMON)
		{
		  sym->st_value = (ld_state.common_section->offset
				   + file->symref[cnt]->merge.value);
		  assert (ld_state.common_section->outscnndx < SHN_LORESERVE);
		  sym->st_shndx = ld_state.common_section->outscnndx;
		  xndx = 0;
		}
	    }
	  else if (unlikely (sym->st_shndx != SHN_ABS))
	    {
	      if (SPECIAL_SECTION_NUMBER_P (&ld_state, sym->st_shndx))
		/* XXX Add code to handle machine specific special
		   sections.  */
		abort ();
	    }

	  /* Add the symbol name to the string table.  If the user
	     chooses the highest level of stripping avoid adding names
	     for local symbols in the string table.  */
	  if (sym->st_name != 0
	      && (ld_state.strip < strip_everything
		  || XELF_ST_BIND (sym->st_info) != STB_LOCAL))
	    symstrent[nsym] = ebl_strtabadd (strtab,
					     elf_strptr (file->elf,
							 file->symstridx,
							 sym->st_name), 0);

	  /* Once we know the name this field will get the correct
	     offset.  For now set it to zero which means no name
	     associated.  */
	  GElf_Word st_name = sym->st_name;
	  sym->st_name = 0;

	  /* If we had to merge sections we have a completely new
	     offset for the symbol.  */
	  if (file->has_merge_sections && file->symref[cnt] != NULL
	      && file->symref[cnt]->merged)
	    sym->st_value = file->symref[cnt]->merge.value;

	  /* Create the record in the output sections.  */
	  assert (nsym < nsym_allocated);
	  xelf_update_symshndx (symdata, xndxdata, nsym, sym, xndx, 1);

	  /* Add the reference to the symbol record in case we need it.
	     Find the symbol if this has not happened yet.  We do
	     not need the information for local symbols.  */
	  if (defp == NULL && cnt >= file->nlocalsymbols)
	    {
	      defp = file->symref[cnt];

	      if (defp == NULL)
		{
		  /* This is a symbol in a discarded COMDAT section.
		     Find the definition we actually use.  */
		  // XXX The question is: do we have to do this here
		  // XXX or can we do it earlier when we discard the
		  // XXX section.
		  struct symbol search;
		  search.name = elf_strptr (file->elf, file->symstridx,
					    st_name);
		  struct symbol *realp
		    = ld_symbol_tab_find (&ld_state.symbol_tab,
					  elf_hash (search.name), &search);
		  if (realp == NULL)
		    // XXX What to do here?
		    error (EXIT_FAILURE, 0,
			   "couldn't find symbol from COMDAT section");

		  file->symref[cnt] = realp;

		  continue;
		}
	    }

	  /* Store the reference to the symbol record.  The sorting
	     code will have to keep this array in the correct order, too.  */
	  ndxtosym[nsym] = defp;

	  /* One more entry finished.  */
	  if (cnt >= file->nlocalsymbols)
	    {
	      assert (file->symref[cnt]->outsymidx == 0);
	      file->symref[cnt]->outsymidx = nsym;
	    }
	  file->symindirect[cnt] = nsym++;
	}
    }
  while ((file = file->next) != ld_state.relfiles->next);
  /* Make sure we didn't create the extended section index table for
     nothing.  */
  assert (xndxdata == NULL || need_xndx);

  /* Create the version related sections.  */
  if (ld_state.verneedscnidx != 0)
    {
      /* We know the number of input files and total number of
	 referenced versions.  This allows us to allocate the memory
	 and then we iterate over the DSOs to get the version
	 information.  */
      struct usedfiles *runp;

      runp = ld_state.dsofiles->next;
      do
	allocate_version_names (runp, dynstrtab);
      while ((runp = runp->next) != ld_state.dsofiles->next);

      if (ld_state.needed != NULL)
	{
	  runp = ld_state.needed->next;
	  do
	    allocate_version_names (runp, dynstrtab);
	  while ((runp = runp->next) != ld_state.needed->next);
	}
    }

  /* At this point we should hide symbols and so on.  */
  if (ld_state.default_bind_local || ld_state.version_str_tab.filled > 0)
    /* XXX Add one more test when handling of wildcard symbol names
       is supported.  */
    {
    /* Check all non-local symbols whether they are on the export list.  */
      bool any_reduced = false;

      for (cnt = 1; cnt < nsym; ++cnt)
	{
	  XElf_Sym_vardef (sym);

	  /* Note that we don't have to use 'xelf_getsymshndx' since we
	     only need the binding and the symbol name.  */
	  xelf_getsym (symdata, cnt, sym);
	  assert (sym != NULL);

	  if (reduce_symbol_p (sym, symstrent[cnt]))
	    {
	      // XXX Check whether this is correct...
	      assert (ndxtosym[cnt]->outdynsymidx != 0);
	      ndxtosym[cnt]->outdynsymidx = 0;

	      sym->st_info = XELF_ST_INFO (STB_LOCAL,
					   XELF_ST_TYPE (sym->st_info));
	      (void) xelf_update_sym (symdata, cnt, sym);

	      /* Show that we don't need this string anymore.  */
	      if (ld_state.strip == strip_everything)
		{
		  symstrent[cnt] = NULL;
		  any_reduced = true;
		}
	    }
	}

      if (unlikely (any_reduced))
	{
	  /* Since we will not write names of local symbols in the
	     output file and we have reduced the binding of some
	     symbols the string table previously constructed contains
	     too many string.  Correct it.  */
	  struct Ebl_Strtab *newp = ebl_strtabinit (true);

	  for (cnt = 1; cnt < nsym; ++cnt)
	    if (symstrent[cnt] != NULL)
	      symstrent[cnt] = ebl_strtabadd (newp,
					      ebl_string (symstrent[cnt]), 0);

	  ebl_strtabfree (strtab);
	  strtab = newp;
	}
    }

  /* Add the references to DSOs.  We can add these entries this late
     (after sorting out versioning) because references to DSOs are not
     effected.  */
  if (ld_state.from_dso != NULL)
    {
      struct symbol *runp;
      size_t plt_base = nsym + ld_state.nfrom_dso - ld_state.nplt;
      size_t plt_idx = 0;
      size_t obj_idx = 0;

      assert (ld_state.nfrom_dso >= ld_state.nplt);
      runp = ld_state.from_dso;
      do
	{
	  // XXX What about functions which are only referenced via
	  // pointers and not PLT entries?  Can we distinguish such uses?
	  size_t idx;
	  if (runp->type == STT_FUNC)
	    {
	      /* Store the PLT entry number.  */
	      runp->merge.value = plt_idx + 1;
	      idx = plt_base + plt_idx++;
	    }
	  else
	    idx = nsym + obj_idx++;

	  XElf_Sym_vardef (sym);
	  xelf_getsym_ptr (symdata, idx, sym);

	  sym->st_value = 0;
	  sym->st_size = runp->size;
	  sym->st_info = XELF_ST_INFO (runp->weak ? STB_WEAK : STB_GLOBAL,
				       runp->type);
	  sym->st_other = STV_DEFAULT;
	  sym->st_shndx = SHN_UNDEF;

	  /* Create the record in the output sections.  */
	  xelf_update_symshndx (symdata, xndxdata, idx, sym, 0, 0);

	  const char *name = runp->name;
	  size_t namelen = 0;

	  if (runp->file->verdefdata != NULL)
	    {
	      // XXX Is it useful to add the versym value to struct symbol?
	      XElf_Versym versym;

	      (void) xelf_getversym_copy (runp->file->versymdata, runp->symidx,
					  versym);

	      /* One can only link with the default version.  */
	      assert ((versym & 0x8000) == 0);

	      const char *versname
		= ebl_string (runp->file->verdefent[versym]);

	      size_t versname_len = strlen (versname) + 1;
	      namelen = strlen (name) + versname_len + 2;
	      char *newp = (char *) obstack_alloc (&ld_state.smem, namelen);
	      memcpy (stpcpy (stpcpy (newp, name), "@@"),
		      versname, versname_len);
	      name = newp;
	    }

	  symstrent[idx] = ebl_strtabadd (strtab, name, namelen);

	  /* Record the initial index in the symbol table.  */
	  runp->outsymidx = idx;

	  /* Remember the symbol record this ELF symbol came from.  */
	  ndxtosym[idx] = runp;
	}
      while ((runp = runp->next) != ld_state.from_dso);

      assert (nsym + obj_idx == plt_base);
      assert (plt_idx == ld_state.nplt);
      nsym = plt_base + plt_idx;
    }

  /* Now we know how many symbols will be in the output file.  Adjust
     the count in the section data.  */
  symdata->d_size = xelf_fsize (ld_state.outelf, ELF_T_SYM, nsym);
  if (unlikely (xndxdata != NULL))
    xndxdata->d_size = xelf_fsize (ld_state.outelf, ELF_T_WORD, nsym);

  /* Create the symbol string table section.  */
  strscn = elf_newscn (ld_state.outelf);
  ld_state.strscnidx = elf_ndxscn (strscn);
  data = elf_newdata (strscn);
  xelf_getshdr (strscn, shdr);
  if (data == NULL || shdr == NULL)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot create section for output file: %s"),
	   elf_errmsg (-1));

  /* Create a compact string table, allocate the memory for it, and
     fill in the section data information.  */
  ebl_strtabfinalize (strtab, data);

  shdr->sh_type = SHT_STRTAB;
  assert (shdr->sh_entsize == 0);

  if (unlikely (xelf_update_shdr (strscn, shdr) == 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot create section for output file: %s"),
	   elf_errmsg (-1));

  /* Fill in the offsets of the symbol names.  */
  for (cnt = 1; cnt < nsym; ++cnt)
    if (symstrent[cnt] != NULL)
      {
	XElf_Sym_vardef (sym);

	/* Note that we don't have to use 'xelf_getsymshndx' since we don't
	   modify the section index.  */
	xelf_getsym (symdata, cnt, sym);
	/* This better worked, we did it before.  */
	assert (sym != NULL);
	sym->st_name = ebl_strtaboffset (symstrent[cnt]);
	(void) xelf_update_sym (symdata, cnt, sym);
      }

  /* Since we are going to reorder the symbol table but still have to
     be able to find the new position based on the old one (since the
     latter is stored in 'symindirect' information of the input file
     data structure) we have to create yet another indirection
     table.  */
  ld_state.dblindirect = dblindirect
    = (Elf32_Word *) xmalloc (nsym * sizeof (Elf32_Word));

  /* Sort the symbol table so that the local symbols come first.  */
  /* XXX We don't use stable sorting here.  It seems not necessary and
     would be more expensive.  If it turns out to be necessary this can
     be fixed easily.  */
  nsym_local = 1;
  cnt = nsym - 1;
  while (nsym_local < cnt)
    {
      XElf_Sym_vardef (locsym);
      Elf32_Word locxndx;
      XElf_Sym_vardef (globsym);
      Elf32_Word globxndx;

      do
	{
	  xelf_getsymshndx (symdata, xndxdata, nsym_local, locsym, locxndx);
	  /* This better works.  */
	  assert (locsym != NULL);

	  if (XELF_ST_BIND (locsym->st_info) != STB_LOCAL
	      && (ld_state.need_symtab || ld_state.export_all_dynamic))
	    {
	      do
		{
		  xelf_getsymshndx (symdata, xndxdata, cnt, globsym, globxndx);
		  /* This better works.  */
		  assert (globsym != NULL);

		  if (unlikely (XELF_ST_BIND (globsym->st_info) == STB_LOCAL))
		    {
		      /* We swap the two entries.  */
#if NATIVE_ELF != 0
		      /* Since we directly modify the data in the ELF
			 data structure we have to make a copy of one
			 of the entries.  */
		      XElf_Sym locsym_copy = *locsym;
		      locsym = &locsym_copy;
#endif
		      xelf_update_symshndx (symdata, xndxdata, nsym_local,
					    globsym, globxndx, 1);
		      xelf_update_symshndx (symdata, xndxdata, cnt,
					    locsym, locxndx, 1);

		      /* Also swap the cross references.  */
		      dblindirect[nsym_local] = cnt;
		      dblindirect[cnt] = nsym_local;

		      /* And the entries for the symbol names.  */
		      struct Ebl_Strent *strtmp = symstrent[nsym_local];
		      symstrent[nsym_local] = symstrent[cnt];
		      symstrent[cnt] = strtmp;

		      /* And the mapping from symbol table entry to
			 struct symbol record.  */
		      struct symbol *symtmp = ndxtosym[nsym_local];
		      ndxtosym[nsym_local] = ndxtosym[cnt];
		      ndxtosym[cnt] = symtmp;

		      /* Go to the next entry.  */
		      ++nsym_local;
		      --cnt;

		      break;
		    }

		  dblindirect[cnt] = cnt;
		}
	      while (nsym_local < --cnt);

	      break;
	    }

	  dblindirect[nsym_local] = nsym_local;
	}
      while (++nsym_local < cnt);
    }

  /* The symbol 'nsym_local' is currently pointing to might be local,
     too.  Check and increment the variable if this is the case.  */
  if (likely (nsym_local < nsym))
    {
      XElf_Sym_vardef (locsym);

      /* This entry isn't moved.  */
      dblindirect[nsym_local] = nsym_local;

      /* Note that it is OK to not use 'xelf_getsymshndx' here.  */
      xelf_getsym (symdata, nsym_local, locsym);
      /* This better works.  */
      assert (locsym != NULL);

      if (XELF_ST_BIND (locsym->st_info) == STB_LOCAL)
	++nsym_local;
    }


  /* We need the versym array right away to keep track of the version
     symbols.  */
  if (ld_state.versymscnidx != 0)
    {
      /* We allocate more memory than we need since the array is morroring
	 the dynamic symbol table and not the normal symbol table.  I.e.,
	 no local symbols are present.  */
      versymscn = elf_getscn (ld_state.outelf, ld_state.versymscnidx);
      versymdata = elf_newdata (versymscn);
      if (versymdata == NULL)
	error (EXIT_FAILURE, 0,
	       gettext ("cannot create versioning section: %s"),
	       elf_errmsg (-1));

      versymdata->d_size = xelf_fsize (ld_state.outelf, ELF_T_HALF,
				       nsym - nsym_local + 1);
      versymdata->d_buf = xcalloc (1, versymdata->d_size);
      versymdata->d_align = xelf_fsize (ld_state.outelf, ELF_T_HALF, 1);
      versymdata->d_off = 0;
      versymdata->d_type = ELF_T_HALF;
    }


  /* If we have to construct the dynamic symbol table we must not include
     the local symbols.  If the normal symbol has to be emitted as well
     we haven't done anything else yet and we can construct it from
     scratch now.  */
  if (unlikely (!ld_state.need_symtab))
    {
      /* Note that the following code works even if there is no entry
	 to remove since the zeroth entry is always local.  */
      size_t reduce = xelf_fsize (ld_state.outelf, ELF_T_SYM, nsym_local - 1);

      XElf_Sym_vardef (nullsym);
      xelf_getsym_ptr (symdata, nsym_local - 1, nullsym);

      /* Note that we don't have to use 'xelf_update_symshndx' since
	 this is the dynamic symbol table we write.  */
      (void) xelf_update_sym (symdata, nsym_local - 1,
			      memset (nullsym, '\0', sizeof (*nullsym)));

      /* Update the buffer pointer and size in the output data.  */
      symdata->d_buf = (char *) symdata->d_buf + reduce;
      symdata->d_size -= reduce;

      /* Add the version symbol information.  */
      if (versymdata != NULL)
	{
	  nsym_dyn = 1;
	  for (cnt = nsym_local; cnt < nsym; ++cnt, ++nsym_dyn)
	    {
	      struct symbol *symp = ndxtosym[cnt];

	      if (symp->file->versymdata != NULL)
		{
		  GElf_Versym versym;

		  gelf_getversym (symp->file->versymdata, symp->symidx,
				  &versym);

		  (void) gelf_update_versym (versymdata, symp->outdynsymidx,
					     &symp->file->verdefused[versym]);
		}
	      }
	}

      /* Since we only created the dynamic symbol table the number of
	 dynamic symbols is the total number of symbols.  */
      nsym_dyn = nsym - nsym_local + 1;

      /* XXX TBI.  Create whatever data structure is missing.  */
      abort ();
    }
  else if (ld_state.need_dynsym)
    {
      /* Create the dynamic symbol table section data along with the
	 string table.  We look at all non-local symbols we found for
	 the normal symbol table and add those.  */
      dynsymscn = elf_getscn (ld_state.outelf, ld_state.dynsymscnidx);
      dynsymdata = elf_newdata (dynsymscn);

      dynstrdata = elf_newdata (elf_getscn (ld_state.outelf,
					    ld_state.dynstrscnidx));
      if (dynsymdata == NULL || dynstrdata == NULL)
	error (EXIT_FAILURE, 0, gettext ("\
cannot create dynamic symbol table for output file: %s"),
	       elf_errmsg (-1));

      nsym_dyn_allocated = nsym - nsym_local + 1;
      dynsymdata->d_size = xelf_fsize (ld_state.outelf, ELF_T_SYM,
				       nsym_dyn_allocated);
      dynsymdata->d_buf = memset (xmalloc (dynsymdata->d_size), '\0',
				  xelf_fsize (ld_state.outelf, ELF_T_SYM, 1));
      dynsymdata->d_type = ELF_T_SYM;
      dynsymdata->d_off = 0;
      dynsymdata->d_align = xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1);

      /* We need one more array which contains the hash codes of the
	 symbol names.  */
      hashcodes = (Elf32_Word *) xcalloc (__builtin_popcount ((int) ld_state.hash_style)
					  * nsym_dyn_allocated,
					  sizeof (Elf32_Word));
      gnuhashcodes = hashcodes;
      if (GENERATE_SYSV_HASH)
	gnuhashcodes += nsym_dyn_allocated;

      /* We have and empty entry at the beginning.  */
      nsym_dyn = 1;

      /* Populate the table.  */
      for (cnt = nsym_local; cnt < nsym; ++cnt)
	{
	  XElf_Sym_vardef (sym);

	  xelf_getsym (symdata, cnt, sym);
	  assert (sym != NULL);

	  if (sym->st_shndx == SHN_XINDEX)
	    error (EXIT_FAILURE, 0, gettext ("\
section index too large in dynamic symbol table"));

	  /* We do not add the symbol to the dynamic symbol table if

	     - the symbol is for a file
	     - it is not externally visible (internal, hidden)
	     - export_all_dynamic is not set and the symbol is only defined
	       in the executable (i.e., it is defined, but not (also) in DSO)

	     Set symstrent[cnt] to NULL in case an entry is ignored.  */
	  if (XELF_ST_TYPE (sym->st_info) == STT_FILE
	      || XELF_ST_VISIBILITY (sym->st_other) == STV_INTERNAL
	      || XELF_ST_VISIBILITY (sym->st_other) == STV_HIDDEN
	      || (!ld_state.export_all_dynamic
		  && !ndxtosym[cnt]->in_dso && ndxtosym[cnt]->defined))
	    {
	      symstrent[cnt] = NULL;
	      continue;
	    }

	  /* Store the index of the symbol in the dynamic symbol
	     table.  This is a preliminary value in case we use the
	     GNU-style hash table.  */
	  ndxtosym[cnt]->outdynsymidx = nsym_dyn;

	  /* Create a new string table entry.  */
	  const char *str = ndxtosym[cnt]->name;
	  symstrent[cnt] = ebl_strtabadd (dynstrtab, str, 0);
	  if (GENERATE_SYSV_HASH)
	    hashcodes[nsym_dyn] = elf_hash (str);
	  if (GENERATE_GNU_HASH)
	    gnuhashcodes[nsym_dyn] = elf_gnu_hash (str);
	  ++nsym_dyn;
	}

      if (ld_state.file_type != relocatable_file_type)
	{
	  /* Finalize the dynamic string table.  */
	  ebl_strtabfinalize (dynstrtab, dynstrdata);

	  assert (ld_state.hashscnidx != 0 || ld_state.gnuhashscnidx != 0);

	  /* Create the GNU-style hash table.  */
	  if (GENERATE_GNU_HASH)
	    create_gnu_hash (nsym_local, nsym, nsym_dyn, gnuhashcodes);

	  /* Create the SysV-style hash table.  This has to happen
	     after the GNU-style table is created since
	     CREATE-GNU-HASH might reorder the dynamic symbol table.  */
	  if (GENERATE_SYSV_HASH)
	    create_hash (nsym_local, nsym, nsym_dyn, hashcodes);
	}

	  /* Add the version information.  */
      if (versymdata != NULL)
	for (cnt = nsym_local; cnt < nsym; ++cnt)
	  if (symstrent[cnt] != NULL)
	    {
	      struct symbol *symp = ndxtosym[cnt];

	      /* Synthetic symbols (i.e., those with no file attached)
		 have no version information.  */
	      if (symp->file != NULL && symp->file->verdefdata != NULL)
		{
		  GElf_Versym versym;

		  gelf_getversym (symp->file->versymdata, symp->symidx,
				  &versym);

		  (void) gelf_update_versym (versymdata, symp->outdynsymidx,
					     &symp->file->verdefused[versym]);
		}
	      else
		{
		  /* XXX Add support for version definitions.  */
		  GElf_Versym global = VER_NDX_GLOBAL;
		  (void) gelf_update_versym (versymdata, nsym_dyn, &global);
		}
	    }

      /* Update the information about the symbol section.  */
      if (versymdata != NULL)
	{
	  /* Correct the size now that we know how many entries the
	     dynamic symbol table has.  */
	  versymdata->d_size = xelf_fsize (ld_state.outelf, ELF_T_HALF,
					   nsym_dyn);

	  /* Add the reference to the symbol table.  */
	  xelf_getshdr (versymscn, shdr);
	  assert (shdr != NULL);

	  shdr->sh_link = ld_state.dynsymscnidx;

	  (void) xelf_update_shdr (versymscn, shdr);
	}
    }

  if (ld_state.file_type != relocatable_file_type)
    {
      /* Now put the names in.  */
      for (cnt = nsym_local; cnt < nsym; ++cnt)
	if (symstrent[cnt] != NULL)
	  {
	    XElf_Sym_vardef (sym);
	    size_t dynidx = ndxtosym[cnt]->outdynsymidx;

#if NATIVE_ELF != 0
	    XElf_Sym *osym;
	    memcpy (xelf_getsym (dynsymdata, dynidx, sym),
		    xelf_getsym (symdata, cnt, osym),
		    sizeof (XElf_Sym));
#else
	    xelf_getsym (symdata, cnt, sym);
	    assert (sym != NULL);
#endif

	    sym->st_name = ebl_strtaboffset (symstrent[cnt]);

	    (void) xelf_update_sym (dynsymdata, dynidx, sym);
	  }

      free (hashcodes);

      /* Create the required version section.  */
      if (ld_state.verneedscnidx != 0)
	{
	  Elf_Scn *verneedscn;
	  Elf_Data *verneeddata;
	  struct usedfiles *runp;
	  size_t verneed_size = xelf_fsize (ld_state.outelf, ELF_T_VNEED, 1);
	  size_t vernaux_size = xelf_fsize (ld_state.outelf, ELF_T_VNAUX, 1);
	  size_t offset;
	  int ntotal;

	  verneedscn = elf_getscn (ld_state.outelf, ld_state.verneedscnidx);
	  xelf_getshdr (verneedscn, shdr);
	  verneeddata = elf_newdata (verneedscn);
	  if (shdr == NULL || verneeddata == NULL)
	    error (EXIT_FAILURE, 0,
		   gettext ("cannot create versioning data: %s"),
		   elf_errmsg (-1));

	  verneeddata->d_size = (ld_state.nverdeffile * verneed_size
				 + ld_state.nverdefused * vernaux_size);
	  verneeddata->d_buf = xmalloc (verneeddata->d_size);
	  verneeddata->d_type = ELF_T_VNEED;
	  verneeddata->d_align = xelf_fsize (ld_state.outelf, ELF_T_WORD, 1);
	  verneeddata->d_off = 0;

	  offset = 0;
	  ntotal = ld_state.nverdeffile;
	  runp = ld_state.dsofiles->next;
	  do
	    {
	      offset = create_verneed_data (offset, verneeddata, runp,
					    &ntotal);
	      runp = runp->next;
	    }
	  while (ntotal > 0 && runp != ld_state.dsofiles->next);

	  if (ntotal > 0)
	    {
	      runp = ld_state.needed->next;
	      do
		{
		  offset = create_verneed_data (offset, verneeddata, runp,
						&ntotal);
		  runp = runp->next;
		}
	      while (ntotal > 0 && runp != ld_state.needed->next);
	    }

	  assert (offset == verneeddata->d_size);

	  /* Add the needed information to the section header.  */
	  shdr->sh_link = ld_state.dynstrscnidx;
	  shdr->sh_info = ld_state.nverdeffile;
	  (void) xelf_update_shdr (verneedscn, shdr);
	}

      /* Adjust the section size.  */
      dynsymdata->d_size = xelf_fsize (ld_state.outelf, ELF_T_SYM, nsym_dyn);
      if (versymdata != NULL)
	versymdata->d_size = xelf_fsize (ld_state.outelf, ELF_T_HALF,
					 nsym_dyn);

      /* Add the remaining information to the section header.  */
      xelf_getshdr (dynsymscn, shdr);
      /* There is always exactly one local symbol.  */
      shdr->sh_info = 1;
      /* Reference the string table.  */
      shdr->sh_link = ld_state.dynstrscnidx;
      /* Write the updated info back.  */
      (void) xelf_update_shdr (dynsymscn, shdr);
    }

  /* We don't need the string table anymore.  */
  free (symstrent);

  /* Remember the total number of symbols in the dynamic symbol table.  */
  ld_state.ndynsym = nsym_dyn;

  /* Fill in the section header information.  */
  symscn = elf_getscn (ld_state.outelf, ld_state.symscnidx);
  xelf_getshdr (symscn, shdr);
  if (shdr == NULL)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot create symbol table for output file: %s"),
	   elf_errmsg (-1));

  shdr->sh_type = SHT_SYMTAB;
  shdr->sh_link = ld_state.strscnidx;
  shdr->sh_info = nsym_local;
  shdr->sh_entsize = xelf_fsize (ld_state.outelf, ELF_T_SYM, 1);

  (void) xelf_update_shdr (symscn, shdr);


  /* Add names for the generated sections.  */
  if (ld_state.symscnidx != 0)
      symtab_ent = ebl_strtabadd (ld_state.shstrtab, ".symtab", 8);
  if (ld_state.xndxscnidx != 0)
    xndx_ent = ebl_strtabadd (ld_state.shstrtab, ".symtab_shndx", 14);
  if (ld_state.strscnidx != 0)
    strtab_ent = ebl_strtabadd (ld_state.shstrtab, ".strtab", 8);
  /* At this point we would have to test for failures in the
     allocation.  But we skip this.  First, the problem will be caught
     later when doing more allocations for the section header table.
     Even if this would not be the case all that would happen is that
     the section names are empty.  The binary would still be usable if
     it is an executable or a DSO.  Not adding the test here saves
     quite a bit of code.  */


  /* Finally create the section for the section header string table.  */
  shstrtab_scn = elf_newscn (ld_state.outelf);
  shstrtab_ndx = elf_ndxscn (shstrtab_scn);
  if (unlikely (shstrtab_ndx == SHN_UNDEF))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot create section header string section: %s"),
	   elf_errmsg (-1));

  /* Add the name of the section to the string table.  */
  shstrtab_ent = ebl_strtabadd (ld_state.shstrtab, ".shstrtab", 10);
  if (unlikely (shstrtab_ent == NULL))
    error (EXIT_FAILURE, errno,
	   gettext ("cannot create section header string section"));

  /* Finalize the section header string table.  */
  data = elf_newdata (shstrtab_scn);
  if (data == NULL)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot create section header string section: %s"),
	   elf_errmsg (-1));
  ebl_strtabfinalize (ld_state.shstrtab, data);

  /* Now we know the string offsets for all section names.  */
  for (cnt = 0; cnt < ld_state.nallsections; ++cnt)
    if (ld_state.allsections[cnt]->scnidx != 0)
      {
	Elf_Scn *scn;

	scn = elf_getscn (ld_state.outelf, ld_state.allsections[cnt]->scnidx);

	xelf_getshdr (scn, shdr);
	assert (shdr != NULL);

	shdr->sh_name = ebl_strtaboffset (ld_state.allsections[cnt]->nameent);

	if (xelf_update_shdr (scn, shdr) == 0)
	  assert (0);
      }

  /* Add the names for the generated sections to the respective
     section headers.  */
  if (symtab_ent != NULL)
    {
      Elf_Scn *scn = elf_getscn (ld_state.outelf, ld_state.symscnidx);

      xelf_getshdr (scn, shdr);
      /* This cannot fail, we already accessed the header before.  */
      assert (shdr != NULL);

      shdr->sh_name = ebl_strtaboffset (symtab_ent);

      (void) xelf_update_shdr (scn, shdr);
    }
  if (xndx_ent != NULL)
    {
      Elf_Scn *scn = elf_getscn (ld_state.outelf, ld_state.xndxscnidx);

      xelf_getshdr (scn, shdr);
      /* This cannot fail, we already accessed the header before.  */
      assert (shdr != NULL);

      shdr->sh_name = ebl_strtaboffset (xndx_ent);

      (void) xelf_update_shdr (scn, shdr);
    }
  if (strtab_ent != NULL)
    {
      Elf_Scn *scn = elf_getscn (ld_state.outelf, ld_state.strscnidx);

      xelf_getshdr (scn, shdr);
      /* This cannot fail, we already accessed the header before.  */
      assert (shdr != NULL);

      shdr->sh_name = ebl_strtaboffset (strtab_ent);

      (void) xelf_update_shdr (scn, shdr);
    }

  /* And the section header table section itself.  */
  xelf_getshdr (shstrtab_scn, shdr);
  if (shdr == NULL)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot create section header string section: %s"),
	   elf_errmsg (-1));

  shdr->sh_name = ebl_strtaboffset (shstrtab_ent);
  shdr->sh_type = SHT_STRTAB;

  if (unlikely (xelf_update_shdr (shstrtab_scn, shdr) == 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot create section header string section: %s"),
	   elf_errmsg (-1));


  /* Add the correct section header info to the section group sections.  */
  groups = ld_state.groups;
  while (groups != NULL)
    {
      Elf_Scn *scn = elf_getscn (ld_state.outelf, groups->outscnidx);
      xelf_getshdr (scn, shdr);
      assert (shdr != NULL);

      shdr->sh_name = ebl_strtaboffset (groups->nameent);
      shdr->sh_type = SHT_GROUP;
      shdr->sh_flags = 0;
      shdr->sh_link = ld_state.symscnidx;
      shdr->sh_entsize = sizeof (Elf32_Word);

      /* Determine the index for the signature symbol.  */
      Elf32_Word si
	= groups->symbol->file->symindirect[groups->symbol->symidx];
      if (si == 0)
	{
	  assert (groups->symbol->file->symref[groups->symbol->symidx]
		  != NULL);
	  si = groups->symbol->file->symref[groups->symbol->symidx]->outsymidx;
	  assert (si != 0);
	}
      shdr->sh_info = ld_state.dblindirect[si];

      (void) xelf_update_shdr (scn, shdr);

      struct scngroup *oldp = groups;
      groups = groups->next;
      free (oldp);
    }


  if (ld_state.file_type != relocatable_file_type)
    {
      /* Every executable needs a program header.  The number of entries
	 varies.  One exists for each segment.  Each SHT_NOTE section gets
	 one, too.  For dynamically linked executables we have to create
	 one for the program header, the interpreter, and the dynamic
	 section.  First count the number of segments.

	 XXX Determine whether the segment is non-empty.  */
      size_t nphdr = 0;

      /* We always add a PT_GNU_stack entry.  */
      ++nphdr;

      struct output_segment *segment = ld_state.output_segments;
      while (segment != NULL)
	{
	  ++nphdr;
	  segment = segment->next;
	}

      /* Add the number of SHT_NOTE sections.  We counted them earlier.  */
      nphdr += ld_state.nnotesections;

      /* If we create a DSO or the file is linked against DSOs we have
	 at least one more entry: DYNAMIC.  If an interpreter is
	 specified we add PHDR and INTERP, too.  */
      if (dynamically_linked_p ())
	{
	  ++nphdr;

	  if (ld_state.interp != NULL || ld_state.file_type != dso_file_type)
	    nphdr += 2;
	}

      /* If we need a TLS segment we need an entry for that.  */
      if (ld_state.need_tls)
	++nphdr;

      /* Create the program header structure.  */
      XElf_Phdr_vardef (phdr);
      if (xelf_newphdr (ld_state.outelf, nphdr) == 0)
	error (EXIT_FAILURE, 0, gettext ("cannot create program header: %s"),
	       elf_errmsg (-1));


      /* Determine the section sizes and offsets.  We have to do this
	 to be able to determine the memory layout (which normally
	 differs from the file layout).  */
      if (elf_update (ld_state.outelf, ELF_C_NULL) == -1)
	error (EXIT_FAILURE, 0, gettext ("while determining file layout: %s"),
	       elf_errmsg (-1));


      /* Now determine the memory addresses of all the sections and
	 segments.  */
      Elf32_Word nsec = 0;
      Elf_Scn *scn = elf_getscn (ld_state.outelf,
				 ld_state.allsections[nsec]->scnidx);
      xelf_getshdr (scn, shdr);
      assert (shdr != NULL);

      /* The address we start with is the offset of the first (not
	 zeroth) section.  */
      XElf_Addr addr = shdr->sh_offset;
      XElf_Addr tls_offset = 0;
      XElf_Addr tls_start = ~((XElf_Addr) 0);
      XElf_Addr tls_end = 0;
      XElf_Off tls_filesize = 0;
      XElf_Addr tls_align = 0;

      /* The index of the first loadable segment.  */
      nphdr = 0;
      if (dynamically_linked_p ())
	{
	  ++nphdr;
	  if (ld_state.interp != NULL
	      || ld_state.file_type != dso_file_type)
	    nphdr += 2;
	}

      segment = ld_state.output_segments;
      while (segment != NULL)
	{
	  struct output_rule *orule;
	  bool first_section = true;
	  XElf_Off nobits_size = 0;
	  XElf_Off memsize = 0;

	  /* The minimum alignment is a page size.  */
	  segment->align = ld_state.pagesize;

	  for (orule = segment->output_rules; orule != NULL;
	       orule = orule->next)
	    if (orule->tag == output_section)
	      {
		/* See whether this output rule corresponds to the next
		   section.  Yes, this is a pointer comparison.  */
		if (ld_state.allsections[nsec]->name
		    != orule->val.section.name)
		  /* No, ignore this output rule.  */
		  continue;

		/* We assign addresses only in segments which are actually
		   loaded.  */
		if (segment->mode != 0)
		  {
		    /* Adjust the offset of the input sections.  */
		    struct scninfo *isect;
		    struct scninfo *first;

		    isect = first = ld_state.allsections[nsec]->last;
		    if (isect != NULL)
		      do
			isect->offset += addr;
		      while ((isect = isect->next) != first);

		    /* Set the address of current section.  */
		    shdr->sh_addr = addr;

		    /* Write the result back.  */
		    (void) xelf_update_shdr (scn, shdr);

		    /* Remember the address.  */
		    ld_state.allsections[nsec]->addr = addr;

		    /* Handle TLS sections.  */
		    if (unlikely (shdr->sh_flags & SHF_TLS))
		      {
			if (tls_start > addr)
			  {
			    tls_start = addr;
			    tls_offset = shdr->sh_offset;
			  }
			if (tls_end < addr + shdr->sh_size)
			  tls_end = addr + shdr->sh_size;
			if (shdr->sh_type != SHT_NOBITS)
			  tls_filesize += shdr->sh_size;
			if (shdr->sh_addralign > tls_align)
			  tls_align = shdr->sh_addralign;
		      }
		  }

		if (first_section)
		  {
		    /* The first segment starts at offset zero.  */
		    if (segment == ld_state.output_segments)
		      {
			segment->offset = 0;
			segment->addr = addr - shdr->sh_offset;
		      }
		    else
		      {
			segment->offset = shdr->sh_offset;
			segment->addr = addr;
		      }

		    /* Determine the maximum alignment requirement.  */
		    segment->align = MAX (segment->align, shdr->sh_addralign);

		    first_section = false;
		  }

		/* NOBITS TLS sections are not laid out in address space
		   along with the other sections.  */
		if (shdr->sh_type != SHT_NOBITS
		    || (shdr->sh_flags & SHF_TLS) == 0)
		  {
		    memsize = (shdr->sh_offset - segment->offset
			       + shdr->sh_size);
		    if (nobits_size != 0 && shdr->sh_type != SHT_NOTE)
		      error (EXIT_FAILURE, 0, gettext ("\
internal error: non-nobits section follows nobits section"));
		    if (shdr->sh_type == SHT_NOBITS)
		      nobits_size += shdr->sh_size;
		  }

		/* Determine the new address which is computed using
		   the difference of the offsets on the sections.  Note
		   that this assumes that the sections following each
		   other in the section header table are also
		   consecutive in the file.  This is true here because
		   libelf constructs files this way.  */
		XElf_Off oldoff = shdr->sh_offset;

		if (++nsec >= ld_state.nallsections)
		  break;

		scn = elf_getscn (ld_state.outelf,
				  ld_state.allsections[nsec]->scnidx);
		xelf_getshdr (scn, shdr);
		assert (shdr != NULL);

		/* This is the new address resulting from the offsets
		   in the file.  */
		assert (oldoff <= shdr->sh_offset);
		addr += shdr->sh_offset - oldoff;
	      }
	    else
	      {
		assert (orule->tag == output_assignment);

		if (strcmp (orule->val.assignment->variable, ".") == 0)
		  /* This is a change of the address.  */
		  addr = eval_expression (orule->val.assignment->expression,
					  addr);
		else if (orule->val.assignment->sym != NULL)
		  {
		    /* This symbol is used.  Update the symbol table
		       entry.  */
		    XElf_Sym_vardef (sym);
		    size_t idx;

		    /* Note that we do not have to use
		       xelf_getsymshndx since we only update the
		       symbol address, not the section
		       information.  */
		    idx = dblindirect[orule->val.assignment->sym->outsymidx];
		    xelf_getsym (symdata, idx, sym);
		    sym->st_value = addr;
		    (void) xelf_update_sym (symdata, idx, sym);

		    idx = orule->val.assignment->sym->outdynsymidx;
		    if (idx != 0)
		      {
			assert (dynsymdata != NULL);
			xelf_getsym (dynsymdata, idx, sym);
			sym->st_value = addr;
			(void) xelf_update_sym (dynsymdata, idx, sym);
		      }
		  }
	      }

	  /* Store the segment parameter for loadable segments.  */
	  if (segment->mode != 0)
	    {
	      xelf_getphdr_ptr (ld_state.outelf, nphdr, phdr);

	      phdr->p_type = PT_LOAD;
	      phdr->p_offset = segment->offset;
	      phdr->p_vaddr = segment->addr;
	      phdr->p_paddr = phdr->p_vaddr;
	      phdr->p_filesz = memsize - nobits_size;
	      phdr->p_memsz = memsize;
	      phdr->p_flags = segment->mode;
	      phdr->p_align = segment->align;

	      (void) xelf_update_phdr (ld_state.outelf, nphdr, phdr);
	      ++nphdr;
	    }

	  segment = segment->next;
	}

      /* Create the other program header entries.  */
      xelf_getehdr (ld_state.outelf, ehdr);
      assert (ehdr != NULL);

      /* Add the TLS information.  */
      if (ld_state.need_tls)
	{
	  xelf_getphdr_ptr (ld_state.outelf, nphdr, phdr);
	  phdr->p_type = PT_TLS;
	  phdr->p_offset = tls_offset;
	  phdr->p_vaddr = tls_start;
	  phdr->p_paddr = tls_start;
	  phdr->p_filesz = tls_filesize;
	  phdr->p_memsz = tls_end - tls_start;
	  phdr->p_flags = PF_R;
	  phdr->p_align = tls_align;
	  ld_state.tls_tcb = tls_end;
	  ld_state.tls_start = tls_start;

	  (void) xelf_update_phdr (ld_state.outelf, nphdr, phdr);
	  ++nphdr;
	}

      /* Add the stack information.  */
      xelf_getphdr_ptr (ld_state.outelf, nphdr, phdr);
      phdr->p_type = PT_GNU_STACK;
      phdr->p_offset = 0;
      phdr->p_vaddr = 0;
      phdr->p_paddr = 0;
      phdr->p_filesz = 0;
      phdr->p_memsz = 0;
      phdr->p_flags = (PF_R | PF_W
		       | (ld_state.execstack == execstack_true ? PF_X : 0));
      phdr->p_align = xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1);

      (void) xelf_update_phdr (ld_state.outelf, nphdr, phdr);
      ++nphdr;


      /* Adjust the addresses in the address fields of the symbol
	 records according to the load addresses of the sections.  */
      if (ld_state.need_symtab)
	for (cnt = 1; cnt < nsym; ++cnt)
	  {
	    XElf_Sym_vardef (sym);
	    Elf32_Word shndx;

	    xelf_getsymshndx (symdata, xndxdata, cnt, sym, shndx);
	    assert (sym != NULL);

	    if (sym->st_shndx != SHN_XINDEX)
	      shndx = sym->st_shndx;

	    if ((shndx > SHN_UNDEF && shndx < SHN_LORESERVE)
		|| shndx > SHN_HIRESERVE)
	      {
		/* Note we subtract 1 from the section index since ALLSECTIONS
		   does not store the dummy section with offset zero.  */
		sym->st_value += ld_state.allsections[shndx - 1]->addr;

		/* We don't have to use 'xelf_update_symshndx' since the
		   section number doesn't change.  */
		(void) xelf_update_sym (symdata, cnt, sym);
	      }
	  }

      if (ld_state.need_dynsym)
	for (cnt = 1; cnt < nsym_dyn; ++cnt)
	  {
	    XElf_Sym_vardef (sym);

	    xelf_getsym (dynsymdata, cnt, sym);
	    assert (sym != NULL);

	    if (sym->st_shndx > SHN_UNDEF && sym->st_shndx < SHN_LORESERVE)
	      {
		/* Note we subtract 1 from the section index since ALLSECTIONS
		   does not store the dummy section with offset zero.  */
		sym->st_value += ld_state.allsections[sym->st_shndx - 1]->addr;

		/* We don't have to use 'xelf_update_symshndx' since the
		   section number doesn't change.  */
		(void) xelf_update_sym (dynsymdata, cnt, sym);
	      }
	  }

      /* Now is a good time to determine the values of all the symbols
	 we encountered.  */
      // XXX This loop is very inefficient.  The hash tab iterator also
      // returns all symbols in DSOs.
      struct symbol *se;
      void *p = NULL;
      while ((se = ld_symbol_tab_iterate (&ld_state.symbol_tab, &p)) != NULL)
	if (! se->in_dso)
	  {
	    XElf_Sym_vardef (sym);

	    addr = 0;

	    if (se->outdynsymidx != 0)
	      {
		xelf_getsym (dynsymdata, se->outdynsymidx, sym);
		assert (sym != NULL);
		addr = sym->st_value;
	      }
	    else if (se->outsymidx != 0)
	      {
		assert (dblindirect[se->outsymidx] != 0);
		xelf_getsym (symdata, dblindirect[se->outsymidx], sym);
		assert (sym != NULL);
		addr = sym->st_value;
	      }
	    else
	      abort ();

	    se->merge.value = addr;
	  }

      /* Complete the header of the .rel.dyn/.rela.dyn section.  Point
	 to the symbol table.  The sh_info field is left zero since
	 there is no specific section the contained relocations are
	 for.  */
      if (ld_state.reldynscnidx != 0)
	{
	  assert (ld_state.dynsymscnidx != 0);
	  scn = elf_getscn (ld_state.outelf, ld_state.reldynscnidx);
	  xelf_getshdr (scn, shdr);
	  assert (shdr != NULL);

	  shdr->sh_link = ld_state.dynsymscnidx;

	  (void) xelf_update_shdr (scn, shdr);
	}

      /* Fill in the dynamic segment/section.  */
      if (dynamically_linked_p ())
	{
	  Elf_Scn *outscn;

	  int idx = 0;
	  if (ld_state.interp != NULL || ld_state.file_type != dso_file_type)
	    {
	      assert (ld_state.interpscnidx != 0);
	      xelf_getshdr (elf_getscn (ld_state.outelf,
					ld_state.interpscnidx), shdr);
	      assert (shdr != NULL);

	      xelf_getphdr_ptr (ld_state.outelf, idx, phdr);
	      phdr->p_type = PT_PHDR;
	      phdr->p_offset = ehdr->e_phoff;
	      phdr->p_vaddr = ld_state.output_segments->addr + phdr->p_offset;
	      phdr->p_paddr = phdr->p_vaddr;
	      phdr->p_filesz = ehdr->e_phnum * ehdr->e_phentsize;
	      phdr->p_memsz = phdr->p_filesz;
	      phdr->p_flags = 0;	/* No need to set PF_R or so.  */
	      phdr->p_align = xelf_fsize (ld_state.outelf, ELF_T_ADDR, 1);

	      (void) xelf_update_phdr (ld_state.outelf, idx, phdr);
	      ++idx;

	      /* The interpreter string.  */
	      xelf_getphdr_ptr (ld_state.outelf, idx, phdr);
	      phdr->p_type = PT_INTERP;
	      phdr->p_offset = shdr->sh_offset;
	      phdr->p_vaddr = shdr->sh_addr;
	      phdr->p_paddr = phdr->p_vaddr;
	      phdr->p_filesz = shdr->sh_size;
	      phdr->p_memsz = phdr->p_filesz;
	      phdr->p_flags = 0;	/* No need to set PF_R or so.  */
	      phdr->p_align = 1;	/* It's a string.  */

	      (void) xelf_update_phdr (ld_state.outelf, idx, phdr);
	      ++idx;
	    }

	  /* The pointer to the dynamic section.  We this we need to
	     get the information for the dynamic section first.  */
	  assert (ld_state.dynamicscnidx);
	  outscn = elf_getscn (ld_state.outelf, ld_state.dynamicscnidx);
	  xelf_getshdr (outscn, shdr);
	  assert (shdr != NULL);

	  xelf_getphdr_ptr (ld_state.outelf, idx, phdr);
	  phdr->p_type = PT_DYNAMIC;
	  phdr->p_offset = shdr->sh_offset;
	  phdr->p_vaddr = shdr->sh_addr;
	  phdr->p_paddr = phdr->p_vaddr;
	  phdr->p_filesz = shdr->sh_size;
	  phdr->p_memsz = phdr->p_filesz;
	  phdr->p_flags = 0;		/* No need to set PF_R or so.  */
	  phdr->p_align = shdr->sh_addralign;

	  (void) xelf_update_phdr (ld_state.outelf, idx, phdr);

	  /* Fill in the reference to the .dynstr section.  */
	  assert (ld_state.dynstrscnidx != 0);
	  shdr->sh_link = ld_state.dynstrscnidx;
	  (void) xelf_update_shdr (outscn, shdr);

	  /* And fill the remaining entries.  */
	  Elf_Data *dyndata = elf_getdata (outscn, NULL);
	  assert (dyndata != NULL);

	  /* Add the DT_NEEDED entries.  */
	  if (ld_state.ndsofiles > 0)
	    {
	      struct usedfiles *runp = ld_state.dsofiles->next;

	      do
		if (runp->used || !runp->as_needed)
		  {
		    /* Add the position-dependent flag if necessary.  */
		    if (runp->lazyload)
		      new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
					 DT_POSFLAG_1, DF_P1_LAZYLOAD);

		    new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
				       DT_NEEDED,
				       ebl_strtaboffset (runp->sonameent));
		  }
	      while ((runp = runp->next) != ld_state.dsofiles->next);
	    }

	  /* We can finish the DT_RUNPATH/DT_RPATH entries now.  */
	  if (ld_state.rxxpath_strent != NULL)
	    new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
			       ld_state.rxxpath_tag,
			       ebl_strtaboffset (ld_state.rxxpath_strent));

	  /* Reference to initialization and finalization functions.  */
	  // XXX This code depends on symbol table being relocated.
	  if (ld_state.init_symbol != NULL)
	    {
	      XElf_Sym_vardef (sym);

	      if (ld_state.need_symtab)
		xelf_getsym (symdata,
			     dblindirect[ld_state.init_symbol->outsymidx],
			     sym);
	      else
		xelf_getsym (dynsymdata, ld_state.init_symbol->outdynsymidx,
			     sym);
	      assert (sym != NULL);

	      new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
				 DT_INIT, sym->st_value);
	    }
	  if (ld_state.fini_symbol != NULL)
	    {
	      XElf_Sym_vardef (sym);

	      if (ld_state.need_symtab)
		xelf_getsym (symdata,
			     dblindirect[ld_state.fini_symbol->outsymidx],
			     sym);
	      else
		xelf_getsym (dynsymdata, ld_state.fini_symbol->outdynsymidx,
			     sym);
	      assert (sym != NULL);

	      new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
				 DT_FINI, sym->st_value);
	    }
	  // XXX Support init,fini,preinit arrays

	  /* The hash table which comes with dynamic symbol table.  */
	  xelf_getshdr (elf_getscn (ld_state.outelf, ld_state.hashscnidx),
			shdr);
	  assert (shdr != NULL);
	  new_dynamic_entry (dyndata, ld_state.ndynamic_filled++, DT_HASH,
			     shdr->sh_addr);

	  /* Reference to the symbol table section.  */
	  assert (ld_state.dynsymscnidx != 0);
	  xelf_getshdr (elf_getscn (ld_state.outelf, ld_state.dynsymscnidx),
			shdr);
	  assert (shdr != NULL);
	  new_dynamic_entry (dyndata, ld_state.ndynamic_filled++, DT_SYMTAB,
			     shdr->sh_addr);

	  new_dynamic_entry (dyndata, ld_state.ndynamic_filled++, DT_SYMENT,
			     xelf_fsize (ld_state.outelf, ELF_T_SYM, 1));

	  /* And the string table which comes with it.  */
	  xelf_getshdr (elf_getscn (ld_state.outelf, ld_state.dynstrscnidx),
			shdr);
	  assert (shdr != NULL);
	  new_dynamic_entry (dyndata, ld_state.ndynamic_filled++, DT_STRTAB,
			     shdr->sh_addr);

	  new_dynamic_entry (dyndata, ld_state.ndynamic_filled++, DT_STRSZ,
			     shdr->sh_size);

	  /* Add the entries related to the .plt.  */
	  if (ld_state.nplt > 0)
	    {
	      // XXX Make this work if there is no PLT
	      xelf_getshdr (elf_getscn (ld_state.outelf,
					ld_state.gotpltscnidx), shdr);
	      assert (shdr != NULL);
	      new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
				 // XXX This should probably be machine
				 // dependent.
				 DT_PLTGOT, shdr->sh_addr);

	      xelf_getshdr (elf_getscn (ld_state.outelf,
					ld_state.pltrelscnidx), shdr);
	      assert (shdr != NULL);
	      new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
				 DT_PLTRELSZ, shdr->sh_size);

	      new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
				 DT_JMPREL, shdr->sh_addr);

	      new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
				 DT_PLTREL, REL_TYPE (statep));
	    }

	  if (ld_state.relsize_total > 0)
	    {
	      int rel = REL_TYPE (statep);
	      xelf_getshdr (elf_getscn (ld_state.outelf,
					ld_state.reldynscnidx), shdr);
	      assert (shdr != NULL);
	      new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
				 rel, shdr->sh_addr);

	      /* Trick ahead.  Use arithmetic to get the right tag.
		 We check the validity of this assumption in the asserts.  */
	      assert (DT_RELASZ - DT_RELA == 1);
	      assert (DT_RELSZ - DT_REL == 1);
	      new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
				 rel + 1, shdr->sh_size);

	      /* Similar for the entry size tag.  */
	      assert (DT_RELAENT - DT_RELA == 2);
	      assert (DT_RELENT - DT_REL == 2);
	      new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
				 rel + 2,
				 rel == DT_REL
				 ? xelf_fsize (ld_state.outelf, ELF_T_REL, 1)
				 : xelf_fsize (ld_state.outelf, ELF_T_RELA,
					       1));
	    }

	  if (ld_state.verneedscnidx != 0)
	    {
	      xelf_getshdr (elf_getscn (ld_state.outelf,
					ld_state.verneedscnidx), shdr);
	      assert (shdr != NULL);
	      new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
				 DT_VERNEED, shdr->sh_addr);

	      new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
				 DT_VERNEEDNUM, ld_state.nverdeffile);
	    }

	  if (ld_state.versymscnidx != 0)
	    {
	      xelf_getshdr (elf_getscn (ld_state.outelf,
					ld_state.versymscnidx), shdr);
	      assert (shdr != NULL);
	      new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
				 DT_VERSYM, shdr->sh_addr);
	    }

	  /* We always create the DT_DEBUG entry.  */
	  new_dynamic_entry (dyndata, ld_state.ndynamic_filled++, DT_DEBUG, 0);
	  assert (ld_state.ndynamic_filled < ld_state.ndynamic);

	  /* Add the flag words if necessary.  */
	  if (ld_state.dt_flags != 0)
	    new_dynamic_entry (dyndata, ld_state.ndynamic_filled++, DT_FLAGS,
			       ld_state.dt_flags);

	  /* Create entry for the DT_FLAGS_1 flag.  */
	  if (ld_state.dt_flags_1 != 0)
	    new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
			       DT_FLAGS_1, ld_state.dt_flags_1);

	  /* Create entry for the DT_FEATURE_1 flag.  */
	  if (ld_state.dt_feature_1 != 0)
	    new_dynamic_entry (dyndata, ld_state.ndynamic_filled++,
			       DT_FEATURE_1, ld_state.dt_feature_1);

	  assert (ld_state.ndynamic_filled <= ld_state.ndynamic);
	}
    }


  // XXX The following code isn't nice.  We use two different
  // mechanisms to handle relocations, one for relocatable files, one
  // for executables and DSOs.  Maybe this is the best method but also
  // maybe it can be somewhat unified.

  /* Now that we created the symbol table we can add the reference to
     it in the sh_link field of the section headers of the relocation
     sections.  */
  while (rellist != NULL)
    {
      assert (ld_state.file_type == relocatable_file_type);
      Elf_Scn *outscn;

      outscn = elf_getscn (ld_state.outelf, rellist->scnidx);
      xelf_getshdr (outscn, shdr);
      /* This must not fail since we did it before.  */
      assert (shdr != NULL);

      /* Remember the symbol table which belongs to the relocation section.  */
      shdr->sh_link = ld_state.symscnidx;

      /* And the reference to the section which is relocated by this
	 relocation section.  We use the info from the first input
	 section but all records should have the same information.  */
      shdr->sh_info =
	rellist->scninfo->fileinfo->scninfo[SCNINFO_SHDR (rellist->scninfo->shdr).sh_info].outscnndx;


      /* Perform the actual relocations.  We only have to adjust
	 offsets and symbol indices.  */
      RELOCATE_SECTION (statep, outscn, rellist->scninfo, dblindirect);

      /* Store the changes.  */
      (void) xelf_update_shdr (outscn, shdr);

      /* Up to the next relocation section.  */
      rellist = rellist->next;
    }

  if (ld_state.rellist != NULL)
    {
      assert (ld_state.file_type != relocatable_file_type);
      /* Create the relocations for the output file.  */
      CREATE_RELOCATIONS (statep, dblindirect);
    }


  /* We need the ELF header once more.  */
  xelf_getehdr (ld_state.outelf, ehdr);
  assert (ehdr != NULL);

  /* Set the section header string table index.  */
  if (likely (shstrtab_ndx < SHN_HIRESERVE)
      && likely (shstrtab_ndx != SHN_XINDEX))
    ehdr->e_shstrndx = shstrtab_ndx;
  else
    {
      /* We have to put the section index in the sh_link field of the
	 zeroth section header.  */
      Elf_Scn *scn = elf_getscn (ld_state.outelf, 0);

      xelf_getshdr (scn, shdr);
      if (unlikely (shdr == NULL))
	error (EXIT_FAILURE, 0,
	       gettext ("cannot get header of 0th section: %s"),
	       elf_errmsg (-1));

      shdr->sh_link = shstrtab_ndx;

      (void) xelf_update_shdr (scn, shdr);

      ehdr->e_shstrndx = SHN_XINDEX;
    }

  if (ld_state.file_type != relocatable_file_type)
    /* DSOs and executables have to define the entry point symbol.  */
    ehdr->e_entry = find_entry_point ();

  if (unlikely (xelf_update_ehdr (ld_state.outelf, ehdr) == 0))
    error (EXIT_FAILURE, 0,
	   gettext ("cannot update ELF header: %s"),
	   elf_errmsg (-1));


  /* Free the data which we don't need anymore.  */
  free (ld_state.dblindirect);


  /* Finalize the .plt section and what else belongs to it.  */
  FINALIZE_PLT (statep, nsym, nsym_local, ndxtosym);


  /* Finally, if we have to compute the build ID.  */
  if (ld_state.build_id != NULL)
    compute_build_id ();


  /* We don't need the map from the symbol table index to the symbol
     structure anymore.  */
  free (ndxtosym);

  return 0;
}


/* This is a function which must be specified in all backends.  */
static void
ld_generic_relocate_section (struct ld_state *statep, Elf_Scn *outscn,
			     struct scninfo *firstp,
			     const Elf32_Word *dblindirect)
{
  error (EXIT_FAILURE, 0, gettext ("\
linker backend didn't specify function to relocate section"));
  /* NOTREACHED */
}


/* Finalize the output file.  */
static int
ld_generic_finalize (struct ld_state *statep)
{
  /* Write out the ELF file data.  */
  if (elf_update (ld_state.outelf, ELF_C_WRITE) == -1)
      error (EXIT_FAILURE, 0, gettext ("while writing output file: %s"),
	     elf_errmsg (-1));

  /* Free the resources.  */
  if (elf_end (ld_state.outelf) != 0)
    error (EXIT_FAILURE, 0, gettext ("while finishing output file: %s"),
	   elf_errmsg (-1));

  /* Get the file status of the temporary file.  */
  struct stat temp_st;
  if (fstat (ld_state.outfd, &temp_st) != 0)
    error (EXIT_FAILURE, errno, gettext ("cannot stat output file"));

  /* Now it's time to rename the file.  Remove an old existing file
     first.  */
  if (rename (ld_state.tempfname, ld_state.outfname) != 0)
    /* Something went wrong.  */
    error (EXIT_FAILURE, errno, gettext ("cannot rename output file"));

  /* Make sure the output file is really the one we created.  */
  struct stat new_st;
  if (stat (ld_state.outfname, &new_st) != 0
      || new_st.st_ino != temp_st.st_ino
      || new_st.st_dev != temp_st.st_dev)
    {
      /* Wow, somebody overwrote the output file, probably some intruder.  */
      unlink (ld_state.outfname);
      error (EXIT_FAILURE, 0, gettext ("\
WARNING: temporary output file overwritten before linking finished"));
    }

  /* Close the file descriptor.  */
  (void) close (ld_state.outfd);

  /* Signal the cleanup handler that the file is correctly created.  */
  ld_state.tempfname = NULL;

  return 0;
}


static bool
ld_generic_special_section_number_p (struct ld_state *statep, size_t number)
{
  /* There are no special section numbers in the gABI.  */
  return false;
}


static bool
ld_generic_section_type_p (struct ld_state *statep, GElf_Word type)
{
  if (type < SHT_NUM
      /* XXX Enable the following two when implemented.  */
      // || type == SHT_GNU_LIBLIST
      // || type == SHT_CHECKSUM
      /* XXX Eventually include SHT_SUNW_move, SHT_SUNW_COMDAT, and
	 SHT_SUNW_syminfo.  */
      || (type >= SHT_GNU_verdef && type <= SHT_GNU_versym))
    return true;

  return false;
}


static XElf_Xword
ld_generic_dynamic_section_flags (struct ld_state *statep)
{
  /* By default the .dynamic section is writable (and is of course
     loaded).  Few architecture differ from this.  */
  return SHF_ALLOC | SHF_WRITE;
}


static void
ld_generic_initialize_plt (struct ld_state *statep, Elf_Scn *scn)
{
  /* This cannot be implemented generally.  There should have been a
     machine dependent implementation and we should never have arrived
     here.  */
  error (EXIT_FAILURE, 0, gettext ("no machine specific '%s' implementation"),
	 "initialize_plt");
}


static void
ld_generic_initialize_pltrel (struct ld_state *statep, Elf_Scn *scn)
{
  /* This cannot be implemented generally.  There should have been a
     machine dependent implementation and we should never have arrived
     here.  */
  error (EXIT_FAILURE, 0, gettext ("no machine specific '%s' implementation"),
	 "initialize_pltrel");
}


static void
ld_generic_initialize_got (struct ld_state *statep, Elf_Scn *scn)
{
  /* This cannot be implemented generally.  There should have been a
     machine dependent implementation and we should never have arrived
     here.  */
  error (EXIT_FAILURE, 0, gettext ("no machine specific '%s' implementation"),
	 "initialize_got");
}


static void
ld_generic_initialize_gotplt (struct ld_state *statep, Elf_Scn *scn)
{
  /* This cannot be implemented generally.  There should have been a
     machine dependent implementation and we should never have arrived
     here.  */
  error (EXIT_FAILURE, 0, gettext ("no machine specific '%s' implementation"),
	 "initialize_gotplt");
}


static void
ld_generic_finalize_plt (struct ld_state *statep, size_t nsym, size_t nsym_dyn,
			 struct symbol **ndxtosymp)
{
  /* By default we assume that nothing has to be done.  */
}


static int
ld_generic_rel_type (struct ld_state *statep)
{
  /* This cannot be implemented generally.  There should have been a
     machine dependent implementation and we should never have arrived
     here.  */
  error (EXIT_FAILURE, 0, gettext ("no machine specific '%s' implementation"),
	 "rel_type");
  /* Just to keep the compiler calm.  */
  return 0;
}


static void
ld_generic_count_relocations (struct ld_state *statep, struct scninfo *scninfo)
{
  /* This cannot be implemented generally.  There should have been a
     machine dependent implementation and we should never have arrived
     here.  */
  error (EXIT_FAILURE, 0, gettext ("no machine specific '%s' implementation"),
	 "count_relocations");
}


static void
ld_generic_create_relocations (struct ld_state *statep,
			       const Elf32_Word *dblindirect)
{
  /* This cannot be implemented generally.  There should have been a
     machine dependent implementation and we should never have arrived
     here.  */
  error (EXIT_FAILURE, 0, gettext ("no machine specific '%s' implementation"),
	 "create_relocations");
}
