/* Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2008 Red Hat, Inc.
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
#include <error.h>
#include <libintl.h>
#include <stdlib.h>
#include <string.h>

// XXX For debugging
#include <stdio.h>

#include <system.h>
#include "ld.h"
#include "list.h"
/* x86 is little endian.  */
#define UNALIGNED_ACCESS_CLASS LITTLE_ENDIAN
#include "unaligned.h"
#include "xelf.h"


/* The old callbacks.  */
static int (*old_open_outfile) (struct ld_state *, int, int, int);


static int
elf_i386_open_outfile (struct ld_state *statep,
		       int machine __attribute__ ((unused)),
		       int klass __attribute__ ((unused)),
		       int data __attribute__ ((unused)))
{
  /* This backend only handles 32-bit object files.  */
  /* XXX For now just use the generic backend.  */
  return old_open_outfile (statep, EM_386, ELFCLASS32, ELFDATA2LSB);
}


/* Process relocations for the output in a relocatable file.  This
   only means adjusting offset and symbol indices.  */
static void
elf_i386_relocate_section (struct ld_state *statep __attribute__ ((unused)),
			   Elf_Scn *outscn, struct scninfo *firstp,
			   const Elf32_Word *dblindirect)
{
  struct scninfo *runp;
  Elf_Data *data;

  /* Iterate over all the input sections.  Appropriate data buffers in the
     output sections were already created.  */
  runp = firstp;
  data = NULL;
  do
    {
      Elf_Data *reltgtdata;
      Elf_Data *insymdata;
      Elf_Data *inxndxdata = NULL;
      size_t maxcnt;
      size_t cnt;
      const Elf32_Word *symindirect;
      struct symbol **symref;
      struct usedfiles *file = runp->fileinfo;
      XElf_Shdr *shdr = &SCNINFO_SHDR (runp->shdr);

      /* Get the output section data buffer for this input section.  */
      data = elf_getdata (outscn, data);
      assert (data != NULL);

      /* Get the data for section in the input file this relocation
	 section is relocating.  Since these buffers are reused in the
	 output modifying these buffers has the correct result.  */
      reltgtdata = elf_getdata (file->scninfo[shdr->sh_info].scn, NULL);

      /* Get the data for the input section symbol table for this
	 relocation section.  */
      insymdata = elf_getdata (file->scninfo[shdr->sh_link].scn, NULL);
      assert (insymdata != NULL);

      /* And the extended section index table.  */
      inxndxdata = runp->fileinfo->xndxdata;

      /* Number of relocations.  */
      maxcnt = shdr->sh_size / shdr->sh_entsize;

      /* Array directing local symbol table offsets to output symbol
	 table offsets.  */
      symindirect = file->symindirect;

      /* References to the symbol records.  */
      symref = file->symref;

      /* Iterate over all the relocations in the section.  */
      for (cnt = 0; cnt < maxcnt; ++cnt)
	{
	  XElf_Rel_vardef (rel);
	  Elf32_Word si;
	  XElf_Sym_vardef (sym);
	  Elf32_Word xndx;

	  /* Get the relocation data itself.  x86 uses Rel
	     relocations.  In case we have to handle Rela as well the
	     whole loop probably should be duplicated.  */
	  xelf_getrel (data, cnt, rel);
	  assert (rel != NULL);

	  /* Compute the symbol index in the output file.  */
	  si = symindirect[XELF_R_SYM (rel->r_info)];
	  if (si == 0)
	    {
	      /* This happens if the symbol is locally undefined or
		 superceded by some other definition.  */
	      assert (symref[XELF_R_SYM (rel->r_info)] != NULL);
	      si = symref[XELF_R_SYM (rel->r_info)]->outsymidx;
	    }
	  /* Take reordering performed to sort the symbol table into
	     account.  */
	  si = dblindirect[si];

	  /* Get the symbol table entry.  */
	  xelf_getsymshndx (insymdata, inxndxdata, XELF_R_SYM (rel->r_info),
			    sym, xndx);
	  if (sym->st_shndx != SHN_XINDEX)
	    xndx = sym->st_shndx;
	  assert (xndx < SHN_LORESERVE || xndx > SHN_HIRESERVE);

	  /* We fortunately don't have to do much.  The relocations
	     mostly get only updates of the offset.  Only for a
	     relocation referring to a section do we have to do
	     something.  In this case the reference to the sections
	     has no direct equivalent since the part the input section
	     contributes need not start at the same offset as in the
	     input file.  Therefore we have to adjust the addend which
	     in the case of Rel relocations is in the target section
	     itself.  */
	  if (XELF_ST_TYPE (sym->st_info) == STT_SECTION)
	    {
	      /* We expect here only R_386_32 relocations.  */
	      assert (XELF_R_TYPE (rel->r_info) == R_386_32);

	      /* Avoid writing to the section memory if this is
		 effectively a no-op since it might save a
		 copy-on-write operation.  */
	      Elf32_Word toadd = file->scninfo[xndx].offset;
	      if (toadd != 0)
		add_4ubyte_unaligned (reltgtdata->d_buf + rel->r_offset,
				      toadd);
	    }

	  /* Adjust the offset for the position of the input section
	     content in the output section.  */
	  rel->r_offset += file->scninfo[shdr->sh_info].offset;

	  /* And finally adjust the index of the symbol in the output
	     symbol table.  */
	  rel->r_info = XELF_R_INFO (si, XELF_R_TYPE (rel->r_info));

	  /* Store the result.  */
	  (void) xelf_update_rel (data, cnt, rel);
	}

      runp = runp->next;
    }
  while (runp != firstp);
}


/* Each PLT entry has 16 bytes.  We need one entry as overhead for
   the code to set up the call into the runtime relocation.  */
#define PLT_ENTRY_SIZE 16

static void
elf_i386_initialize_plt (struct ld_state *statep, Elf_Scn *scn)
{
  Elf_Data *data;
  XElf_Shdr_vardef (shdr);

  /* Change the entry size in the section header.  */
  xelf_getshdr (scn, shdr);
  assert (shdr != NULL);
  shdr->sh_entsize = PLT_ENTRY_SIZE;
  (void) xelf_update_shdr (scn, shdr);

  data = elf_newdata (scn);
  if (data == NULL)
    error (EXIT_FAILURE, 0, gettext ("cannot allocate PLT section: %s"),
	   elf_errmsg (-1));

  /* We need one special PLT entry (performing the jump to the runtime
     relocation routines) and one for each function we call in a DSO.  */
  data->d_size = (1 + statep->nplt) * PLT_ENTRY_SIZE;
  data->d_buf = xcalloc (1, data->d_size);
  assert (data->d_type == ELF_T_BYTE);
  data->d_off = 0;
  data->d_align = 8;

  statep->nplt_used = 1;
}


static void
elf_i386_initialize_pltrel (struct ld_state *statep, Elf_Scn *scn)
{
  Elf_Data *data;

  data = elf_newdata (scn);
  if (data == NULL)
    error (EXIT_FAILURE, 0, gettext ("cannot allocate PLTREL section: %s"),
	   elf_errmsg (-1));

  /* One relocation per PLT entry.  */
  size_t size = statep->nplt * sizeof (Elf32_Rel);
  data->d_buf = xcalloc (1, size);
  data->d_type = ELF_T_REL;
  data->d_size = size;
  data->d_align = 4;
  data->d_off = 0;
}


static void
elf_i386_initialize_got (struct ld_state *statep, Elf_Scn *scn)
{
  /* If we come here we better need a GOT.  */
  assert (statep->ngot != 0);

  Elf_Data *data = elf_newdata (scn);
  if (data == NULL)
    error (EXIT_FAILURE, 0, gettext ("cannot allocate GOT section: %s"),
	   elf_errmsg (-1));

  /* Just a single word per GOT entry is needed.  */
  size_t size = statep->ngot * sizeof (Elf32_Addr);
  data->d_buf = xcalloc (1, size);
  data->d_size = size;
  data->d_type = ELF_T_WORD;
  data->d_off = 0;
  data->d_align = sizeof (Elf32_Addr);
}


static void
elf_i386_initialize_gotplt (struct ld_state *statep, Elf_Scn *scn)
{
  /* If we come here we better need a PLT.  */
  assert (statep->nplt != 0);

  Elf_Data *data = elf_newdata (scn);
  if (data == NULL)
    error (EXIT_FAILURE, 0, gettext ("cannot allocate GOTPLT section: %s"),
	   elf_errmsg (-1));

  /* We construct the .got.plt section in pieces.  Here we only add the data
     structures which are used by the PLT.  This includes three reserved
     entries at the beginning (the first will contain a pointer to the
     .dynamic section), and one word for each PLT entry.  */
  size_t size = (3 + statep->nplt) * sizeof (Elf32_Addr);
  data->d_buf = xcalloc (1, size);
  data->d_type = ELF_T_WORD;
  data->d_size = size;
  data->d_off = 0;
  data->d_align = sizeof (Elf32_Addr);
}


/* The first entry in an absolute procedure linkage table looks like
   this.  See the SVR4 ABI i386 supplement to see how this works.  */
static const unsigned char elf_i386_plt0_entry[PLT_ENTRY_SIZE] =
{
  0xff, 0x35,	/* pushl contents of address */
  0, 0, 0, 0,	/* replaced with address of .got + 4.  */
  0xff, 0x25,	/* jmp indirect */
  0, 0, 0, 0,	/* replaced with address of .got + 8.  */
  0x0f, 0x0b,	/* ud2a, to prevent further decoding.  */
  0, 0		/* pad out to 16 bytes.  */
};

/* Type describing the first PLT entry in non-PIC.  */
struct plt0_entry
{
  /* First a 'push' of the second GOT entry.  */
  unsigned char push_instr[2];
  uint32_t gotp4_addr;
  /* Second, a 'jmp indirect' to the third GOT entry.  */
  unsigned char jmp_instr[2];
  uint32_t gotp8_addr;
  /* Padding.  */
  unsigned char padding[4];
} __attribute__ ((packed));

/* The first entry in a PIC procedure linkage table look like this.  */
static const unsigned char elf_i386_pic_plt0_entry[PLT_ENTRY_SIZE] =
{
  0xff, 0xb3, 4, 0, 0, 0,	/* pushl 4(%ebx) */
  0xff, 0xa3, 8, 0, 0, 0,	/* jmp *8(%ebx) */
  0x0f, 0x0b,			/* ud2a, to prevent further decoding.  */
  0, 0				/* pad out to 16 bytes.  */
};

/* Contents of all but the first PLT entry in executable.  */
static const unsigned char elf_i386_plt_entry[PLT_ENTRY_SIZE] =
{
  0xff, 0x25,   /* jmp indirect */
  0, 0, 0, 0,   /* replaced with address of this symbol in .got.  */
  0x68,         /* pushl immediate */
  0, 0, 0, 0,   /* replaced with offset into relocation table.  */
  0xe9,         /* jmp relative */
  0, 0, 0, 0    /* replaced with offset to start of .plt.  */
};

/* Contents of all but the first PLT entry in DSOs.  */
static const unsigned char elf_i386_pic_plt_entry[PLT_ENTRY_SIZE] =
{
  0xff, 0xa3,	/* jmp *offset(%ebx) */
  0, 0, 0, 0,	/* replaced with offset of this symbol in .got.  */
  0x68,		/* pushl immediate */
  0, 0, 0, 0,	/* replaced with offset into relocation table.  */
  0xe9,		/* jmp relative */
  0, 0, 0, 0	/* replaced with offset to start of .plt.  */
};

/* Type describing a PLT entry.  */
struct plt_entry
{
  /* The first instruction is 'jmp indirect' or 'jmp *offset(%ebs)'.  */
  unsigned char jmp_instr[2];
  uint32_t offset_got;
  /* The second instruction is 'push immediate'.  */
  unsigned char push_instr;
  uint32_t push_imm;
  /* Finally a 'jmp relative'.  */
  unsigned char jmp_instr2;
  uint32_t plt0_offset;
} __attribute__ ((packed));


static void
elf_i386_finalize_plt (struct ld_state *statep, size_t nsym,
		       size_t nsym_local, struct symbol **ndxtosym)
{
  if (unlikely (statep->nplt + statep->ngot == 0))
    /* Nothing to be done.  */
    return;

  Elf_Scn *scn;
  XElf_Shdr_vardef (shdr);
  Elf_Data *data;
  const bool build_dso = statep->file_type == dso_file_type;

  /* Get the address of the .got.plt section.  */
  scn = elf_getscn (statep->outelf, statep->gotpltscnidx);
  xelf_getshdr (scn, shdr);
  data = elf_getdata (scn, NULL);
  assert (shdr != NULL && data != NULL);
  /* The address points to the .got.plt section, not the .got section.  */
  Elf32_Addr gotaddr = shdr->sh_addr;

  /* Now create the initial values for the .got.plt section.  The
     first word contains the address of the .dynamic section.  The
     second and third entry are left empty for use by the dynamic
     linker.  The following entries are pointers to the instructions
     following the initial jmp instruction in the corresponding PLT
     entry.  */
  xelf_getshdr (elf_getscn (statep->outelf, statep->dynamicscnidx), shdr);
  assert (shdr != NULL);
  ((Elf32_Word *) data->d_buf)[0] = shdr->sh_addr;

  /* The PLT contains code which a user of a function jumps to.  The first
     PLT entry is special, so the first used one has the index 1.  */
  scn = elf_getscn (statep->outelf, statep->pltscnidx);
  XElf_Shdr_vardef (pltshdr);
  xelf_getshdr (scn, pltshdr);
  assert (pltshdr != NULL);

  Elf_Data *dynsymdata = elf_getdata (elf_getscn (statep->outelf,
						  statep->dynsymscnidx), NULL);
  assert (dynsymdata != NULL);

  Elf_Data *symdata = NULL;
  if (statep->symscnidx != 0)
    {
      symdata = elf_getdata (elf_getscn (statep->outelf, statep->symscnidx),
			     NULL);
      assert (symdata != NULL);
    }

  /* Create the .plt section.  */
  scn = elf_getscn (statep->outelf, statep->pltscnidx);
  Elf_Data *pltdata = elf_getdata (scn, NULL);
  assert (pltdata != NULL);

  /* Also create the .rel.plt section data.  It simply means relocations
     addressing the corresponding entry in the .got.plt section.  The
     section name is misleading.  */
  scn = elf_getscn (statep->outelf, statep->pltrelscnidx);
  xelf_getshdr (scn, shdr);
  Elf_Data *reldata = elf_getdata (scn, NULL);
  assert (shdr != NULL && reldata != NULL);

  /* Update the sh_link to point to the section being modified.  We
     point it here (correctly) to the .got.plt section.  Some linkers
     (e.g., the GNU binutils linker) point to the .plt section.  This
     is wrong since the .plt section isn't modified even though the
     name .rel.plt suggests that this is correct.  */
  shdr->sh_link = statep->dynsymscnidx;
  shdr->sh_info = statep->gotpltscnidx;
  (void) xelf_update_shdr (scn, shdr);

  /* Create the first entry of the .plt section.  */
  assert (pltdata->d_size >= PLT_ENTRY_SIZE);
  if (build_dso)
    /* Copy the entry.  It's complete, no relocation needed.  */
    memcpy (pltdata->d_buf, elf_i386_pic_plt0_entry, PLT_ENTRY_SIZE);
  else
    {
      /* Copy the skeleton.  */
      memcpy (pltdata->d_buf, elf_i386_plt0_entry, PLT_ENTRY_SIZE);

      /* And fill in the addresses.  */
      struct plt0_entry *addr = (struct plt0_entry *) pltdata->d_buf;
      addr->gotp4_addr = target_bswap_32 (gotaddr + 4);
      addr->gotp8_addr = target_bswap_32 (gotaddr + 8);
    }

  /* For DSOs we need GOT offsets, otherwise the GOT address.  */
  Elf32_Addr gotaddr_off = build_dso ? 0 : gotaddr;

  /* Create the remaining entries.  */
  const unsigned char *plt_template
    = build_dso ? elf_i386_pic_plt_entry : elf_i386_plt_entry;

  for (size_t idx = nsym_local; idx < nsym; ++idx)
    {
      struct symbol *symbol = ndxtosym[idx];
      if (symbol == NULL || symbol->type != STT_FUNC
	  || ndxtosym[idx]->outdynsymidx == 0
	  // XXX is the following test correct?
	  || ! ndxtosym[idx]->in_dso)
	continue;

      size_t pltidx = symbol->merge.value;

      assert (pltidx > 0);
      assert ((3 + pltidx) * sizeof (Elf32_Word) <= data->d_size);

      /* Address in the PLT.  */
      Elf32_Addr pltentryaddr = (pltshdr->sh_addr + pltidx * PLT_ENTRY_SIZE);

      /* Point the GOT entry at the PLT entry, after the initial jmp.  */
      ((Elf32_Word *) data->d_buf)[2 + pltidx] = pltentryaddr + 6;

      /* If the symbol is defined, adjust the address.  */
      if (((Elf32_Sym *) dynsymdata->d_buf)[ndxtosym[idx]->outdynsymidx].st_shndx != SHN_UNDEF)
	{
	  /* The value of the symbol is the address of the corresponding PLT
	     entry.  Store the address, also for the normal symbol table if
	     this is necessary.  */
	  ((Elf32_Sym *) dynsymdata->d_buf)[pltidx].st_value = pltentryaddr;

	  if (symdata != NULL)
 {
   assert(nsym - statep->nplt + (pltidx - 1) == idx);
	    ((Elf32_Sym *) symdata->d_buf)[nsym - statep->nplt
					   + (pltidx - 1)].st_value
	      = pltentryaddr;
 }
	}

      /* Copy the PLT entry template.  */
      assert (pltdata->d_size >= (1 + pltidx) * PLT_ENTRY_SIZE);
      struct plt_entry *addr = (struct plt_entry *) ((char *) pltdata->d_buf
						     + (pltidx
							* PLT_ENTRY_SIZE));
      memcpy (addr, plt_template, PLT_ENTRY_SIZE);

      /* And once more, fill in the addresses.  First the address of
	 this symbol in .got.  */
      addr->offset_got = target_bswap_32 (gotaddr_off
					  + (2 + pltidx) * sizeof (Elf32_Addr));
      /* Offset into relocation table.  */
      addr->push_imm = target_bswap_32 ((pltidx - 1) * sizeof (Elf32_Rel));
      /* Offset to start of .plt.  */
      addr->plt0_offset = target_bswap_32 (-(1 + pltidx) * PLT_ENTRY_SIZE);


      XElf_Rel_vardef (rel);
      assert (pltidx * sizeof (Elf32_Rel) <= reldata->d_size);
      xelf_getrel_ptr (reldata, pltidx - 1, rel);
      rel->r_offset = gotaddr + (2 + pltidx) * sizeof (Elf32_Addr);
      /* The symbol table entries for the functions from DSOs are at
	 the beginning of the symbol table.  */
      rel->r_info = XELF_R_INFO (ndxtosym[idx]->outdynsymidx, R_386_JMP_SLOT);
      (void) xelf_update_rel (reldata, pltidx - 1, rel);
    }
}


static int
elf_i386_rel_type (struct ld_state *statep __attribute__ ((__unused__)))
{
  /* ELF/i386 uses REL.  */
  return DT_REL;
}


static void
elf_i386_count_relocations (struct ld_state *statep, struct scninfo *scninfo)
{
  /* We go through the list of input sections and count those relocations
     which are not handled by the linker.  At the same time we have to
     see how many GOT entries we need and how much .bss space is needed
     for copy relocations.  */
  Elf_Data *data = elf_getdata (scninfo->scn, NULL);
  XElf_Shdr *shdr = &SCNINFO_SHDR (scninfo->shdr);
  size_t maxcnt = shdr->sh_size / shdr->sh_entsize;
  size_t relsize = 0;
  size_t cnt;
  struct symbol *sym;

  assert (shdr->sh_type == SHT_REL);

  for (cnt = 0; cnt < maxcnt; ++cnt)
    {
      XElf_Rel_vardef (rel);

      xelf_getrel (data, cnt, rel);
      /* XXX Should we complain about failing accesses?  */
      if (rel != NULL)
	{
	  Elf32_Word r_sym = XELF_R_SYM (rel->r_info);

	  /* Symbols in COMDAT group sections which are discarded do
	     not have to be relocated.  */
	  if (r_sym >= scninfo->fileinfo->nlocalsymbols
	      && unlikely (scninfo->fileinfo->symref[r_sym] == NULL))
	    continue;

	  switch (XELF_R_TYPE (rel->r_info))
	    {
	    case R_386_GOT32:
	      if (! scninfo->fileinfo->symref[r_sym]->defined
		  || scninfo->fileinfo->symref[r_sym]->in_dso
		  || statep->file_type == dso_file_type)
		{
		  relsize += sizeof (Elf32_Rel);
		  ++statep->nrel_got;
		}

	      /* Even if this relocation is not emitted in the output
		 file it requires a GOT entry.  */
	      ++statep->ngot;

	      /* FALLTHROUGH */

	    case R_386_GOTOFF:
	    case R_386_GOTPC:
	      statep->need_got = true;
	      break;

	    case R_386_32:
	    case R_386_PC32:
	      /* These relocations cause text relocations in DSOs.  */
	      if (linked_from_dso_p (scninfo, r_sym))
		{
		  if (statep->file_type == dso_file_type)
		    {
		      relsize += sizeof (Elf32_Rel);
		      // XXX Do we have to check whether the target
		      // XXX section is read-only first?
		      statep->dt_flags |= DF_TEXTREL;
		    }
		  else
		    {
		      /* Non-function objects from a DSO need to get a
			 copy relocation.  */
		      sym = scninfo->fileinfo->symref[r_sym];

		      /* Only do this if we have not requested a copy
			 relocation already.  */
		      if (unlikely (sym->type != STT_FUNC) && ! sym->need_copy)
			{
			  sym->need_copy = 1;
			  ++statep->ncopy;
			  relsize += sizeof (Elf32_Rel);
			}
		    }
		}
	      else if (statep->file_type == dso_file_type
		       && XELF_R_TYPE (rel->r_info) == R_386_32)
		relsize += sizeof (Elf32_Rel);

	      break;

	    case R_386_PLT32:
	      /* We might need a PLT entry.  But we cannot say for sure
		 here since one of the symbols might turn up being
		 defined in the executable (if we create such a thing).
		 If a DSO is created we still might use a local
		 definition.

		 If the symbol is not defined and we are not creating
		 a statically linked binary, then we need in any case
		 a PLT entry.  */
	      if (! scninfo->fileinfo->symref[r_sym]->defined
		  && !statep->statically)
		{
		  sym = scninfo->fileinfo->symref[r_sym];
		  sym->type = STT_FUNC;
		  sym->in_dso = 1;
		  sym->defined = 1;

		  /* Remove from the list of unresolved symbols.  */
		  --statep->nunresolved;
		  if (! sym->weak)
		    --statep->nunresolved_nonweak;
		  CDBL_LIST_DEL (statep->unresolved, sym);

		  /* Add to the list of symbols we expect from a DSO.  */
		  ++statep->nplt;
		  ++statep->nfrom_dso;
		  CDBL_LIST_ADD_REAR (statep->from_dso, sym);
		}
	      break;

	    case R_386_TLS_LDO_32:
	      if (statep->file_type != executable_file_type)
		abort ();
	      /* We do not need a relocation in the output file.  */
	      break;

	    case R_386_TLS_LE:
	      /* We never need a relocation in the output file.  */
	      break;

	    case R_386_TLS_IE:
	      if (statep->file_type == dso_file_type)
		error (EXIT_FAILURE, 0, gettext ("initial-executable TLS relocation cannot be used "));
	      if (!scninfo->fileinfo->symref[r_sym]->defined
		  || scninfo->fileinfo->symref[r_sym]->in_dso)
		{
		  abort ();
		}
	      break;

	    case R_386_TLS_GD:
	      if (statep->file_type != executable_file_type
		  || !scninfo->fileinfo->symref[r_sym]->defined
		  || scninfo->fileinfo->symref[r_sym]->in_dso)
		{
		  abort ();
		}
	      break;

	    case R_386_TLS_GOTIE:
	    case R_386_TLS_LDM:
	    case R_386_TLS_GD_32:
	    case R_386_TLS_GD_PUSH:
	    case R_386_TLS_GD_CALL:
	    case R_386_TLS_GD_POP:
	    case R_386_TLS_LDM_32:
	    case R_386_TLS_LDM_PUSH:
	    case R_386_TLS_LDM_CALL:
	    case R_386_TLS_LDM_POP:
	    case R_386_TLS_IE_32:
	    case R_386_TLS_LE_32:
	      /* XXX */
	      abort ();
	      break;

	    case R_386_NONE:
	      /* Nothing to be done.  */
	      break;

	      /* These relocation should never be generated by an
		 assembler.  */
	    case R_386_COPY:
	    case R_386_GLOB_DAT:
	    case R_386_JMP_SLOT:
	    case R_386_RELATIVE:
	    case R_386_TLS_DTPMOD32:
	    case R_386_TLS_DTPOFF32:
	    case R_386_TLS_TPOFF32:
	      /* Unknown relocation.  */
	    default:
	      abort ();
	    }
	}
    }

  scninfo->relsize = relsize;
}


static void
elf_i386_create_relocations (struct ld_state *statep,
			     const Elf32_Word *dblindirect __attribute__ ((unused)))
{
  /* Get the address of the got section.  */
  Elf_Scn *pltscn = elf_getscn (statep->outelf, statep->pltscnidx);
  Elf32_Shdr *shdr = elf32_getshdr (pltscn);
  assert (shdr != NULL);
  Elf32_Addr pltaddr = shdr->sh_addr;

  Elf_Scn *gotscn = elf_getscn (statep->outelf, statep->gotscnidx);
  // XXX Adjust the address, if necessary, for relro
  Elf_Data *gotdata = NULL;
  if (statep->need_got)
    {
      gotdata = elf_getdata (gotscn, NULL);
      assert (gotdata != NULL);
    }

  Elf_Scn *gotpltscn = elf_getscn (statep->outelf, statep->gotpltscnidx);
  shdr = elf32_getshdr (gotpltscn);
  assert (shdr != NULL);
  Elf32_Addr gotaddr = shdr->sh_addr;

  Elf_Scn *reldynscn = elf_getscn (statep->outelf, statep->reldynscnidx);
  Elf_Data *reldyndata = elf_getdata (reldynscn, NULL);
  assert (reldyndata != NULL);

  size_t nreldyn = 0;
  size_t ngotconst = statep->nrel_got;

  struct scninfo *first = statep->rellist->next;
  struct scninfo *runp = first;
  do
    {
      XElf_Shdr *rshdr = &SCNINFO_SHDR (runp->shdr);
      Elf_Data *reldata = elf_getdata (runp->scn, NULL);
      int nrels = rshdr->sh_size / rshdr->sh_entsize;

      /* We will need the following values a couple of times.  Help
	 the compiler and improve readability.  */
      struct symbol **symref = runp->fileinfo->symref;
      struct scninfo *scninfo = runp->fileinfo->scninfo;

      /* This is the offset of the input section we are looking at in
	 the output file.  */
      XElf_Addr inscnoffset = scninfo[rshdr->sh_info].offset;

      /* The target section.  We use the data from the input file.  */
      Elf_Data *data = elf_getdata (scninfo[rshdr->sh_info].scn, NULL);

      /* We cannot handle relocations against merge-able sections.  */
      assert ((SCNINFO_SHDR (scninfo[rshdr->sh_link].shdr).sh_flags
	       & SHF_MERGE) == 0);

      /* Cache the access to the symbol table data.  */
      Elf_Data *symdata = elf_getdata (scninfo[rshdr->sh_link].scn, NULL);

      for (int cnt = 0; cnt < nrels; ++cnt)
	{
	  XElf_Rel_vardef (rel);
	  XElf_Rel *rel2;
	  xelf_getrel (reldata, cnt, rel);
	  assert (rel != NULL);
	  XElf_Addr reladdr = inscnoffset + rel->r_offset;
	  XElf_Addr value;

	  size_t idx = XELF_R_SYM (rel->r_info);
	  if (idx < runp->fileinfo->nlocalsymbols)
	    {
	      XElf_Sym_vardef (sym);
	      xelf_getsym (symdata, idx, sym);

	      /* The value only depends on the position of the referenced
		 section in the output file and the addend.  */
	      value = scninfo[sym->st_shndx].offset + sym->st_value;
	    }
	  else
	    {
	      if (symref[idx] == NULL)
		/* Symbol in ignored COMDAT group section.  */
		continue;

	      value = symref[idx]->merge.value;
	      if (symref[idx]->in_dso)
		{
		  /* MERGE.VALUE contains the PLT index.  If this is not for
		     a function the actual value will be computed later.  */
		  assert (value != 0 || symref[idx]->type != STT_FUNC);
		  value = pltaddr + value * PLT_ENTRY_SIZE;
		}
	    }

	  /* Address of the relocated memory in the data buffer.  */
	  unsigned char *relloc = (unsigned char *) data->d_buf + rel->r_offset;

	  uint32_t thisgotidx;
	  switch (XELF_R_TYPE (rel->r_info))
	    {
	      /* These three cases can be handled together since the
		 symbol associated with the R_386_GOTPC relocation is
		 _GLOBAL_OFFSET_TABLE_ which has a value corresponding
		 to the address of the GOT and the address of the PLT
		 entry required for R_386_PLT32 is computed above.  */
	    case R_386_PC32:
	    case R_386_GOTPC:
	    case R_386_PLT32:
	      value -= reladdr;
	      /* FALLTHROUGH */

	    case R_386_32:
	      if (linked_from_dso_p (scninfo, idx)
		  && statep->file_type != dso_file_type
		  && symref[idx]->type != STT_FUNC)
		{
		  value = (ld_state.copy_section->offset
			   + symref[idx]->merge.value);

		  if (unlikely (symref[idx]->need_copy))
		    {
		      /* Add a relocation to initialize the GOT entry.  */
		      assert (symref[idx]->outdynsymidx != 0);
#if NATIVE_ELF != 0
		      xelf_getrel_ptr (reldyndata, nreldyn, rel2);
#else
		      rel2 = &rel_mem;
#endif
		      rel2->r_offset = value;
		      rel2->r_info
			= XELF_R_INFO (symref[idx]->outdynsymidx, R_386_COPY);
		      (void) xelf_update_rel (reldyndata, nreldyn, rel2);
		      ++nreldyn;
		      assert (nreldyn <= statep->nrel_got);

		      /* Update the symbol table record for the new
			 address.  */
		      Elf32_Word symidx = symref[idx]->outdynsymidx;
		      Elf_Scn *symscn = elf_getscn (statep->outelf,
						    statep->dynsymscnidx);
		      Elf_Data *outsymdata = elf_getdata (symscn, NULL);
		      assert (outsymdata != NULL);
		      XElf_Sym_vardef (sym);
		      xelf_getsym (outsymdata, symidx, sym);
		      sym->st_value = value;
		      sym->st_shndx = statep->copy_section->outscnndx;
		      (void) xelf_update_sym (outsymdata, symidx, sym);

		      symidx = symref[idx]->outsymidx;
		      if (symidx != 0)
			{
			  symidx = statep->dblindirect[symidx];
			  symscn = elf_getscn (statep->outelf,
					       statep->symscnidx);
			  outsymdata = elf_getdata (symscn, NULL);
			  assert (outsymdata != NULL);
			  xelf_getsym (outsymdata, symidx, sym);
			  sym->st_value = value;
			  sym->st_shndx = statep->copy_section->outscnndx;
			  (void) xelf_update_sym (outsymdata, symidx, sym);
			}

		      /* Remember that we set up the copy relocation.  */
		      symref[idx]->need_copy = 0;
		    }
		}
	      else if (statep->file_type == dso_file_type
		       && XELF_R_TYPE (rel->r_info) == R_386_32)
		{
#if NATIVE_ELF != 0
		  xelf_getrel_ptr (reldyndata, nreldyn, rel2);
#else
		  rel2 = &rel_mem;
#endif
		  rel2->r_offset = value;

		  /* For symbols we do not export we generate a relative
		     relocation.  */
		  if (idx < SCNINFO_SHDR (scninfo[rshdr->sh_link].shdr).sh_info
		      || symref[idx]->outdynsymidx == 0)
		    rel2->r_info = XELF_R_INFO (0, R_386_RELATIVE);
		  else
		    rel2->r_info
		      = XELF_R_INFO (symref[idx]->outdynsymidx, R_386_32);
		  (void) xelf_update_rel (reldyndata, nreldyn, rel2);
		  ++nreldyn;
		  assert (nreldyn <= statep->nrel_got);

		  value = 0;
		}
	      add_4ubyte_unaligned (relloc, value);
	      break;

	    case R_386_GOT32:
	      if (! symref[idx]->defined || symref[idx]->in_dso)
		{
		  thisgotidx = nreldyn++;
		  assert (thisgotidx < statep->nrel_got);

		  /* Add a relocation to initialize the GOT entry.  */
#if NATIVE_ELF != 0
		  xelf_getrel_ptr (reldyndata, thisgotidx, rel2);
#else
		  rel2 = &rel_mem;
#endif
		  rel2->r_offset = gotaddr + ((thisgotidx - statep->ngot)
					      * sizeof (Elf32_Addr));
		  rel2->r_info
		    = XELF_R_INFO (symref[idx]->outdynsymidx, R_386_GLOB_DAT);
		  (void) xelf_update_rel (reldyndata, thisgotidx, rel2);
		}
	      else if (statep->file_type != dso_file_type)
		{
		  thisgotidx = ngotconst++;
		  assert (thisgotidx < statep->ngot);

		  /* We have to use a GOT since the generated code
		     requires it but we know the address and therefore
		     do not need a relocation.  */
		  ((uint32_t *) gotdata->d_buf)[thisgotidx] = value;
		}
	      else
		{
		  thisgotidx = nreldyn++;
		  assert (thisgotidx < statep->nrel_got);

		  // XXX generate a relative relocation.
		  abort ();
		}

	      store_4ubyte_unaligned (relloc,
				      (thisgotidx - statep->ngot)
				      * sizeof (Elf32_Addr));
	      break;

	    case R_386_GOTOFF:
	      add_4ubyte_unaligned (relloc, value - gotaddr);
	      break;

	    case R_386_TLS_LE:
	      value = symref[idx]->merge.value - ld_state.tls_tcb;
	      store_4ubyte_unaligned (relloc, value);
	      break;

	    case R_386_TLS_IE:
	      if (symref[idx]->defined && !symref[idx]->in_dso)
		{
		  /* The symbol is defined in the executable.
		     Perform the IE->LE optimization.
		     There are multiple versions, though.

		     First version: mov ADDR,REG.  */
		  if (relloc[-2] == 0x8b
		      && ((relloc[-1] & 0xc7) == 0x05))
		    {
		      relloc[-2] = 0xc7;
		      relloc[-1] = 0xc0 | ((relloc[-1] >> 3) & 7);
		      store_4ubyte_unaligned (relloc, (symref[idx]->merge.value
						       - ld_state.tls_tcb));
		    }
		  else
		    {
		      abort ();
		    }
		}
	      else
		{
		  abort ();
		}
	      break;

	    case R_386_TLS_LDO_32:
	      value = symref[idx]->merge.value - ld_state.tls_start;
	      store_4ubyte_unaligned (relloc, value);
	      break;

	    case R_386_TLS_GD:
	      if (ld_state.file_type == executable_file_type)
		{
		  if (symref[idx]->defined && !symref[idx]->in_dso)
		    {
		      /* The symbol is defined in the executable.
			 Perform the GD->LE optimization.  */
		      static const char gd_to_le[] =
			{
			  /* mov %gs:0x0,%eax */
			  0x65, 0xa1, 0x00, 0x00, 0x00, 0x00,
			  /* sub $OFFSET,%eax */
			  0x81, 0xe8
			};
#ifndef NDEBUG
		      static const char gd_text[] =
			{
			  /* lea 0x0(,%ebx,1),%eax */
			  0x8d, 0x04, 0x1d, 0x00, 0x00, 0x00, 0x00,
			  /* call ___tls_get_addr */
			  0xe8
			};
		      assert (memcmp (relloc - 3, gd_text, sizeof (gd_text))
			      == 0);
#endif
		      relloc = mempcpy (relloc - 3, gd_to_le,
					sizeof (gd_to_le));
		      value = ld_state.tls_tcb- symref[idx]->merge.value;
		      store_4ubyte_unaligned (relloc, value);

		      /* We have to skip over the next relocation which is
			 the matching R_i386_PLT32 for __tls_get_addr.  */
		      ++cnt;
#ifndef NDEBUG
		      assert (cnt < nrels);
		      XElf_Off old_offset = rel->r_offset;
		      xelf_getrel (reldata, cnt, rel);
		      assert (rel != NULL);
		      assert (XELF_R_TYPE (rel->r_info) == R_386_PLT32);
		      idx = XELF_R_SYM (rel->r_info);
		      assert (strcmp (symref[idx]->name, "___tls_get_addr")
			      == 0);
		      assert (old_offset + 5 == rel->r_offset);
#endif

		      break;
		    }
		}
	      abort ();
	      break;

	    case R_386_32PLT:
	    case R_386_TLS_TPOFF:
	    case R_386_TLS_GOTIE:
	    case R_386_TLS_LDM:
	    case R_386_16:
	    case R_386_PC16:
	    case R_386_8:
	    case R_386_PC8:
	    case R_386_TLS_GD_32:
	    case R_386_TLS_GD_PUSH:
	    case R_386_TLS_GD_CALL:
	    case R_386_TLS_GD_POP:
	    case R_386_TLS_LDM_32:
	    case R_386_TLS_LDM_PUSH:
	    case R_386_TLS_LDM_CALL:
	    case R_386_TLS_LDM_POP:
	    case R_386_TLS_IE_32:
	    case R_386_TLS_LE_32:
	      // XXX For now fall through
	      break;

	    case R_386_NONE:
	      /* Nothing to do.  */
	      break;

	    case R_386_COPY:
	    case R_386_JMP_SLOT:
	    case R_386_RELATIVE:
	    case R_386_GLOB_DAT:
	    case R_386_TLS_DTPMOD32:
	    case R_386_TLS_DTPOFF32:
	    case R_386_TLS_TPOFF32:
	    default:
	      /* Should not happen.  */
	      abort ();
	    }
	}
    }
  while ((runp = runp->next) != first);
}


int
elf_i386_ld_init (struct ld_state *statep)
{
  /* We have a few callbacks available.  */
  old_open_outfile = statep->callbacks.open_outfile;
  statep->callbacks.open_outfile = elf_i386_open_outfile;

  statep->callbacks.relocate_section  = elf_i386_relocate_section;

  statep->callbacks.initialize_plt = elf_i386_initialize_plt;
  statep->callbacks.initialize_pltrel = elf_i386_initialize_pltrel;

  statep->callbacks.initialize_got = elf_i386_initialize_got;
  statep->callbacks.initialize_gotplt = elf_i386_initialize_gotplt;

  statep->callbacks.finalize_plt = elf_i386_finalize_plt;

  statep->callbacks.rel_type = elf_i386_rel_type;

  statep->callbacks.count_relocations = elf_i386_count_relocations;

  statep->callbacks.create_relocations = elf_i386_create_relocations;

  return 0;
}
