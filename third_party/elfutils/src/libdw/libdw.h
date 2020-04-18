/* Interfaces for libdw.
   Copyright (C) 2002-2010, 2013 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#ifndef _LIBDW_H
#define _LIBDW_H	1

#include <gelf.h>
#include <stdbool.h>
#include <stddef.h>


#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
# define __nonnull_attribute__(...) __attribute__ ((__nonnull__ (__VA_ARGS__)))
# define __deprecated_attribute__ __attribute__ ((__deprecated__))
#else
# define __nonnull_attribute__(args...)
# define __deprecated_attribute__
#endif


#ifdef __GNUC_STDC_INLINE__
# define __libdw_extern_inline extern __inline __attribute__ ((__gnu_inline__))
#else
# define __libdw_extern_inline extern __inline
#endif


/* Mode for the session.  */
typedef enum
  {
    DWARF_C_READ,		/* Read .. */
    DWARF_C_RDWR,		/* Read and write .. */
    DWARF_C_WRITE,		/* Write .. */
  }
Dwarf_Cmd;


/* Callback results.  */
enum
{
  DWARF_CB_OK = 0,
  DWARF_CB_ABORT
};


/* Error values.  */
enum
  {
    DW_TAG_invalid = 0
#define DW_TAG_invalid	DW_TAG_invalid
  };


/* Type for offset in DWARF file.  */
typedef GElf_Off Dwarf_Off;

/* Type for address in DWARF file.  */
typedef GElf_Addr Dwarf_Addr;

/* Integer types.  Big enough to hold any numeric value.  */
typedef GElf_Xword Dwarf_Word;
typedef GElf_Sxword Dwarf_Sword;
/* For the times we know we do not need that much.  */
typedef GElf_Half Dwarf_Half;


/* DWARF abbreviation record.  */
typedef struct Dwarf_Abbrev Dwarf_Abbrev;

/* Returned to show the last DIE has be returned.  */
#define DWARF_END_ABBREV ((Dwarf_Abbrev *) -1l)

/* Source code line information for CU.  */
typedef struct Dwarf_Lines_s Dwarf_Lines;

/* One source code line information.  */
typedef struct Dwarf_Line_s Dwarf_Line;

/* Source file information.  */
typedef struct Dwarf_Files_s Dwarf_Files;

/* One address range record.  */
typedef struct Dwarf_Arange_s Dwarf_Arange;

/* Address ranges of a file.  */
typedef struct Dwarf_Aranges_s Dwarf_Aranges;

/* CU representation.  */
struct Dwarf_CU;

/* Macro information.  */
typedef struct Dwarf_Macro_s Dwarf_Macro;

/* Attribute representation.  */
typedef struct
{
  unsigned int code;
  unsigned int form;
  unsigned char *valp;
  struct Dwarf_CU *cu;
} Dwarf_Attribute;


/* Data block representation.  */
typedef struct
{
  Dwarf_Word length;
  unsigned char *data;
} Dwarf_Block;


/* DIE information.  */
typedef struct
{
  /* The offset can be computed from the address.  */
  void *addr;
  struct Dwarf_CU *cu;
  Dwarf_Abbrev *abbrev;
  // XXX We'll see what other information will be needed.
  long int padding__;
} Dwarf_Die;

/* Returned to show the last DIE has be returned.  */
#define DWARF_END_DIE ((Dwarf_Die *) -1l)


/* Global symbol information.  */
typedef struct
{
  Dwarf_Off cu_offset;
  Dwarf_Off die_offset;
  const char *name;
} Dwarf_Global;


/* One operation in a DWARF location expression.
   A location expression is an array of these.  */
typedef struct
{
  uint8_t atom;			/* Operation */
  Dwarf_Word number;		/* Operand */
  Dwarf_Word number2;		/* Possible second operand */
  Dwarf_Word offset;		/* Offset in location expression */
} Dwarf_Op;


/* This describes one Common Information Entry read from a CFI section.
   Pointers here point into the DATA->d_buf block passed to dwarf_next_cfi.  */
typedef struct
{
  Dwarf_Off CIE_id;	 /* Always DW_CIE_ID_64 in Dwarf_CIE structures.  */

  /* Instruction stream describing initial state used by FDEs.  If
     we did not understand the whole augmentation string and it did
     not use 'z', then there might be more augmentation data here
     (and in FDEs) before the actual instructions.  */
  const uint8_t *initial_instructions;
  const uint8_t *initial_instructions_end;

  Dwarf_Word code_alignment_factor;
  Dwarf_Sword data_alignment_factor;
  Dwarf_Word return_address_register;

  const char *augmentation;	/* Augmentation string.  */

  /* Augmentation data, might be NULL.  The size is correct only if
     we understood the augmentation string sufficiently.  */
  const uint8_t *augmentation_data;
  size_t augmentation_data_size;
  size_t fde_augmentation_data_size;
} Dwarf_CIE;

/* This describes one Frame Description Entry read from a CFI section.
   Pointers here point into the DATA->d_buf block passed to dwarf_next_cfi.  */
typedef struct
{
  /* Section offset of CIE this FDE refers to.  This will never be
     DW_CIE_ID_64 in an FDE.  If this value is DW_CIE_ID_64, this is
     actually a Dwarf_CIE structure.  */
  Dwarf_Off CIE_pointer;

  /* We can't really decode anything further without looking up the CIE
     and checking its augmentation string.  Here follows the encoded
     initial_location and address_range, then any augmentation data,
     then the instruction stream.  This FDE describes PC locations in
     the byte range [initial_location, initial_location+address_range).
     When the CIE augmentation string uses 'z', the augmentation data is
     a DW_FORM_block (self-sized).  Otherwise, when we understand the
     augmentation string completely, fde_augmentation_data_size gives
     the number of bytes of augmentation data before the instructions.  */
  const uint8_t *start;
  const uint8_t *end;
} Dwarf_FDE;

/* Each entry in a CFI section is either a CIE described by Dwarf_CIE or
   an FDE described by Dward_FDE.  Check CIE_id to see which you have.  */
typedef union
{
  Dwarf_Off CIE_id;	 /* Always DW_CIE_ID_64 in Dwarf_CIE structures.  */
  Dwarf_CIE cie;
  Dwarf_FDE fde;
} Dwarf_CFI_Entry;

#define dwarf_cfi_cie_p(entry)	((entry)->cie.CIE_id == DW_CIE_ID_64)

/* Opaque type representing a frame state described by CFI.  */
typedef struct Dwarf_Frame_s Dwarf_Frame;

/* Opaque type representing a CFI section found in a DWARF or ELF file.  */
typedef struct Dwarf_CFI_s Dwarf_CFI;


/* Handle for debug sessions.  */
typedef struct Dwarf Dwarf;


/* Out-Of-Memory handler.  */
#if __GNUC__ < 4
typedef void (*Dwarf_OOM) (void);
#else
typedef void (*__attribute__ ((noreturn)) Dwarf_OOM) (void);
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* Create a handle for a new debug session.  */
extern Dwarf *dwarf_begin (int fildes, Dwarf_Cmd cmd);

/* Create a handle for a new debug session for an ELF file.  */
extern Dwarf *dwarf_begin_elf (Elf *elf, Dwarf_Cmd cmd, Elf_Scn *scngrp);

/* Retrieve ELF descriptor used for DWARF access.  */
extern Elf *dwarf_getelf (Dwarf *dwarf);

/* Release debugging handling context.  */
extern int dwarf_end (Dwarf *dwarf);


/* Get the data block for the .debug_info section.  */
extern Elf_Data *dwarf_getscn_info (Dwarf *dwarf);

/* Read the header for the DWARF CU.  */
extern int dwarf_nextcu (Dwarf *dwarf, Dwarf_Off off, Dwarf_Off *next_off,
			 size_t *header_sizep, Dwarf_Off *abbrev_offsetp,
			 uint8_t *address_sizep, uint8_t *offset_sizep)
     __nonnull_attribute__ (3);

/* Read the header of a DWARF CU or type unit.  If TYPE_SIGNATUREP is not
   null, this reads a type unit from the .debug_types section; otherwise
   this reads a CU from the .debug_info section.  */
extern int dwarf_next_unit (Dwarf *dwarf, Dwarf_Off off, Dwarf_Off *next_off,
			    size_t *header_sizep, Dwarf_Half *versionp,
			    Dwarf_Off *abbrev_offsetp,
			    uint8_t *address_sizep, uint8_t *offset_sizep,
			    uint64_t *type_signaturep, Dwarf_Off *type_offsetp)
     __nonnull_attribute__ (3);


/* Decode one DWARF CFI entry (CIE or FDE) from the raw section data.
   The E_IDENT from the originating ELF file indicates the address
   size and byte order used in the CFI section contained in DATA;
   EH_FRAME_P should be true for .eh_frame format and false for
   .debug_frame format.  OFFSET is the byte position in the section
   to start at; on return *NEXT_OFFSET is filled in with the byte
   position immediately after this entry.

   On success, returns 0 and fills in *ENTRY; use dwarf_cfi_cie_p to
   see whether ENTRY->cie or ENTRY->fde is valid.

   On errors, returns -1.  Some format errors will permit safely
   skipping to the next CFI entry though the current one is unusable.
   In that case, *NEXT_OFF will be updated before a -1 return.

   If there are no more CFI entries left in the section,
   returns 1 and sets *NEXT_OFFSET to (Dwarf_Off) -1.  */
extern int dwarf_next_cfi (const unsigned char e_ident[],
			   Elf_Data *data, bool eh_frame_p,
			   Dwarf_Off offset, Dwarf_Off *next_offset,
			   Dwarf_CFI_Entry *entry)
  __nonnull_attribute__ (1, 2, 5, 6);

/* Use the CFI in the DWARF .debug_frame section.
   Returns NULL if there is no such section (not an error).
   The pointer returned can be used until dwarf_end is called on DWARF,
   and must not be passed to dwarf_cfi_end.
   Calling this more than once returns the same pointer.  */
extern Dwarf_CFI *dwarf_getcfi (Dwarf *dwarf);

/* Use the CFI in the ELF file's exception-handling data.
   Returns NULL if there is no such data.
   The pointer returned can be used until elf_end is called on ELF,
   and must be passed to dwarf_cfi_end before then.
   Calling this more than once allocates independent data structures.  */
extern Dwarf_CFI *dwarf_getcfi_elf (Elf *elf);

/* Release resources allocated by dwarf_getcfi_elf.  */
extern int dwarf_cfi_end (Dwarf_CFI *cache);


/* Return DIE at given offset in .debug_info section.  */
extern Dwarf_Die *dwarf_offdie (Dwarf *dbg, Dwarf_Off offset,
				Dwarf_Die *result) __nonnull_attribute__ (3);

/* Return DIE at given offset in .debug_types section.  */
extern Dwarf_Die *dwarf_offdie_types (Dwarf *dbg, Dwarf_Off offset,
				      Dwarf_Die *result)
     __nonnull_attribute__ (3);

/* Return offset of DIE.  */
extern Dwarf_Off dwarf_dieoffset (Dwarf_Die *die);

/* Return offset of DIE in CU.  */
extern Dwarf_Off dwarf_cuoffset (Dwarf_Die *die);

/* Return CU DIE containing given DIE.  */
extern Dwarf_Die *dwarf_diecu (Dwarf_Die *die, Dwarf_Die *result,
			       uint8_t *address_sizep, uint8_t *offset_sizep)
     __nonnull_attribute__ (2);

/* Return CU DIE containing given address.  */
extern Dwarf_Die *dwarf_addrdie (Dwarf *dbg, Dwarf_Addr addr,
				 Dwarf_Die *result) __nonnull_attribute__ (3);

/* Return child of current DIE.  */
extern int dwarf_child (Dwarf_Die *die, Dwarf_Die *result)
     __nonnull_attribute__ (2);

/* Locates the first sibling of DIE and places it in RESULT.
   Returns 0 if a sibling was found, -1 if something went wrong.
   Returns 1 if no sibling could be found and, if RESULT is not
   the same as DIE, it sets RESULT->addr to the address of the
   (non-sibling) DIE that follows this one, or NULL if this DIE
   was the last one in the compilation unit.  */
extern int dwarf_siblingof (Dwarf_Die *die, Dwarf_Die *result)
     __nonnull_attribute__ (2);

/* Check whether the DIE has children.  */
extern int dwarf_haschildren (Dwarf_Die *die) __nonnull_attribute__ (1);

/* Walks the attributes of DIE, starting at the one OFFSET bytes in,
   calling the CALLBACK function for each one.  Stops if the callback
   function ever returns a value other than DWARF_CB_OK and returns the
   offset of the offending attribute.  If the end of the attributes
   is reached 1 is returned.  If something goes wrong -1 is returned and
   the dwarf error number is set.  */
extern ptrdiff_t dwarf_getattrs (Dwarf_Die *die,
				 int (*callback) (Dwarf_Attribute *, void *),
				 void *arg, ptrdiff_t offset)
     __nonnull_attribute__ (2);

/* Return tag of given DIE.  */
extern int dwarf_tag (Dwarf_Die *die) __nonnull_attribute__ (1);


/* Return specific attribute of DIE.  */
extern Dwarf_Attribute *dwarf_attr (Dwarf_Die *die, unsigned int search_name,
				    Dwarf_Attribute *result)
     __nonnull_attribute__ (3);

/* Check whether given DIE has specific attribute.  */
extern int dwarf_hasattr (Dwarf_Die *die, unsigned int search_name);

/* These are the same as dwarf_attr and dwarf_hasattr, respectively,
   but they resolve an indirect attribute through DW_AT_abstract_origin.  */
extern Dwarf_Attribute *dwarf_attr_integrate (Dwarf_Die *die,
					      unsigned int search_name,
					      Dwarf_Attribute *result)
     __nonnull_attribute__ (3);
extern int dwarf_hasattr_integrate (Dwarf_Die *die, unsigned int search_name);




/* Check whether given attribute has specific form.  */
extern int dwarf_hasform (Dwarf_Attribute *attr, unsigned int search_form);

/* Return attribute code of given attribute.  */
extern unsigned int dwarf_whatattr (Dwarf_Attribute *attr);

/* Return form code of given attribute.  */
extern unsigned int dwarf_whatform (Dwarf_Attribute *attr);


/* Return string associated with given attribute.  */
extern const char *dwarf_formstring (Dwarf_Attribute *attrp);

/* Return unsigned constant represented by attribute.  */
extern int dwarf_formudata (Dwarf_Attribute *attr, Dwarf_Word *return_uval)
     __nonnull_attribute__ (2);

/* Return signed constant represented by attribute.  */
extern int dwarf_formsdata (Dwarf_Attribute *attr, Dwarf_Sword *return_uval)
     __nonnull_attribute__ (2);

/* Return address represented by attribute.  */
extern int dwarf_formaddr (Dwarf_Attribute *attr, Dwarf_Addr *return_addr)
     __nonnull_attribute__ (2);

/* This function is deprecated.  Always use dwarf_formref_die instead.
   Return reference offset represented by attribute.  */
extern int dwarf_formref (Dwarf_Attribute *attr, Dwarf_Off *return_offset)
     __nonnull_attribute__ (2) __deprecated_attribute__;

/* Look up the DIE in a reference-form attribute.  */
extern Dwarf_Die *dwarf_formref_die (Dwarf_Attribute *attr, Dwarf_Die *die_mem)
     __nonnull_attribute__ (2);

/* Return block represented by attribute.  */
extern int dwarf_formblock (Dwarf_Attribute *attr, Dwarf_Block *return_block)
     __nonnull_attribute__ (2);

/* Return flag represented by attribute.  */
extern int dwarf_formflag (Dwarf_Attribute *attr, bool *return_bool)
     __nonnull_attribute__ (2);


/* Simplified attribute value access functions.  */

/* Return string in name attribute of DIE.  */
extern const char *dwarf_diename (Dwarf_Die *die);

/* Return high PC attribute of DIE.  */
extern int dwarf_highpc (Dwarf_Die *die, Dwarf_Addr *return_addr)
     __nonnull_attribute__ (2);

/* Return low PC attribute of DIE.  */
extern int dwarf_lowpc (Dwarf_Die *die, Dwarf_Addr *return_addr)
     __nonnull_attribute__ (2);

/* Return entry_pc or low_pc attribute of DIE.  */
extern int dwarf_entrypc (Dwarf_Die *die, Dwarf_Addr *return_addr)
     __nonnull_attribute__ (2);

/* Return 1 if DIE's lowpc/highpc or ranges attributes match the PC address,
   0 if not, or -1 for errors.  */
extern int dwarf_haspc (Dwarf_Die *die, Dwarf_Addr pc);

/* Enumerate the PC address ranges covered by this DIE, covering all
   addresses where dwarf_haspc returns true.  In the first call OFFSET
   should be zero and *BASEP need not be initialized.  Returns -1 for
   errors, zero when there are no more address ranges to report, or a
   nonzero OFFSET value to pass to the next call.  Each subsequent call
   must preserve *BASEP from the prior call.  Successful calls fill in
   *STARTP and *ENDP with a contiguous address range.  */
extern ptrdiff_t dwarf_ranges (Dwarf_Die *die,
			       ptrdiff_t offset, Dwarf_Addr *basep,
			       Dwarf_Addr *startp, Dwarf_Addr *endp);


/* Return byte size attribute of DIE.  */
extern int dwarf_bytesize (Dwarf_Die *die);

/* Return bit size attribute of DIE.  */
extern int dwarf_bitsize (Dwarf_Die *die);

/* Return bit offset attribute of DIE.  */
extern int dwarf_bitoffset (Dwarf_Die *die);

/* Return array order attribute of DIE.  */
extern int dwarf_arrayorder (Dwarf_Die *die);

/* Return source language attribute of DIE.  */
extern int dwarf_srclang (Dwarf_Die *die);


/* Get abbreviation at given offset for given DIE.  */
extern Dwarf_Abbrev *dwarf_getabbrev (Dwarf_Die *die, Dwarf_Off offset,
				      size_t *lengthp);

/* Get abbreviation at given offset in .debug_abbrev section.  */
extern int dwarf_offabbrev (Dwarf *dbg, Dwarf_Off offset, size_t *lengthp,
			    Dwarf_Abbrev *abbrevp)
     __nonnull_attribute__ (4);

/* Get abbreviation code.  */
extern unsigned int dwarf_getabbrevcode (Dwarf_Abbrev *abbrev);

/* Get abbreviation tag.  */
extern unsigned int dwarf_getabbrevtag (Dwarf_Abbrev *abbrev);

/* Return true if abbreviation is children flag set.  */
extern int dwarf_abbrevhaschildren (Dwarf_Abbrev *abbrev);

/* Get number of attributes of abbreviation.  */
extern int dwarf_getattrcnt (Dwarf_Abbrev *abbrev, size_t *attrcntp)
     __nonnull_attribute__ (2);

/* Get specific attribute of abbreviation.  */
extern int dwarf_getabbrevattr (Dwarf_Abbrev *abbrev, size_t idx,
				unsigned int *namep, unsigned int *formp,
				Dwarf_Off *offset);


/* Get string from-debug_str section.  */
extern const char *dwarf_getstring (Dwarf *dbg, Dwarf_Off offset,
				    size_t *lenp);


/* Get public symbol information.  */
extern ptrdiff_t dwarf_getpubnames (Dwarf *dbg,
				    int (*callback) (Dwarf *, Dwarf_Global *,
						     void *),
				    void *arg, ptrdiff_t offset)
     __nonnull_attribute__ (2);


/* Get source file information for CU.  */
extern int dwarf_getsrclines (Dwarf_Die *cudie, Dwarf_Lines **lines,
			      size_t *nlines) __nonnull_attribute__ (2, 3);

/* Return one of the source lines of the CU.  */
extern Dwarf_Line *dwarf_onesrcline (Dwarf_Lines *lines, size_t idx);

/* Get the file source files used in the CU.  */
extern int dwarf_getsrcfiles (Dwarf_Die *cudie, Dwarf_Files **files,
			      size_t *nfiles)
     __nonnull_attribute__ (2);


/* Get source for address in CU.  */
extern Dwarf_Line *dwarf_getsrc_die (Dwarf_Die *cudie, Dwarf_Addr addr);

/* Get source for file and line number.  */
extern int dwarf_getsrc_file (Dwarf *dbg, const char *fname, int line, int col,
			      Dwarf_Line ***srcsp, size_t *nsrcs)
     __nonnull_attribute__ (2, 5, 6);


/* Return line address.  */
extern int dwarf_lineaddr (Dwarf_Line *line, Dwarf_Addr *addrp);

/* Return line VLIW operation index.  */
extern int dwarf_lineop_index (Dwarf_Line *line, unsigned int *op_indexp);

/* Return line number.  */
extern int dwarf_lineno (Dwarf_Line *line, int *linep)
     __nonnull_attribute__ (2);

/* Return column in line.  */
extern int dwarf_linecol (Dwarf_Line *line, int *colp)
     __nonnull_attribute__ (2);

/* Return true if record is for beginning of a statement.  */
extern int dwarf_linebeginstatement (Dwarf_Line *line, bool *flagp)
     __nonnull_attribute__ (2);

/* Return true if record is for end of sequence.  */
extern int dwarf_lineendsequence (Dwarf_Line *line, bool *flagp)
     __nonnull_attribute__ (2);

/* Return true if record is for beginning of a basic block.  */
extern int dwarf_lineblock (Dwarf_Line *line, bool *flagp)
     __nonnull_attribute__ (2);

/* Return true if record is for end of prologue.  */
extern int dwarf_lineprologueend (Dwarf_Line *line, bool *flagp)
     __nonnull_attribute__ (2);

/* Return true if record is for beginning of epilogue.  */
extern int dwarf_lineepiloguebegin (Dwarf_Line *line, bool *flagp)
     __nonnull_attribute__ (2);

/* Return instruction-set architecture in this record.  */
extern int dwarf_lineisa (Dwarf_Line *line, unsigned int *isap)
     __nonnull_attribute__ (2);

/* Return code path discriminator in this record.  */
extern int dwarf_linediscriminator (Dwarf_Line *line, unsigned int *discp)
     __nonnull_attribute__ (2);


/* Find line information for address.  */
extern const char *dwarf_linesrc (Dwarf_Line *line,
				  Dwarf_Word *mtime, Dwarf_Word *length);

/* Return file information.  */
extern const char *dwarf_filesrc (Dwarf_Files *file, size_t idx,
				  Dwarf_Word *mtime, Dwarf_Word *length);

/* Return the directory list used in the file information extracted.
   (*RESULT)[0] is the CU's DW_AT_comp_dir value, and may be null.
   (*RESULT)[0..*NDIRS-1] are the compile-time include directory path
   encoded by the compiler.  */
extern int dwarf_getsrcdirs (Dwarf_Files *files,
			     const char *const **result, size_t *ndirs)
  __nonnull_attribute__ (2, 3);


/* Return location expression, decoded as a list of operations.  */
extern int dwarf_getlocation (Dwarf_Attribute *attr, Dwarf_Op **expr,
			      size_t *exprlen) __nonnull_attribute__ (2, 3);

/* Return location expressions.  If the attribute uses a location list,
   ADDRESS selects the relevant location expressions from the list.
   There can be multiple matches, resulting in multiple expressions to
   return.  EXPRS and EXPRLENS are parallel arrays of NLOCS slots to
   fill in.  Returns the number of locations filled in, or -1 for
   errors.  If EXPRS is a null pointer, stores nothing and returns the
   total number of locations.  A return value of zero means that the
   location list indicated no value is accessible.  */
extern int dwarf_getlocation_addr (Dwarf_Attribute *attr, Dwarf_Addr address,
				   Dwarf_Op **exprs, size_t *exprlens,
				   size_t nlocs);

/* Enumerate the locations ranges and descriptions covered by the
   given attribute.  In the first call OFFSET should be zero and
   *BASEP need not be initialized.  Returns -1 for errors, zero when
   there are no more locations to report, or a nonzero OFFSET
   value to pass to the next call.  Each subsequent call must preserve
   *BASEP from the prior call.  Successful calls fill in *STARTP and
   *ENDP with a contiguous address range and *EXPR with a pointer to
   an array of operations with length *EXPRLEN.  If the attribute
   describes a single location description and not a location list the
   first call (with OFFSET zero) will return the location description
   in *EXPR with *STARTP set to zero and *ENDP set to minus one.  */
extern ptrdiff_t dwarf_getlocations (Dwarf_Attribute *attr,
				     ptrdiff_t offset, Dwarf_Addr *basep,
				     Dwarf_Addr *startp, Dwarf_Addr *endp,
				     Dwarf_Op **expr, size_t *exprlen);

/* Return the block associated with a DW_OP_implicit_value operation.
   The OP pointer must point into an expression that dwarf_getlocation
   or dwarf_getlocation_addr has returned given the same ATTR.  */
extern int dwarf_getlocation_implicit_value (Dwarf_Attribute *attr,
					     const Dwarf_Op *op,
					     Dwarf_Block *return_block)
  __nonnull_attribute__ (2, 3);

/* Return the attribute indicated by a DW_OP_GNU_implicit_pointer operation.
   The OP pointer must point into an expression that dwarf_getlocation
   or dwarf_getlocation_addr has returned given the same ATTR.
   The result is the DW_AT_location or DW_AT_const_value attribute
   of the OP->number DIE.  */
extern int dwarf_getlocation_implicit_pointer (Dwarf_Attribute *attr,
					       const Dwarf_Op *op,
					       Dwarf_Attribute *result)
  __nonnull_attribute__ (2, 3);

/* Return the DIE associated with an operation such as
   DW_OP_GNU_implicit_pointer, DW_OP_GNU_parameter_ref, DW_OP_GNU_convert,
   DW_OP_GNU_reinterpret, DW_OP_GNU_const_type, DW_OP_GNU_regval_type or
   DW_OP_GNU_deref_type.  The OP pointer must point into an expression that
   dwarf_getlocation or dwarf_getlocation_addr has returned given the same
   ATTR.  The RESULT is a DIE that expresses a type or value needed by the
   given OP.  */
extern int dwarf_getlocation_die (Dwarf_Attribute *attr,
				  const Dwarf_Op *op,
				  Dwarf_Die *result)
  __nonnull_attribute__ (2, 3);

/* Return the attribute expressing a value associated with an operation such
   as DW_OP_implicit_value, DW_OP_GNU_entry_value or DW_OP_GNU_const_type.
   The OP pointer must point into an expression that dwarf_getlocation
   or dwarf_getlocation_addr has returned given the same ATTR.
   The RESULT is a value expressed by an attribute such as DW_AT_location
   or DW_AT_const_value.  */
extern int dwarf_getlocation_attr (Dwarf_Attribute *attr,
				   const Dwarf_Op *op,
				   Dwarf_Attribute *result)
  __nonnull_attribute__ (2, 3);


/* Compute the byte-size of a type DIE according to DWARF rules.
   For most types, this is just DW_AT_byte_size.
   For DW_TAG_array_type it can apply much more complex rules.  */
extern int dwarf_aggregate_size (Dwarf_Die *die, Dwarf_Word *size);


/* Return scope DIEs containing PC address.
   Sets *SCOPES to a malloc'd array of Dwarf_Die structures,
   and returns the number of elements in the array.
   (*SCOPES)[0] is the DIE for the innermost scope containing PC,
   (*SCOPES)[1] is the DIE for the scope containing that scope, and so on.
   Returns -1 for errors or 0 if no scopes match PC.  */
extern int dwarf_getscopes (Dwarf_Die *cudie, Dwarf_Addr pc,
			    Dwarf_Die **scopes);

/* Return scope DIEs containing the given DIE.
   Sets *SCOPES to a malloc'd array of Dwarf_Die structures,
   and returns the number of elements in the array.
   (*SCOPES)[0] is a copy of DIE.
   (*SCOPES)[1] is the DIE for the scope containing that scope, and so on.
   Returns -1 for errors or 0 if DIE is not found in any scope entry.  */
extern int dwarf_getscopes_die (Dwarf_Die *die, Dwarf_Die **scopes);


/* Search SCOPES[0..NSCOPES-1] for a variable called NAME.
   Ignore the first SKIP_SHADOWS scopes that match the name.
   If MATCH_FILE is not null, accept only declaration in that source file;
   if MATCH_LINENO or MATCH_LINECOL are also nonzero, accept only declaration
   at that line and column.

   If successful, fill in *RESULT with the DIE of the variable found,
   and return N where SCOPES[N] is the scope defining the variable.
   Return -1 for errors or -2 for no matching variable found.  */
extern int dwarf_getscopevar (Dwarf_Die *scopes, int nscopes,
			      const char *name, int skip_shadows,
			      const char *match_file,
			      int match_lineno, int match_linecol,
			      Dwarf_Die *result);



/* Return list address ranges.  */
extern int dwarf_getaranges (Dwarf *dbg, Dwarf_Aranges **aranges,
			     size_t *naranges)
     __nonnull_attribute__ (2);

/* Return one of the address range entries.  */
extern Dwarf_Arange *dwarf_onearange (Dwarf_Aranges *aranges, size_t idx);

/* Return information in address range record.  */
extern int dwarf_getarangeinfo (Dwarf_Arange *arange, Dwarf_Addr *addrp,
				Dwarf_Word *lengthp, Dwarf_Off *offsetp);

/* Get address range which includes given address.  */
extern Dwarf_Arange *dwarf_getarange_addr (Dwarf_Aranges *aranges,
					   Dwarf_Addr addr);



/* Get functions in CUDIE.  The given callback will be called for all
   defining DW_TAG_subprograms in the CU DIE tree.  If the callback
   returns DWARF_CB_ABORT the return value can be used as offset argument
   to resume the function to find all remaining functions (this is not
   really recommended, since it needs to rewalk the CU DIE tree first till
   that offset is found again).  If the callback returns DWARF_CB_OK
   dwarf_getfuncs will not return but keep calling the callback for each
   function DIE it finds.  Pass zero for offset on the first call to walk
   the full CU DIE tree.  If no more functions can be found and the callback
   returned DWARF_CB_OK then the function returns zero.  */
extern ptrdiff_t dwarf_getfuncs (Dwarf_Die *cudie,
				 int (*callback) (Dwarf_Die *, void *),
				 void *arg, ptrdiff_t offset);


/* Return file name containing definition of the given declaration.  */
extern const char *dwarf_decl_file (Dwarf_Die *decl);

/* Get line number of beginning of given declaration.  */
extern int dwarf_decl_line (Dwarf_Die *decl, int *linep)
     __nonnull_attribute__ (2);

/* Get column number of beginning of given declaration.  */
extern int dwarf_decl_column (Dwarf_Die *decl, int *colp)
     __nonnull_attribute__ (2);


/* Return nonzero if given function is an abstract inline definition.  */
extern int dwarf_func_inline (Dwarf_Die *func);

/* Find each concrete inlined instance of the abstract inline definition.  */
extern int dwarf_func_inline_instances (Dwarf_Die *func,
					int (*callback) (Dwarf_Die *, void *),
					void *arg);


/* Find the appropriate PC location or locations for function entry
   breakpoints for the given DW_TAG_subprogram DIE.  Returns -1 for errors.
   On success, returns the number of breakpoint locations (never zero)
   and sets *BKPTS to a malloc'd vector of addresses.  */
extern int dwarf_entry_breakpoints (Dwarf_Die *die, Dwarf_Addr **bkpts);


/* Call callback function for each of the macro information entry for
   the CU.  */
extern ptrdiff_t dwarf_getmacros (Dwarf_Die *cudie,
				  int (*callback) (Dwarf_Macro *, void *),
				  void *arg, ptrdiff_t offset)
     __nonnull_attribute__ (2);

/* Return macro opcode.  */
extern int dwarf_macro_opcode (Dwarf_Macro *macro, unsigned int *opcodep)
     __nonnull_attribute__ (2);

/* Return first macro parameter.  */
extern int dwarf_macro_param1 (Dwarf_Macro *macro, Dwarf_Word *paramp)
     __nonnull_attribute__ (2);

/* Return second macro parameter.  */
extern int dwarf_macro_param2 (Dwarf_Macro *macro, Dwarf_Word *paramp,
			       const char **strp);


/* Compute what's known about a call frame when the PC is at ADDRESS.
   Returns 0 for success or -1 for errors.
   On success, *FRAME is a malloc'd pointer.  */
extern int dwarf_cfi_addrframe (Dwarf_CFI *cache,
				Dwarf_Addr address, Dwarf_Frame **frame)
  __nonnull_attribute__ (3);

/* Return the DWARF register number used in FRAME to denote
   the return address in FRAME's caller frame.  The remaining
   arguments can be non-null to fill in more information.

   Fill [*START, *END) with the PC range to which FRAME's information applies.
   Fill in *SIGNALP to indicate whether this is a signal-handling frame.
   If true, this is the implicit call frame that calls a signal handler.
   This frame's "caller" is actually the interrupted state, not a call;
   its return address is an exact PC, not a PC after a call instruction.  */
extern int dwarf_frame_info (Dwarf_Frame *frame,
			     Dwarf_Addr *start, Dwarf_Addr *end, bool *signalp);

/* Return a DWARF expression that yields the Canonical Frame Address at
   this frame state.  Returns -1 for errors, or zero for success, with
   *NOPS set to the number of operations stored at *OPS.  That pointer
   can be used only as long as FRAME is alive and unchanged.  *NOPS is
   zero if the CFA cannot be determined here.  Note that if nonempty,
   *OPS is a DWARF expression, not a location description--append
   DW_OP_stack_value to a get a location description for the CFA.  */
extern int dwarf_frame_cfa (Dwarf_Frame *frame, Dwarf_Op **ops, size_t *nops)
  __nonnull_attribute__ (2);

/* Deliver a DWARF location description that yields the location or
   value of DWARF register number REGNO in the state described by FRAME.

   Returns -1 for errors or zero for success, setting *NOPS to the
   number of operations in the array stored at *OPS.  Note the last
   operation is DW_OP_stack_value if there is no mutable location but
   only a computable value.

   *NOPS zero with *OPS set to OPS_MEM means CFI says the caller's
   REGNO is "undefined", i.e. it's call-clobbered and cannot be recovered.

   *NOPS zero with *OPS set to a null pointer means CFI says the
   caller's REGNO is "same_value", i.e. this frame did not change it;
   ask the caller frame where to find it.

   For common simple expressions *OPS is OPS_MEM.  For arbitrary DWARF
   expressions in the CFI, *OPS is an internal pointer that can be used as
   long as the Dwarf_CFI used to create FRAME remains alive.  */
extern int dwarf_frame_register (Dwarf_Frame *frame, int regno,
				 Dwarf_Op ops_mem[3],
				 Dwarf_Op **ops, size_t *nops)
  __nonnull_attribute__ (3, 4, 5);


/* Return error code of last failing function call.  This value is kept
   separately for each thread.  */
extern int dwarf_errno (void);

/* Return error string for ERROR.  If ERROR is zero, return error string
   for most recent error or NULL is none occurred.  If ERROR is -1 the
   behaviour is similar to the last case except that not NULL but a legal
   string is returned.  */
extern const char *dwarf_errmsg (int err);


/* Register new Out-Of-Memory handler.  The old handler is returned.  */
extern Dwarf_OOM dwarf_new_oom_handler (Dwarf *dbg, Dwarf_OOM handler);


/* Inline optimizations.  */
#ifdef __OPTIMIZE__
/* Return attribute code of given attribute.  */
__libdw_extern_inline unsigned int
dwarf_whatattr (Dwarf_Attribute *attr)
{
  return attr == NULL ? 0 : attr->code;
}

/* Return attribute code of given attribute.  */
__libdw_extern_inline unsigned int
dwarf_whatform (Dwarf_Attribute *attr)
{
  return attr == NULL ? 0 : attr->form;
}
#endif	/* Optimize.  */

#ifdef __cplusplus
}
#endif

#endif	/* libdw.h */
