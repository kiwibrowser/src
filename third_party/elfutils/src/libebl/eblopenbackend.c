/* Generate ELF backend handle.
   Copyright (C) 2000-2013 Red Hat, Inc.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <dlfcn.h>
#include <error.h>
#include <libelfP.h>
#include <dwarf.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libeblP.h>


/* This table should contain the complete list of architectures as far
   as the ELF specification is concerned.  */
/* XXX When things are stable replace the string pointers with char
   arrays to avoid relocations.  */
static const struct
{
  const char *dsoname;
  const char *emulation;
  const char *prefix;
  int prefix_len;
  int em;
  int class;
  int data;
} machines[] =
{
  { "i386", "elf_i386", "i386", 4, EM_386, ELFCLASS32, ELFDATA2LSB },
  { "ia64", "elf_ia64", "ia64", 4, EM_IA_64, ELFCLASS64, ELFDATA2LSB },
  { "alpha", "elf_alpha", "alpha", 5, EM_ALPHA, ELFCLASS64, ELFDATA2LSB },
  { "x86_64", "elf_x86_64", "x86_64", 6, EM_X86_64, ELFCLASS64, ELFDATA2LSB },
  { "ppc", "elf_ppc", "ppc", 3, EM_PPC, ELFCLASS32, ELFDATA2MSB },
  { "ppc64", "elf_ppc64", "ppc64", 5, EM_PPC64, ELFCLASS64, ELFDATA2MSB },
  { "tilegx", "elf_tilegx", "tilegx", 6, EM_TILEGX, ELFCLASS64, ELFDATA2LSB },
  // XXX class and machine fields need to be filled in for all archs.
  { "sh", "elf_sh", "sh", 2, EM_SH, 0, 0 },
  { "arm", "ebl_arm", "arm", 3, EM_ARM, 0, 0 },
  { "sparc", "elf_sparcv9", "sparc", 5, EM_SPARCV9, 0, 0 },
  { "sparc", "elf_sparc", "sparc", 5, EM_SPARC, 0, 0 },
  { "sparc", "elf_sparcv8plus", "sparc", 5, EM_SPARC32PLUS, 0, 0 },
  { "s390", "ebl_s390", "s390", 4, EM_S390, 0, 0 },

  { "m32", "elf_m32", "m32", 3, EM_M32, 0, 0 },
  { "m68k", "elf_m68k", "m68k", 4, EM_68K, 0, 0 },
  { "m88k", "elf_m88k", "m88k", 4, EM_88K, 0, 0 },
  { "i860", "elf_i860", "i860", 4, EM_860, 0, 0 },
  { "s370", "ebl_s370", "s370", 4, EM_S370, 0, 0 },
  { "parisc", "elf_parisc", "parisc", 6, EM_PARISC, 0, 0 },
  { "vpp500", "elf_vpp500", "vpp500", 5, EM_VPP500, 0, 0 },
  { "sparc", "elf_v8plus", "v8plus", 6, EM_SPARC32PLUS, 0, 0 },
  { "i960", "elf_i960", "i960", 4, EM_960, 0, 0 },
  { "v800", "ebl_v800", "v800", 4, EM_V800, 0, 0 },
  { "fr20", "ebl_fr20", "fr20", 4, EM_FR20, 0, 0 },
  { "rh32", "ebl_rh32", "rh32", 4, EM_RH32, 0, 0 },
  { "rce", "ebl_rce", "rce", 3, EM_RCE, 0, 0 },
  { "tricore", "elf_tricore", "tricore", 7, EM_TRICORE, 0, 0 },
  { "arc", "elf_arc", "arc", 3, EM_ARC, 0, 0 },
  { "h8", "elf_h8_300", "h8_300", 6, EM_H8_300, 0, 0 },
  { "h8", "elf_h8_300h", "h8_300h", 6, EM_H8_300H, 0, 0 },
  { "h8", "elf_h8s", "h8s", 6, EM_H8S, 0, 0 },
  { "h8", "elf_h8_500", "h8_500", 6, EM_H8_500, 0, 0 },
  { "coldfire", "elf_coldfire", "coldfire", 8, EM_COLDFIRE, 0, 0 },
  { "m68k", "elf_68hc12", "68hc12", 6, EM_68HC12, 0, 0 },
  { "mma", "elf_mma", "mma", 3, EM_MMA, 0, 0 },
  { "pcp", "elf_pcp", "pcp", 3, EM_PCP, 0, 0 },
  { "ncpu", "elf_ncpu", "ncpu", 4, EM_NCPU, 0, 0 },
  { "ndr1", "elf_ndr1", "ndr1", 4, EM_NDR1, 0, 0 },
  { "starcore", "elf_starcore", "starcore", 8, EM_STARCORE, 0, 0 },
  { "me16", "elf_me16", "em16", 4, EM_ME16, 0, 0 },
  { "st100", "elf_st100", "st100", 5, EM_ST100, 0, 0 },
  { "tinyj", "elf_tinyj", "tinyj", 5, EM_TINYJ, 0, 0 },
  { "pdsp", "elf_pdsp", "pdsp", 4, EM_PDSP, 0, 0 },
  { "fx66", "elf_fx66", "fx66", 4, EM_FX66, 0, 0 },
  { "st9plus", "elf_st9plus", "st9plus", 7, EM_ST9PLUS, 0, 0 },
  { "st7", "elf_st7", "st7", 3, EM_ST7, 0, 0 },
  { "m68k", "elf_68hc16", "68hc16", 6, EM_68HC16, 0, 0 },
  { "m68k", "elf_68hc11", "68hc11", 6, EM_68HC11, 0, 0 },
  { "m68k", "elf_68hc08", "68hc08", 6, EM_68HC08, 0, 0 },
  { "m68k", "elf_68hc05", "68hc05", 6, EM_68HC05, 0, 0 },
  { "svx", "elf_svx", "svx", 3, EM_SVX, 0, 0 },
  { "st19", "elf_st19", "st19", 4, EM_ST19, 0, 0 },
  { "vax", "elf_vax", "vax", 3, EM_VAX, 0, 0 },
  { "cris", "elf_cris", "cris", 4, EM_CRIS, 0, 0 },
  { "javelin", "elf_javelin", "javelin", 7, EM_JAVELIN, 0, 0 },
  { "firepath", "elf_firepath", "firepath", 8, EM_FIREPATH, 0, 0 },
  { "zsp", "elf_zsp", "zsp", 3, EM_ZSP, 0, 0 },
  { "mmix", "elf_mmix", "mmix", 4, EM_MMIX, 0, 0 },
  { "hunay", "elf_huany", "huany", 5, EM_HUANY, 0, 0 },
  { "prism", "elf_prism", "prism", 5, EM_PRISM, 0, 0 },
  { "avr", "elf_avr", "avr", 3, EM_AVR, 0, 0 },
  { "fr30", "elf_fr30", "fr30", 4, EM_FR30, 0, 0 },
  { "dv10", "elf_dv10", "dv10", 4, EM_D10V, 0, 0 },
  { "dv30", "elf_dv30", "dv30", 4, EM_D30V, 0, 0 },
  { "v850", "elf_v850", "v850", 4, EM_V850, 0, 0 },
  { "m32r", "elf_m32r", "m32r", 4, EM_M32R, 0, 0 },
  { "mn10300", "elf_mn10300", "mn10300", 7, EM_MN10300, 0, 0 },
  { "mn10200", "elf_mn10200", "mn10200", 7, EM_MN10200, 0, 0 },
  { "pj", "elf_pj", "pj", 2, EM_PJ, 0, 0 },
  { "openrisc", "elf_openrisc", "openrisc", 8, EM_OPENRISC, 0, 0 },
  { "arc", "elf_arc_a5", "arc_a5", 6, EM_ARC_A5, 0, 0 },
  { "xtensa", "elf_xtensa", "xtensa", 6, EM_XTENSA, 0, 0 },
  { "aarch64", "elf_aarch64", "aarch64", 7, EM_AARCH64, ELFCLASS64, 0 },
};
#define nmachines (sizeof (machines) / sizeof (machines[0]))


/* Default callbacks.  Mostly they just return the error value.  */
static const char *default_object_type_name (int ignore, char *buf,
					     size_t len);
static const char *default_reloc_type_name (int ignore, char *buf, size_t len);
static bool default_reloc_type_check (int ignore);
static bool default_reloc_valid_use (Elf *elf, int ignore);
static Elf_Type default_reloc_simple_type (Ebl *ebl, int ignore);
static bool default_gotpc_reloc_check (Elf *elf, int ignore);
static const char *default_segment_type_name (int ignore, char *buf,
					      size_t len);
static const char *default_section_type_name (int ignore, char *buf,
					      size_t len);
static const char *default_section_name (int ignore, int ignore2, char *buf,
					 size_t len);
static const char *default_machine_flag_name (Elf64_Word *ignore);
static bool default_machine_flag_check (Elf64_Word flags);
static bool default_machine_section_flag_check (GElf_Xword flags);
static const char *default_symbol_type_name (int ignore, char *buf,
					     size_t len);
static const char *default_symbol_binding_name (int ignore, char *buf,
						size_t len);
static const char *default_dynamic_tag_name (int64_t ignore, char *buf,
					     size_t len);
static bool default_dynamic_tag_check (int64_t ignore);
static GElf_Word default_sh_flags_combine (GElf_Word flags1, GElf_Word flags2);
static const char *default_osabi_name (int ignore, char *buf, size_t len);
static void default_destr (struct ebl *ignore);
static const char *default_core_note_type_name (uint32_t, char *buf,
						size_t len);
static const char *default_object_note_type_name (const char *name, uint32_t,
						  char *buf, size_t len);
static int default_core_note (const GElf_Nhdr *nhdr, const char *name,
			      GElf_Word *regs_offset, size_t *nregloc,
			      const Ebl_Register_Location **reglocs,
			      size_t *nitems, const Ebl_Core_Item **);
static int default_auxv_info (GElf_Xword a_type,
			      const char **name, const char **format);
static bool default_object_note (const char *name, uint32_t type,
				 uint32_t descsz, const char *desc);
static bool default_debugscn_p (const char *name);
static bool default_copy_reloc_p (int reloc);
static bool default_none_reloc_p (int reloc);
static bool default_relative_reloc_p (int reloc);
static bool default_check_special_symbol (Elf *elf, GElf_Ehdr *ehdr,
					  const GElf_Sym *sym,
					  const char *name,
					  const GElf_Shdr *destshdr);
static bool default_check_st_other_bits (unsigned char st_other);
static bool default_check_special_section (Ebl *, int,
					   const GElf_Shdr *, const char *);
static bool default_bss_plt_p (Elf *elf, GElf_Ehdr *ehdr);
static int default_return_value_location (Dwarf_Die *functypedie,
					  const Dwarf_Op **locops);
static ssize_t default_register_info (Ebl *ebl,
				      int regno, char *name, size_t namelen,
				      const char **prefix,
				      const char **setname,
				      int *bits, int *type);
static int default_syscall_abi (Ebl *ebl, int *sp, int *pc,
				int *callno, int args[6]);
static bool default_check_object_attribute (Ebl *ebl, const char *vendor,
					    int tag, uint64_t value,
					    const char **tag_name,
					    const char **value_name);
static int default_abi_cfi (Ebl *ebl, Dwarf_CIE *abi_info);


static void
fill_defaults (Ebl *result)
{
  result->object_type_name = default_object_type_name;
  result->reloc_type_name = default_reloc_type_name;
  result->reloc_type_check = default_reloc_type_check;
  result->reloc_valid_use = default_reloc_valid_use;
  result->reloc_simple_type = default_reloc_simple_type;
  result->gotpc_reloc_check = default_gotpc_reloc_check;
  result->segment_type_name = default_segment_type_name;
  result->section_type_name = default_section_type_name;
  result->section_name = default_section_name;
  result->machine_flag_name = default_machine_flag_name;
  result->machine_flag_check = default_machine_flag_check;
  result->machine_section_flag_check = default_machine_section_flag_check;
  result->check_special_section = default_check_special_section;
  result->symbol_type_name = default_symbol_type_name;
  result->symbol_binding_name = default_symbol_binding_name;
  result->dynamic_tag_name = default_dynamic_tag_name;
  result->dynamic_tag_check = default_dynamic_tag_check;
  result->sh_flags_combine = default_sh_flags_combine;
  result->osabi_name = default_osabi_name;
  result->core_note_type_name = default_core_note_type_name;
  result->object_note_type_name = default_object_note_type_name;
  result->core_note = default_core_note;
  result->auxv_info = default_auxv_info;
  result->object_note = default_object_note;
  result->debugscn_p = default_debugscn_p;
  result->copy_reloc_p = default_copy_reloc_p;
  result->none_reloc_p = default_none_reloc_p;
  result->relative_reloc_p = default_relative_reloc_p;
  result->check_special_symbol = default_check_special_symbol;
  result->check_st_other_bits = default_check_st_other_bits;
  result->bss_plt_p = default_bss_plt_p;
  result->return_value_location = default_return_value_location;
  result->register_info = default_register_info;
  result->syscall_abi = default_syscall_abi;
  result->check_object_attribute = default_check_object_attribute;
  result->disasm = NULL;
  result->abi_cfi = default_abi_cfi;
  result->destr = default_destr;
  result->sysvhash_entrysize = sizeof (Elf32_Word);
}


/* Find an appropriate backend for the file associated with ELF.  */
static Ebl *
openbackend (elf, emulation, machine)
     Elf *elf;
     const char *emulation;
     GElf_Half machine;
{
  Ebl *result;
  size_t cnt;

  /* First allocate the data structure for the result.  We do this
     here since this assures that the structure is always large
     enough.  */
  result = (Ebl *) calloc (1, sizeof (Ebl));
  if (result == NULL)
    {
      // XXX uncomment
      // __libebl_seterror (ELF_E_NOMEM);
      return NULL;
    }

  /* Fill in the default callbacks.  The initializer for the machine
     specific module can overwrite the values.  */
  fill_defaults (result);

  /* XXX Currently all we do is to look at 'e_machine' value in the
     ELF header.  With an internal mapping table from EM_* value to
     DSO name we try to load the appropriate module to handle this
     binary type.

     Multiple modules for the same machine type are possible and they
     will be tried in sequence.  The lookup process will only stop
     when a module which can handle the machine type is found or all
     available matching modules are tried.  */
  for (cnt = 0; cnt < nmachines; ++cnt)
    if ((emulation != NULL && strcmp (emulation, machines[cnt].emulation) == 0)
	|| (emulation == NULL && machines[cnt].em == machine))
      {
	/* Well, we know the emulation name now.  */
	result->emulation = machines[cnt].emulation;

	/* We access some data structures directly.  Make sure the 32 and
	   64 bit variants are laid out the same.  */
	assert (offsetof (Elf32_Ehdr, e_machine)
		== offsetof (Elf64_Ehdr, e_machine));
	assert (sizeof (((Elf32_Ehdr *) 0)->e_machine)
		== sizeof (((Elf64_Ehdr *) 0)->e_machine));
	assert (offsetof (Elf, state.elf32.ehdr)
		== offsetof (Elf, state.elf64.ehdr));

	/* Prefer taking the information from the ELF file.  */
	if (elf == NULL)
	  {
	    result->machine = machines[cnt].em;
	    result->class = machines[cnt].class;
	    result->data = machines[cnt].data;
	  }
	else
	  {
	    result->machine = elf->state.elf32.ehdr->e_machine;
	    result->class = elf->state.elf32.ehdr->e_ident[EI_CLASS];
	    result->data = elf->state.elf32.ehdr->e_ident[EI_DATA];
	  }

#ifndef LIBEBL_SUBDIR
# define LIBEBL_SUBDIR PACKAGE
#endif
#define ORIGINDIR "$ORIGIN/../$LIB/" LIBEBL_SUBDIR "/"

	/* Give it a try.  At least the machine type matches.  First
           try to load the module.  */
	char dsoname[100];
	strcpy (stpcpy (stpcpy (dsoname, ORIGINDIR "libebl_"),
			machines[cnt].dsoname),
		".so");

	void *h = dlopen (dsoname, RTLD_LAZY);
	if (h == NULL)
	  {
	    strcpy (stpcpy (stpcpy (dsoname, "libebl_"),
			    machines[cnt].dsoname),
		    ".so");
	    h = dlopen (dsoname, RTLD_LAZY);
	  }

	  /* Try without an explicit path.  */
	if (h != NULL)
	  {
	    /* We managed to load the object.  Now see whether the
	       initialization function likes our file.  */
	    static const char version[] = MODVERSION;
	    const char *modversion;
	    ebl_bhinit_t initp;
	    char symname[machines[cnt].prefix_len + sizeof "_init"];

	    strcpy (mempcpy (symname, machines[cnt].prefix,
			     machines[cnt].prefix_len), "_init");

	    initp = (ebl_bhinit_t) dlsym (h, symname);
	    if (initp != NULL
		&& (modversion = initp (elf, machine, result, sizeof (Ebl)))
		&& strcmp (version, modversion) == 0)
	      {
		/* We found a module to handle our file.  */
		result->dlhandle = h;
		result->elf = elf;

		/* A few entries are mandatory.  */
		assert (result->name != NULL);
		assert (result->destr != NULL);

		return result;
	      }

	    /* Not the module we need.  */
	    (void) dlclose (h);
	  }

	/* We cannot find a DSO but the emulation/machine ID matches.
	   Return that information.  */
	result->dlhandle = NULL;
	result->elf = elf;
	result->name = machines[cnt].prefix;
	fill_defaults (result);

	return result;
      }

  /* Nothing matched.  We use only the default callbacks.   */
  result->dlhandle = NULL;
  result->elf = elf;
  result->emulation = "<unknown>";
  result->name = "<unknown>";
  fill_defaults (result);

  return result;
}


/* Find an appropriate backend for the file associated with ELF.  */
Ebl *
ebl_openbackend (elf)
     Elf *elf;
{
  GElf_Ehdr ehdr_mem;
  GElf_Ehdr *ehdr;

  /* Get the ELF header of the object.  */
  ehdr = gelf_getehdr (elf, &ehdr_mem);
  if (ehdr == NULL)
    {
      // XXX uncomment
      // __libebl_seterror (elf_errno ());
      return NULL;
    }

  return openbackend (elf, NULL, ehdr->e_machine);
}


/* Find backend without underlying ELF file.  */
Ebl *
ebl_openbackend_machine (machine)
     GElf_Half machine;
{
  return openbackend (NULL, NULL, machine);
}


/* Find backend with given emulation name.  */
Ebl *
ebl_openbackend_emulation (const char *emulation)
{
  return openbackend (NULL, emulation, EM_NONE);
}


/* Default callbacks.  Mostly they just return the error value.  */
static const char *
default_object_type_name (int ignore __attribute__ ((unused)),
			  char *buf __attribute__ ((unused)),
			  size_t len __attribute__ ((unused)))
{
  return NULL;
}

static const char *
default_reloc_type_name (int ignore __attribute__ ((unused)),
			 char *buf __attribute__ ((unused)),
			 size_t len __attribute__ ((unused)))
{
  return NULL;
}

static bool
default_reloc_type_check (int ignore __attribute__ ((unused)))
{
  return false;
}

static bool
default_reloc_valid_use (Elf *elf __attribute__ ((unused)),
			 int ignore __attribute__ ((unused)))
{
  return false;
}

static Elf_Type
default_reloc_simple_type (Ebl *eh __attribute__ ((unused)),
			   int ignore __attribute__ ((unused)))
{
  return ELF_T_NUM;
}

static bool
default_gotpc_reloc_check (Elf *elf __attribute__ ((unused)),
			   int ignore __attribute__ ((unused)))
{
  return false;
}

static const char *
default_segment_type_name (int ignore __attribute__ ((unused)),
			   char *buf __attribute__ ((unused)),
			   size_t len __attribute__ ((unused)))
{
  return NULL;
}

static const char *
default_section_type_name (int ignore __attribute__ ((unused)),
			   char *buf __attribute__ ((unused)),
			   size_t len __attribute__ ((unused)))
{
  return NULL;
}

static const char *
default_section_name (int ignore __attribute__ ((unused)),
		      int ignore2 __attribute__ ((unused)),
		      char *buf __attribute__ ((unused)),
		      size_t len __attribute__ ((unused)))
{
  return NULL;
}

static const char *
default_machine_flag_name (Elf64_Word *ignore __attribute__ ((unused)))
{
  return NULL;
}

static bool
default_machine_flag_check (Elf64_Word flags __attribute__ ((unused)))
{
  return flags == 0;
}

static bool
default_machine_section_flag_check (GElf_Xword flags)
{
  return flags == 0;
}

static bool
default_check_special_section (Ebl *ebl __attribute__ ((unused)),
			       int ndx __attribute__ ((unused)),
			       const GElf_Shdr *shdr __attribute__ ((unused)),
			       const char *sname __attribute__ ((unused)))
{
  return false;
}

static const char *
default_symbol_type_name (int ignore __attribute__ ((unused)),
			  char *buf __attribute__ ((unused)),
			  size_t len __attribute__ ((unused)))
{
  return NULL;
}

static const char *
default_symbol_binding_name (int ignore __attribute__ ((unused)),
			     char *buf __attribute__ ((unused)),
			     size_t len __attribute__ ((unused)))
{
  return NULL;
}

static const char *
default_dynamic_tag_name (int64_t ignore __attribute__ ((unused)),
			  char *buf __attribute__ ((unused)),
			  size_t len __attribute__ ((unused)))
{
  return NULL;
}

static bool
default_dynamic_tag_check (int64_t ignore __attribute__ ((unused)))
{
  return false;
}

static GElf_Word
default_sh_flags_combine (GElf_Word flags1, GElf_Word flags2)
{
  return SH_FLAGS_COMBINE (flags1, flags2);
}

static void
default_destr (struct ebl *ignore __attribute__ ((unused)))
{
}

static const char *
default_osabi_name (int ignore __attribute__ ((unused)),
		    char *buf __attribute__ ((unused)),
		    size_t len __attribute__ ((unused)))
{
  return NULL;
}

static const char *
default_core_note_type_name (uint32_t ignore __attribute__ ((unused)),
			     char *buf __attribute__ ((unused)),
			     size_t len __attribute__ ((unused)))
{
  return NULL;
}

static int
default_auxv_info (GElf_Xword a_type __attribute__ ((unused)),
		   const char **name __attribute__ ((unused)),
		   const char **format __attribute__ ((unused)))
{
  return 0;
}

static int
default_core_note (const GElf_Nhdr *nhdr __attribute__ ((unused)),
		   const char *name __attribute__ ((unused)),
		   GElf_Word *ro __attribute__ ((unused)),
		   size_t *nregloc  __attribute__ ((unused)),
		   const Ebl_Register_Location **reglocs
		   __attribute__ ((unused)),
		   size_t *nitems __attribute__ ((unused)),
		   const Ebl_Core_Item **items __attribute__ ((unused)))
{
  return 0;
}

static const char *
default_object_note_type_name (const char *name __attribute__ ((unused)),
			       uint32_t ignore __attribute__ ((unused)),
			       char *buf __attribute__ ((unused)),
			       size_t len __attribute__ ((unused)))
{
  return NULL;
}

static bool
default_object_note (const char *name __attribute__ ((unused)),
		     uint32_t type __attribute__ ((unused)),
		     uint32_t descsz __attribute__ ((unused)),
		     const char *desc __attribute__ ((unused)))
{
  return NULL;
}

static bool
default_debugscn_p (const char *name)
{
  /* We know by default only about the DWARF debug sections which have
     fixed names.  */
  static const char *dwarf_scn_names[] =
    {
      /* DWARF 1 */
      ".debug",
      ".line",
      /* GNU DWARF 1 extensions */
      ".debug_srcinfo",
      ".debug_sfnames",
      /* DWARF 1.1 and DWARF 2 */
      ".debug_aranges",
      ".debug_pubnames",
      /* DWARF 2 */
      ".debug_info",
      ".debug_abbrev",
      ".debug_line",
      ".debug_frame",
      ".debug_str",
      ".debug_loc",
      ".debug_macinfo",
      /* DWARF 3 */
      ".debug_ranges",
      ".debug_pubtypes",
      /* DWARF 4 */
      ".debug_types",
      /* GDB DWARF 4 extension */
      ".gdb_index",
      /* GNU/DWARF 5 extension/proposal */
      ".debug_macro",
      /* SGI/MIPS DWARF 2 extensions */
      ".debug_weaknames",
      ".debug_funcnames",
      ".debug_typenames",
      ".debug_varnames"
    };
  const size_t ndwarf_scn_names = (sizeof (dwarf_scn_names)
				   / sizeof (dwarf_scn_names[0]));
  for (size_t cnt = 0; cnt < ndwarf_scn_names; ++cnt)
    if (strcmp (name, dwarf_scn_names[cnt]) == 0)
      return true;

  return false;
}

static bool
default_copy_reloc_p (int reloc __attribute__ ((unused)))
{
  return false;
}
strong_alias (default_copy_reloc_p, default_none_reloc_p)
strong_alias (default_copy_reloc_p, default_relative_reloc_p)

static bool
default_check_special_symbol (Elf *elf __attribute__ ((unused)),
			      GElf_Ehdr *ehdr __attribute__ ((unused)),
			      const GElf_Sym *sym __attribute__ ((unused)),
			      const char *name __attribute__ ((unused)),
			      const GElf_Shdr *destshdr __attribute__ ((unused)))
{
  return false;
}

static bool
default_check_st_other_bits (unsigned char st_other __attribute__ ((unused)))
{
  return false;
}


static bool
default_bss_plt_p (Elf *elf __attribute__ ((unused)),
		   GElf_Ehdr *ehdr __attribute__ ((unused)))
{
  return false;
}

static int
default_return_value_location (Dwarf_Die *functypedie __attribute__ ((unused)),
			       const Dwarf_Op **locops __attribute__ ((unused)))
{
  return -2;
}

static ssize_t
default_register_info (Ebl *ebl __attribute__ ((unused)),
		       int regno, char *name, size_t namelen,
		       const char **prefix,
		       const char **setname,
		       int *bits, int *type)
{
  if (name == NULL)
    return 0;

  *setname = "???";
  *prefix = "";
  *bits = -1;
  *type = DW_ATE_void;
  return snprintf (name, namelen, "reg%d", regno);
}

static int
default_syscall_abi (Ebl *ebl __attribute__ ((unused)),
		     int *sp, int *pc, int *callno, int args[6])
{
  *sp = *pc = *callno = -1;
  args[0] = -1;
  args[1] = -1;
  args[2] = -1;
  args[3] = -1;
  args[4] = -1;
  args[5] = -1;
  return -1;
}

static bool
default_check_object_attribute (Ebl *ebl __attribute__ ((unused)),
				const char *vendor  __attribute__ ((unused)),
				int tag __attribute__ ((unused)),
				uint64_t value __attribute__ ((unused)),
				const char **tag_name, const char **value_name)
{
  *tag_name = NULL;
  *value_name = NULL;
  return false;
}

static int
default_abi_cfi (Ebl *ebl __attribute__ ((unused)),
		 Dwarf_CIE *abi_info __attribute__ ((unused)))
{
  return -1;
}
