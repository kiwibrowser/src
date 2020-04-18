/* Copyright (C) 2001, 2002, 2003, 2005, 2006, 2008, 2009 Red Hat, Inc.
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

#ifndef LD_H
#define LD_H	1

#include <dlfcn.h>
#include <obstack.h>
#include <stdbool.h>
#include <stdio.h>
#include "xelf.h"


/* Recommended size of the buffer passed to ld_strerror.  */
#define ERRBUFSIZE	(512)

/* Character used to introduce version name after symbol.  */
#define VER_CHR	'@'


/* Methods for handling archives.  */
enum extract_rule
  {
    defaultextract,	/* Weak references don't cause archive member to
			   be used.  */
    weakextract,	/* Weak references cause archive member to be
			   extracted.  */
    allextract		/* Extract all archive members regardless of
			   references (aka whole-archive).  */
  };


/* Type of output file.  */
enum file_type
  {
    no_file_type = 0,		/* None selected so far.  */
    executable_file_type,	/* Executable.  */
    dso_file_type,		/* DSO.  */
    dso_needed_file_type,	/* DSO introduced by DT_NEEDED.  */
    relocatable_file_type,	/* Relocatable object file.  */
    archive_file_type		/* Archive (input only).  */
  };


struct usedfiles
{
  /* The next file given at the command line.  */
  struct usedfiles *next;
  /* Nonzero if this file is the beginning of a group.  */
  bool group_start;
  /* Nonzero if this file is the end of a group.  */
  bool group_end;
  /* Pointer to the beginning of the group.  It is necessary to
     explain why we cannot simply use the 'next' pointer and have a
     circular single-linked list like in many cases.  The problem is
     that the last archive of the group, if it is the last file of the
     group, contains the only existing pointer to the next file we
     have to look at.  All files are initially connected via the
     'next' pointer in a single-linked list.  Therefore we cannot
     overwrite this value.  It instead will be used once the group is
     handled and we go on processing the rest of the files.  */
  struct usedfiles *group_backref;

  /* Name/path of the file.  */
  const char *fname;
  /* Resolved file name.  */
  const char *rfname;
  /* Name used as reference in DT_NEEDED entries.  This is normally
     the SONAME.  If it is missing it's normally the fname above.  */
  const char *soname;
  /* Handle for the SONAME in the string table.  */
  struct Ebl_Strent *sonameent;

  /* Help to identify duplicates.  */
  dev_t dev;
  ino_t ino;

  enum
    {
      not_opened,
      opened,
      in_archive,
      closed
    } status;

  /* How to extract elements from archives.  */
  enum extract_rule extract_rule;

  /* Lazy-loading rule.  */
  bool lazyload;

  /* If this is a DSO the flag indicates whether the file is directly
     used in a reference.  */
  bool used;

  /* True when file should be added to DT_NEEDED list only when
     directly referenced.  */
  bool as_needed;

  /* If nonzero this is the archive sequence number which can be used to
     determine whether back refernces from -( -) or GROUP statements
     have to be followed.  */
  int archive_seq;

  /* Pointer to the record for the archive containing this file.  */
  struct usedfiles *archive_file;

  /* Type of file.  We have to distinguish these types since they
     are searched for differently.  */
  enum file_type file_type;
  /* This is the ELF library handle for this file.  */
  Elf *elf;

  /* The ELF header.  */
#if NATIVE_ELF != 0
  XElf_Ehdr *ehdr;
# define FILEINFO_EHDR(fi) (*(fi))
#else
  XElf_Ehdr ehdr;
# define FILEINFO_EHDR(fi) (fi)
#endif

  /* Index of the section header string table section.  We use a
     separate field and not the e_shstrndx field in the ELF header
     since in case of a file with more than 64000 sections the index
     might be stored in the section header of section zero.  The
     elf_getshdrstrndx() function can find the value but it is too
     costly to repeat this call over and over.  */
  size_t shstrndx;

  /* Info about the sections of the file.  */
  struct scninfo
  {
    /* Handle for the section.  Note that we can store a section
       handle here because the file is not changing.  This together
       with the knowledge about the libelf library is enough for us to
       assume the section reference remains valid at all times.  */
    Elf_Scn *scn;
    /* Section header.  */
#if NATIVE_ELF != 0
    XElf_Shdr *shdr;
# define SCNINFO_SHDR(si) (*(si))
#else
    XElf_Shdr shdr;
# define SCNINFO_SHDR(si) (si)
#endif
    /* Offset of this files section in the combined section.  */
    XElf_Off offset;
    /* Index of the section in the output file.  */
    Elf32_Word outscnndx;
    /* Index of the output section in the 'allsection' array.  */
    Elf32_Word allsectionsidx;
    /* True if the section is used.  */
    bool used;
    /* True if section is an unused COMDAT section.  */
    bool unused_comdat;
    /* True if this is a COMDAT group section.  */
    bool comdat_group;
    /* Section group number.  This is the index of the SHT_GROUP section.  */
    Elf32_Word grpid;
    /* Pointer back to the containing file information structure.  */
    struct usedfiles *fileinfo;
    /* List of symbols in this section (set only for merge-able sections
       and group sections).  */
    struct symbol *symbols;
    /* Size of relocations in this section.  Only used for relocation
       sections.  */
    size_t relsize;
    /* Pointer to next section which is put in the given output
       section.  */
    struct scninfo *next;
  } *scninfo;

  /* List of section group sections.  */
  struct scninfo *groups;

  /* The symbol table section.

     XXX Maybe support for more than one symbol table is needed.  */
  Elf_Data *symtabdata;
  /* Extra section index table section.  */
  Elf_Data *xndxdata;
  /* Dynamic symbol table section.  */
  Elf_Data *dynsymtabdata;
  /* The version number section.  */
  Elf_Data *versymdata;
  /* The defined versions.  */
  Elf_Data *verdefdata;
  /* Number of versions defined.  */
  size_t nverdef;
  /* True if the version with the given index number is used in the
     output.  */
  XElf_Versym *verdefused;
  /* How many versions are used.  */
  size_t nverdefused;
  /* Handle for name of the version.  */
  struct Ebl_Strent **verdefent;
  /* The needed versions.  */
  Elf_Data *verneeddata;
  /* String table section associated with the symbol table.  */
  Elf32_Word symstridx;
  /* String table section associated with the dynamic symbol table.  */
  Elf32_Word dynsymstridx;
  /* Number of entries in the symbol table.  */
  size_t nsymtab;
  size_t nlocalsymbols;
  size_t ndynsymtab;
  /* Dynamic section.  */
  Elf_Scn *dynscn;

  /* Indirection table for the symbols defined here.  */
  Elf32_Word *symindirect;
  Elf32_Word *dynsymindirect;
  /* For undefined or common symbols we need a reference to the symbol
     record.  */
  struct symbol **symref;
  struct symbol **dynsymref;

  /* This is the file descriptor.  The value is -1 if the descriptor
     was already closed.  This can happen if we needed file descriptors
     to open new files.  */
  int fd;
  /* This flag is true if the descriptor was passed to the generic
     functions from somewhere else.  This is an implementation detail;
     no machine-specific code must use this flag.  */
  bool fd_passed;

  /* True if any of the sections is merge-able.  */
  bool has_merge_sections;
};


/* Functions to test for the various types of files we handle.  */
static inline int
ld_file_rel_p (struct usedfiles *file)
{
  return (elf_kind (file->elf) == ELF_K_ELF
	  && FILEINFO_EHDR (file->ehdr).e_type == ET_REL);
}

static inline int
ld_file_dso_p (struct usedfiles *file)
{
  return (elf_kind (file->elf) == ELF_K_ELF
	  && FILEINFO_EHDR (file->ehdr).e_type == ET_DYN);
}

static inline int
ld_file_ar_p (struct usedfiles *file)
{
  return elf_kind (file->elf) == ELF_K_AR;
}


struct pathelement
{
  /* The next path to search.  */
  struct pathelement *next;
  /* The path name.  */
  const char *pname;
  /* Larger than zero if the directory exists, smaller than zero if not,
     zero if it is not yet known.  */
  int exist;
};


/* Forward declaration.  */
struct ld_state;


/* Callback functions.  */
struct callbacks
{
  /* Library names passed to the linker as -lXXX represent files named
     libXXX.YY.  The YY part can have different forms, depending on the
     architecture.  The generic set is .so and .a (in this order).  */
  const char **(*lib_extensions) (struct ld_state *)
       __attribute__ ((__const__));
#define LIB_EXTENSION(state) \
  DL_CALL_FCT ((state)->callbacks.lib_extensions, (state))

  /* Process the given file.  If the file is not yet open, open it.
     The first parameter is a file descriptor for the file which can
     be -1 to indicate the file has not yet been found.  The second
     parameter describes the file to be opened, the last one is the
     state of the linker which among other information contain the
     paths we look at.*/
  int (*file_process) (int fd, struct usedfiles *, struct ld_state *,
		       struct usedfiles **);
#define FILE_PROCESS(fd, file, state, nextp) \
  DL_CALL_FCT ((state)->callbacks.file_process, (fd, file, state, nextp))

  /* Close the given file.  */
  int (*file_close) (struct usedfiles *, struct ld_state *);
#define FILE_CLOSE(file, state) \
  DL_CALL_FCT ((state)->callbacks.file_close, (file, state))

  /* Create the output sections now.  This requires knowledge about
     all the sections we will need.  It may be necessary to sort the
     sections in the order they are supposed to appear in the
     executable.  The sorting use many different kinds of information
     to optimize the resulting binary.  Important is to respect
     segment boundaries and the needed alignment.  The mode of the
     segments will be determined afterwards automatically by the
     output routines.  */
  void (*create_sections) (struct ld_state *);
#define CREATE_SECTIONS(state) \
  DL_CALL_FCT ((state)->callbacks.create_sections, (state))

  /* Determine whether we have any non-weak unresolved references left.  */
  int (*flag_unresolved) (struct ld_state *);
#define FLAG_UNRESOLVED(state) \
  DL_CALL_FCT ((state)->callbacks.flag_unresolved, (state))

  /* Create the sections which are generated by the linker and are not
     present in the input file.  */
  void (*generate_sections) (struct ld_state *);
#define GENERATE_SECTIONS(state) \
  DL_CALL_FCT ((state)->callbacks.generate_sections, (state))

  /* Open the output file.  The file name is given or "a.out".  We
     create as much of the ELF structure as possible.  */
  int (*open_outfile) (struct ld_state *, int, int, int);
#define OPEN_OUTFILE(state, machine, class, data) \
  DL_CALL_FCT ((state)->callbacks.open_outfile, (state, machine, class, data))

  /* Create the data for the output file.  */
  int (*create_outfile) (struct ld_state *);
#define CREATE_OUTFILE(state) \
  DL_CALL_FCT ((state)->callbacks.create_outfile, (state))

  /* Process a relocation section.  */
  void (*relocate_section) (struct ld_state *, Elf_Scn *, struct scninfo *,
			    const Elf32_Word *);
#define RELOCATE_SECTION(state, outscn, first, dblindirect) \
  DL_CALL_FCT ((state)->callbacks.relocate_section, (state, outscn, first,    \
						     dblindirect))

  /* Allocate a data buffer for the relocations of the given output
     section.  */
  void (*count_relocations) (struct ld_state *, struct scninfo *);
#define COUNT_RELOCATIONS(state, scninfo) \
  DL_CALL_FCT ((state)->callbacks.count_relocations, (state, scninfo))

  /* Create relocations for executable or DSO.  */
  void (*create_relocations) (struct ld_state *, const Elf32_Word *);
#define CREATE_RELOCATIONS(state, dlbindirect) \
  DL_CALL_FCT ((state)->callbacks.create_relocations, (state, dblindirect))

  /* Finalize the output file.  */
  int (*finalize) (struct ld_state *);
#define FINALIZE(state) \
  DL_CALL_FCT ((state)->callbacks.finalize, (state))

  /* Check whether special section number is known.  */
  bool (*special_section_number_p) (struct ld_state *, size_t);
#define SPECIAL_SECTION_NUMBER_P(state, number) \
  DL_CALL_FCT ((state)->callbacks.special_section_number_p, (state, number))

  /* Check whether section type is known.  */
  bool (*section_type_p) (struct ld_state *, XElf_Word);
#define SECTION_TYPE_P(state, type) \
  DL_CALL_FCT ((state)->callbacks.section_type_p, (state, type))

  /* Return section flags for .dynamic section.  */
  XElf_Xword (*dynamic_section_flags) (struct ld_state *);
#define DYNAMIC_SECTION_FLAGS(state) \
  DL_CALL_FCT ((state)->callbacks.dynamic_section_flags, (state))

  /* Create the data structures for the .plt section and initialize it.  */
  void (*initialize_plt) (struct ld_state *, Elf_Scn *scn);
#define INITIALIZE_PLT(state, scn) \
  DL_CALL_FCT ((state)->callbacks.initialize_plt, (state, scn))

  /* Create the data structures for the .rel.plt section and initialize it.  */
  void (*initialize_pltrel) (struct ld_state *, Elf_Scn *scn);
#define INITIALIZE_PLTREL(state, scn) \
  DL_CALL_FCT ((state)->callbacks.initialize_pltrel, (state, scn))

  /* Finalize the .plt section the what belongs to them.  */
  void (*finalize_plt) (struct ld_state *, size_t, size_t, struct symbol **);
#define FINALIZE_PLT(state, nsym, nsym_dyn, ndxtosym) \
  DL_CALL_FCT ((state)->callbacks.finalize_plt, (state, nsym, nsym_dyn, \
						 ndxtosym))

  /* Create the data structures for the .got section and initialize it.  */
  void (*initialize_got) (struct ld_state *, Elf_Scn *scn);
#define INITIALIZE_GOT(state, scn) \
  DL_CALL_FCT ((state)->callbacks.initialize_got, (state, scn))

  /* Create the data structures for the .got.plt section and initialize it.  */
  void (*initialize_gotplt) (struct ld_state *, Elf_Scn *scn);
#define INITIALIZE_GOTPLT(state, scn) \
  DL_CALL_FCT ((state)->callbacks.initialize_gotplt, (state, scn))

  /* Return the tag corresponding to the native relocation type for
     the platform.  */
  int (*rel_type) (struct ld_state *);
#define REL_TYPE(state) \
  DL_CALL_FCT ((state)->callbacks.rel_type, (state))
};


/* Structure for symbol representation.  This data structure is used a
   lot, so size is important.  */
struct symbol
{
  /* Symbol name.  */
  const char *name;
  /* Size of the object.  */
  XElf_Xword size;
  /* Index of the symbol in the symbol table of the object.  */
  size_t symidx;
  /* Index of the symbol in the symbol table of the output file.  */
  size_t outsymidx;

  /* Description where the symbol is found/needed.  */
  size_t scndx;
  struct usedfiles *file;
  /* Index of the symbol table.  */
  Elf32_Word symscndx;

  /* Index of the symbol in the dynamic symbol table of the output
     file.  Note that the value only needs to be 16 bit wide since
     there cannot be more sections in an executable or DSO.  */
  unsigned int outdynsymidx:16;

  /* Type of the symbol.  */
  unsigned int type:4;
  /* Various flags.  */
  unsigned int defined:1;
  unsigned int common:1;
  unsigned int weak:1;
  unsigned int added:1;
  unsigned int merged:1;
  unsigned int local:1;
  unsigned int hidden:1;
  /* Nonzero if the symbol is on the from_dso list.  */
  unsigned int on_dsolist:1;
  /* Nonzero if symbol needs copy relocation, reset when the
     relocation has been created.  */
  unsigned int need_copy:1;
  unsigned int in_dso:1;

  union
  {
    /* Pointer to the handle created by the functions which create
       merged section contents.  We use 'void *' because there are
       different implementations used.  */
    void *handle;
    XElf_Addr value;
  } merge;

  /* Pointer to next/previous symbol on whatever list the symbol is.  */
  struct symbol *next;
  struct symbol *previous;
  /* Pointer to next symbol of the same section (only set for merge-able
     sections).  */
  struct symbol *next_in_scn;
};


/* Get the definition for the symbol table.  */
#include <symbolhash.h>

/* Simple single linked list of file names.  */
struct filename_list
{
  const char *name;
  struct usedfiles *real;
  struct filename_list *next;
  bool group_start;
  bool group_end;
  bool as_needed;
};


/* Data structure to describe expression in linker script.  */
struct expression
{
  enum expression_tag
    {
      exp_num,
      exp_sizeof_headers,
      exp_pagesize,
      exp_id,
      exp_mult,
      exp_div,
      exp_mod,
      exp_plus,
      exp_minus,
      exp_and,
      exp_or,
      exp_align
    } tag;

  union
  {
    uintmax_t num;
    struct expression *child;
    struct
    {
      struct expression *left;
      struct expression *right;
    } binary;
    const char *str;
  } val;
};


/* Data structure for section name with flags.  */
struct input_section_name
{
  const char *name;
  bool sort_flag;
};

/* File name mask with section name.  */
struct filemask_section_name
{
  const char *filemask;
  const char *excludemask;
  struct input_section_name *section_name;
  bool keep_flag;
};

/* Data structure for assignments.  */
struct assignment
{
  const char *variable;
  struct expression *expression;
  struct symbol *sym;
  bool provide_flag;
};


/* Data structure describing input for an output section.  */
struct input_rule
{
  enum
    {
      input_section,
      input_assignment
    } tag;

  union
  {
    struct assignment *assignment;
    struct filemask_section_name *section;
  } val;

  struct input_rule *next;
};


/* Data structure to describe output section.  */
struct output_section
{
  const char *name;
  struct input_rule *input;
  XElf_Addr max_alignment;
  bool ignored;
};


/* Data structure to describe output file format.  */
struct output_rule
{
  enum
    {
      output_section,
      output_assignment
    } tag;

  union
  {
    struct assignment *assignment;
    struct output_section section;
  } val;

  struct output_rule *next;
};


/* List of all the segments the linker script describes.  */
struct output_segment
{
  int mode;
  struct output_rule *output_rules;
  struct output_segment *next;

  XElf_Off offset;
  XElf_Addr addr;
  XElf_Xword align;
};


/* List of identifiers.  */
struct id_list
{
  union
  {
    enum id_type
      {
	id_str,		/* Normal string.  */
	id_all,		/* "*", matches all.  */
	id_wild		/* Globbing wildcard string.  */
      } id_type;
    struct
    {
      bool local;
      const char *versionname;
    } s;
  } u;
  const char *id;
  struct id_list *next;
};


/* Version information.  */
struct version
{
  struct version *next;
  struct id_list *local_names;
  struct id_list *global_names;
  const char *versionname;
  const char *parentname;
};


/* Head for list of sections.  */
struct scnhead
{
  /* Name of the sections.  */
  const char *name;

  /* Accumulated flags for the sections.  */
  XElf_Xword flags;

  /* Type of the sections.  */
  XElf_Word type;

  /* Entry size.  If there are differencs between the sections with
     the same name this field contains 1.  */
  XElf_Word entsize;

  /* If non-NULL pointer to group signature.  */
  const char *grp_signature;

  /* Maximum alignment for all sections.  */
  XElf_Word align;

  /* Distinguish between normal sections coming from the input file
     and sections generated by the linker.  */
  enum scn_kind
    {
      scn_normal,		/* Section from the input file(s).  */
      scn_dot_interp,		/* Generated .interp section.  */
      scn_dot_got,		/* Generated .got section.  */
      scn_dot_gotplt,		/* Generated .got.plt section.  */
      scn_dot_dynrel,		/* Generated .rel.dyn section.  */
      scn_dot_dynamic,		/* Generated .dynamic section.  */
      scn_dot_dynsym,		/* Generated .dynsym section.  */
      scn_dot_dynstr,		/* Generated .dynstr section.  */
      scn_dot_hash,		/* Generated .hash section.  */
      scn_dot_gnu_hash,		/* Generated .gnu.hash section.  */
      scn_dot_plt,		/* Generated .plt section.  */
      scn_dot_pltrel,		/* Generated .rel.plt section.  */
      scn_dot_version,		/* Generated .gnu.version section.  */
      scn_dot_version_r,	/* Generated .gnu.version_r section.  */
      scn_dot_note_gnu_build_id	/* Generated .note.gnu.build-id section.  */
    } kind;

  /* True is the section is used in the output.  */
  bool used;

  /* Total size (only determined this way for relocation sections).  */
  size_t relsize;

  /* Filled in by the section sorting to indicate which segment the
     section goes in.  */
  int segment_nr;

  /* Index of the output section.  We cannot store the section handle
     directly here since the handle is a pointer in a dynamically
     allocated table which might move if it becomes too small for all
     the sections.  Using the index the correct value can be found at
     all times.  */
  XElf_Word scnidx;

  /* Index of the STT_SECTION entry for this section in the symbol
     table.  */
  XElf_Word scnsymidx;

  /* Address of the section in the output file.  */
  XElf_Addr addr;

  /* Handle for the section name in the output file's section header
     string table.  */
  struct Ebl_Strent *nameent;

  /* Tail of list of symbols for this section.  Only set if the
     section is merge-able.  */
  struct symbol *symbols;

  /* Pointer to last section.  */
  struct scninfo *last;
};


/* Define hash table for sections.  */
#include <sectionhash.h>

/* Define hash table for version symbols.  */
#include <versionhash.h>


/* State of the linker.  */
struct ld_state
{
  /* ELF backend library handle.  */
  Ebl *ebl;

  /* List of all archives participating, in this order.  */
  struct usedfiles *archives;
  /* End of the list.  */
  struct usedfiles *tailarchives;
  /* If nonzero we are looking for the beginning of a group.  */
  bool group_start_requested;
  /* Pointer to the archive starting the group.  */
  struct usedfiles *group_start_archive;

  /* List of the DSOs we found.  */
  struct usedfiles *dsofiles;
  /* Number of DSO files.  */
  size_t ndsofiles;
  /* Ultimate list of object files which are linked in.  */
  struct usedfiles *relfiles;

  /* List the DT_NEEDED DSOs.  */
  struct usedfiles *needed;

  /* Temporary storage for the parser.  */
  struct filename_list *srcfiles;

  /* List of all the paths to look at.  */
  struct pathelement *paths;
  /* Tail of the list.  */
  struct pathelement *tailpaths;

  /* User provided paths for lookup of DSOs.  */
  struct pathelement *rpath;
  struct pathelement *rpath_link;
  struct pathelement *runpath;
  struct pathelement *runpath_link;
  struct Ebl_Strent *rxxpath_strent;
  int rxxpath_tag;

  /* From the environment variable LD_LIBRARY_PATH.  */
  struct pathelement *ld_library_path1;
  struct pathelement *ld_library_path2;

  /* Name of the output file.  */
  const char *outfname;
  /* Name of the temporary file we initially create.  */
  const char *tempfname;
  /* File descriptor opened for the output file.  */
  int outfd;
  /* The ELF descriptor for the output file.  */
  Elf *outelf;

  /* Type of output file.  */
  enum file_type file_type;

  /* Is this a system library or not.  */
  bool is_system_library;

  /* Page size to be assumed for the binary.  */
  size_t pagesize;

  /* Name of the interpreter for dynamically linked objects.  */
  const char *interp;
  /* Index of the .interp section.  */
  Elf32_Word interpscnidx;

  /* Optimization level.  */
  unsigned long int optlevel;

  /* If true static linking is requested.  */
  bool statically;

  /* If true, add DT_NEEDED entries for following files if they are
     needed.  */
  bool as_needed;

  /* How to extract elements from archives.  */
  enum extract_rule extract_rule;

  /* Sequence number of the last archive we used.  */
  int last_archive_used;

  /* If true print to stdout information about the files we are
     trying to open.  */
  bool trace_files;

  /* If true multiple definitions are not considered an error; the
     first is used.  */
  bool muldefs;

  /* If true undefined symbols when building DSOs are not fatal.  */
  bool nodefs;

  /* If true add line indentifying link-editor to .comment section.  */
  bool add_ld_comment;

  /* Stripping while linking.  */
  enum
    {
      strip_none,
      strip_debug,
      strip_all,
      strip_everything
    } strip;

  /* The callback function vector.  */
  struct callbacks callbacks;

  /* Name of the entry symbol.  Can also be a numeric value.  */
  const char *entry;

  /* The description of the segments in the output file.  */
  struct output_segment *output_segments;

  /* List of the symbols we created from linker script definitions.  */
  struct symbol *lscript_syms;
  size_t nlscript_syms;

  /* Table with known symbols.  */
  ld_symbol_tab symbol_tab;

  /* Table with used sections.  */
  ld_section_tab section_tab;

  /* The list of sections once we collected them.   */
  struct scnhead **allsections;
  size_t nallsections;
  size_t nusedsections;
  size_t nnotesections;

  /* Beginning of the list of symbols which are still unresolved.  */
  struct symbol *unresolved;
  /* Number of truely unresolved entries in the list.  */
  size_t nunresolved;
  /* Number of truely unresolved, non-weak entries in the list.  */
  size_t nunresolved_nonweak;

  /* List of common symbols.  */
  struct symbol *common_syms;
  /* Section for the common symbols.  */
  struct scninfo *common_section;

  /* List of symbols defined in DSOs and used in a relocatable file.
     DSO symbols not referenced in the relocatable files are not on
     the list.  If a symbol is on the list the on_dsolist field in the
     'struct symbol' is nonzero.  */
  struct symbol *from_dso;
  /* Number of entries in from_dso.  */
  size_t nfrom_dso;
  /* Number of entries in the dynamic symbol table.  */
  size_t ndynsym;
  /* Number of PLT entries from DSO references.  */
  size_t nplt;
  /* Number of PLT entries from DSO references.  */
  size_t ngot;
  /* Number of copy relocations.  */
  size_t ncopy;
  /* Section for copy relocations.  */
  struct scninfo *copy_section;

  /* Keeping track of the number of symbols in the output file.  */
  size_t nsymtab;
  size_t nlocalsymbols;

  /* Special symbols.  */
  struct symbol *init_symbol;
  struct symbol *fini_symbol;

  /* The description of the segments in the output file as described
     in the default linker script.  This information will be used in
     addition to the user-provided information.  */
  struct output_segment *default_output_segments;
  /* Search paths added by the default linker script.  */
  struct pathelement *default_paths;

#ifndef BASE_ELF_NAME
  /* The handle of the ld backend library.  */
  void *ldlib;
#endif

  /* String table for the section headers.  */
  struct Ebl_Strtab *shstrtab;

  /* True if output file should contain symbol table.  */
  bool need_symtab;
  /* Symbol table section.  */
  Elf32_Word symscnidx;
  /* Extended section table section.  */
  Elf32_Word xndxscnidx;
  /* Symbol string table section.  */
  Elf32_Word strscnidx;

  /* True if output file should contain dynamic symbol table.  */
  bool need_dynsym;
  /* Dynamic symbol table section.  */
  Elf32_Word dynsymscnidx;
  /* Dynamic symbol string table section.  */
  Elf32_Word dynstrscnidx;
  /* Dynamic symbol hash tables.  */
  size_t hashscnidx;
  size_t gnuhashscnidx;

  /* Procedure linkage table section.  */
  Elf32_Word pltscnidx;
  /* Number of entries already in the PLT section.  */
  size_t nplt_used;
  /* Relocation for procedure linkage table section.  */
  Elf32_Word pltrelscnidx;

  /* Global offset table section.  */
  Elf32_Word gotscnidx;
  /* And the part of the PLT.  */
  Elf32_Word gotpltscnidx;

  /* This section will hole all non-PLT relocations.  */
  Elf32_Word reldynscnidx;

  /* Index of the sections to handle versioning.  */
  Elf32_Word versymscnidx;
  Elf32_Word verneedscnidx;
  /* XXX Should the following names be verneed...?  */
  /* Number of version definitions in input DSOs used.  */
  int nverdefused;
  /* Number of input DSOs using versioning.  */
  int nverdeffile;
  /* Index of next version.  */
  int nextveridx;

  /* TLS segment.  */
  bool need_tls;
  XElf_Addr tls_start;
  XElf_Addr tls_tcb;

  /* Hash table for version symbol strings.  Only strings without
     special characters are hashed here.  */
  ld_version_str_tab version_str_tab;
  /* At most one of the following two variables is set to true if either
     global or local symbol binding is selected as the default.  */
  bool default_bind_local;
  bool default_bind_global;

  /* Execuatable stack selection.  */
  enum execstack
    {
      execstack_false = 0,
      execstack_true,
      execstack_false_force
    } execstack;

  /* True if only used sections are used.  */
  bool gc_sections;

  /* Array to determine final index of symbol.  */
  Elf32_Word *dblindirect;

  /* Section group handling.  */
  struct scngroup
  {
    Elf32_Word outscnidx;
    int nscns;
    struct member
    {
      struct scnhead *scn;
      struct member *next;
    } *member;
    struct Ebl_Strent *nameent;
    struct symbol *symbol;
    struct scngroup *next;
  } *groups;

  /* True if the output file needs a .got section.  */
  bool need_got;
  /* Number of relocations for GOT section caused.  */
  size_t nrel_got;

  /* Number of entries needed in the .dynamic section.  */
  int ndynamic;
  /* To keep track of added entries.  */
  int ndynamic_filled;
  /* Index for the dynamic section.  */
  Elf32_Word dynamicscnidx;

  /* Flags set in the DT_FLAGS word.  */
  Elf32_Word dt_flags;
  /* Flags set in the DT_FLAGS_1 word.  */
  Elf32_Word dt_flags_1;
  /* Flags set in the DT_FEATURE_1 word.  */
  Elf32_Word dt_feature_1;

  /* Lazy-loading state for dependencies.  */
  bool lazyload;

  /* True if an .eh_frame_hdr section should be generated.  */
  bool eh_frame_hdr;

  /* What hash style to generate.  */
  enum
    {
      hash_style_none = 0,
      hash_style_sysv = 1,
#define GENERATE_SYSV_HASH ((ld_state.hash_style & hash_style_sysv) != 0)
      hash_style_gnu = 2
#define GENERATE_GNU_HASH ((ld_state.hash_style & hash_style_gnu) != 0)
    }
  hash_style;


  /* True if in executables all global symbols should be exported in
     the dynamic symbol table.  */
  bool export_all_dynamic;

  /* Build-ID style.  NULL is none.  */
  const char *build_id;
  Elf32_Word buildidscnidx;

  /* If DSO is generated, this is the SONAME.  */
  const char *soname;

  /* List of all relocation sections.  */
  struct scninfo *rellist;
  /* Total size of non-PLT relocations.  */
  size_t relsize_total;

  /* Record for the GOT symbol, if known.  */
  struct symbol *got_symbol;
  /* Record for the dynamic section symbol, if known.  */
  struct symbol *dyn_symbol;

  /* Obstack used for small objects which will not be deleted.  */
  struct obstack smem;
};


/* The interface to the scanner.  */

/* Parser entry point.  */
extern int ldparse (void);

/* The input file.  */
extern FILE *ldin;

/* Name of the input file.  */
extern const char *ldin_fname;

/* Current line number.  Must be reset for a new file.  */
extern int ldlineno;

/* If nonzero we are currently parsing a version script.  */
extern int ld_scan_version_script;

/* Flags defined in ld.c.  */
extern int verbose;
extern int conserve_memory;


/* Linker state.  This contains all global information.  */
extern struct ld_state ld_state;


/* Generic ld helper functions.  */

/* Append a new directory to search libraries in.  */
extern void ld_new_searchdir (const char *dir);

/* Append a new file to the list of input files.  */
extern struct usedfiles *ld_new_inputfile (const char *fname,
					   enum file_type type);


/* These are the generic implementations for the callbacks used by ld.  */

/* Initialize state object.  This callback function is called after the
   parameters are parsed but before any file is searched for.  */
extern int ld_prepare_state (const char *emulation);


/* Function to determine whether an object will be dynamically linked.  */
extern bool dynamically_linked_p (void);

/* Helper functions for the architecture specific code.  */

/* Checked whether the symbol is undefined and referenced from a DSO.  */
extern bool linked_from_dso_p (struct scninfo *scninfo, size_t symidx);
#ifdef __GNUC_STDC_INLINE__
__attribute__ ((__gnu_inline__))
#endif
extern inline bool
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

#endif	/* ld.h */
