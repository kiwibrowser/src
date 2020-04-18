/* Macros to enable writing native and generic ELF access code.
   Copyright (C) 2003 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2003.

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

#include <libebl.h>


/* By default the linker is handling all architectures.  But it can
   be configured to be a native-only linker.  */
#if NATIVE_ELF == 32
/* 32-bit only.  */
# define XElf_Ehdr Elf32_Ehdr
# define XElf_Shdr Elf32_Shdr
# define XElf_Off Elf32_Off
# define XElf_Addr Elf32_Addr
# define XElf_Half Elf32_Half
# define XElf_Word Elf32_Word
# define XElf_Xword Elf32_Word
# define XElf_Sxword Elf32_Sword
# define XElf_Versym Elf32_Versym
# define XElf_Sym Elf32_Sym
# define XElf_Rel Elf32_Rel
# define XElf_Rela Elf32_Rela

# define XElf_Ehdr_vardef(name) Elf32_Ehdr *name
# define xelf_getehdr(elf, name) name = elf32_getehdr (elf)
# define xelf_getehdr_copy(elf, name, copy) \
  (copy) = *(name = elf32_getehdr (elf))
# define xelf_newehdr(elf, klass) elf32_newehdr (elf)
# define xelf_update_ehdr(elf, ehdr) \
  /* nothing */ ((void) (elf), (void) (ehdr), 1)

# define xelf_getclass(elf) ELFCLASS32

# define XElf_Phdr_vardef(name) Elf32_Phdr *name
# define xelf_newphdr(elf, n) elf32_newphdr (elf, n)
# define xelf_getphdr(elf, idx, name) name = elf32_getphdr (elf) + idx
# define xelf_getphdr_ptr(elf, idx, name) name = elf32_getphdr (elf) + idx
# define xelf_update_phdr(elf, idx, phdr) \
  /* nothing */ ((void) (elf), (void) (idx), (void) (phdr), 1)

# define XElf_Shdr_vardef(name) Elf32_Shdr *name
# define xelf_getshdr(scn, name) name = elf32_getshdr (scn)
# define xelf_getshdr_copy(scn, name, copy) \
  (copy) = *(name = elf32_getshdr (scn))
# define xelf_update_shdr(scn, shdr) \
  /* nothing */ ((void) (scn), (void) (shdr), 1)

# define XElf_Sym_vardef(name) Elf32_Sym *name
# define xelf_getsym(data, idx, name) \
  name = &((Elf32_Sym *) (data)->d_buf)[idx]
# define xelf_getsym_ptr(data, idx, name) \
  name = &((Elf32_Sym *) (data)->d_buf)[idx]
# define xelf_getsymshndx(data, ndxdata, idx, name1, name2) \
  (name1 = &((Elf32_Sym *) ((data)->d_buf))[idx]);			      \
  name2 = (unlikely ((ndxdata) != NULL)					      \
	   ? ((Elf32_Word *) ((ndxdata)->d_buf))[idx] : 0)
# define xelf_update_sym(data, idx, sym) \
  /* nothing */ ((void) (data), (void) (idx), (void) (sym), 1)
# define xelf_update_symshndx(data, ndxdata, idx, name1, name2, datachanged) \
  if (datachanged)							      \
    ((Elf32_Sym *) ((data)->d_buf))[idx] = *name1;			      \
  if (unlikely (ndxdata != NULL))					      \
    ((Elf32_Word *) ((ndxdata)->d_buf))[idx] = name2

# define XElf_Versym_vardef(name) Elf32_Versym name
# define xelf_getversym_copy(data, idx, name) \
  (name = ((Elf32_Versym *) ((data)->d_buf))[idx], &name)

# define XElf_Dyn_vardef(name) Elf32_Dyn *name
# define xelf_getdyn(data, idx, name) \
  name = &((Elf32_Dyn *) ((data)->d_buf))[idx]
# define xelf_getdyn_ptr(data, idx, name) \
  name = &((Elf32_Dyn *) ((data)->d_buf))[idx]
# define xelf_update_dyn(data, idx, name) \
  /* nothing */ ((void) (data), (void) (idx), (void) (name), 1)

# define XElf_Rel_vardef(name) Elf32_Rel *name
# define xelf_getrel(data, idx, name) \
  name = &((Elf32_Rel *) ((data)->d_buf))[idx]
# define xelf_getrel_ptr(data, idx, name) \
  name = &((Elf32_Rel *) ((data)->d_buf))[idx]
# define xelf_update_rel(data, idx, name) \
  /* nothing */ ((void) (data), (void) (idx), (void) (name), 1)

# define XElf_Rela_vardef(name) Elf32_Rela *name
# define xelf_getrela(data, idx, name) \
  name = &((Elf32_Rela *) ((data)->d_buf))[idx]
# define xelf_getrela_ptr(data, idx, name) \
  name = &((Elf32_Rela *) ((data)->d_buf))[idx]
# define xelf_update_rela(data, idx, name) \
  /* nothing */ ((void) (data), (void) (idx), (void) (name), 1)

# define XElf_Verdef_vardef(name) Elf32_Verdef *name
# define xelf_getverdef(data, offset, name) \
  name = ((Elf32_Verdef *) ((char *) ((data)->d_buf) + (offset)))

# define XElf_Verdaux_vardef(name) Elf32_Verdaux *name
# define xelf_getverdaux(data, offset, name) \
  name = ((Elf32_Verdaux *) ((char *) ((data)->d_buf) + (offset)))

# define XELF_ST_TYPE(info) ELF32_ST_TYPE (info)
# define XELF_ST_BIND(info) ELF32_ST_BIND (info)
# define XELF_ST_INFO(bind, type) ELF32_ST_INFO (bind, type)
# define XELF_ST_VISIBILITY(info) ELF32_ST_VISIBILITY (info)

# define XELF_R_SYM(info) ELF32_R_SYM (info)
# define XELF_R_TYPE(info) ELF32_R_TYPE (info)
# define XELF_R_INFO(sym, type) ELF32_R_INFO (sym, type)

# define xelf_fsize(elf, type, cnt) \
  (__builtin_constant_p (type)						      \
   ? ({ size_t fsize;							      \
        switch (type)							      \
	  {								      \
	  case ELF_T_BYTE: fsize = 1; break;				      \
	  case ELF_T_ADDR: fsize = sizeof (Elf32_Addr); break;		      \
	  case ELF_T_DYN: fsize = sizeof (Elf32_Dyn); break;		      \
	  case ELF_T_EHDR: fsize = sizeof (Elf32_Ehdr); break;		      \
	  case ELF_T_HALF: fsize = sizeof (Elf32_Half); break;		      \
	  case ELF_T_OFF: fsize = sizeof (Elf32_Off); break;		      \
	  case ELF_T_PHDR: fsize = sizeof (Elf32_Phdr); break;		      \
	  case ELF_T_RELA: fsize = sizeof (Elf32_Rela); break;		      \
	  case ELF_T_REL: fsize = sizeof (Elf32_Rel); break;		      \
	  case ELF_T_SHDR: fsize = sizeof (Elf32_Shdr); break;		      \
	  case ELF_T_SWORD: fsize = sizeof (Elf32_Sword); break;	      \
	  case ELF_T_SYM: fsize = sizeof (Elf32_Sym); break;		      \
	  case ELF_T_WORD: fsize = sizeof (Elf32_Word); break;		      \
	  case ELF_T_XWORD: fsize = sizeof (Elf32_Xword); break;	      \
	  case ELF_T_SXWORD: fsize = sizeof (Elf32_Sxword); break;	      \
	  case ELF_T_VDEF: fsize = sizeof (Elf32_Verdef); break;	      \
	  case ELF_T_VDAUX: fsize = sizeof (Elf32_Verdaux); break;	      \
	  case ELF_T_VNEED: fsize = sizeof (Elf32_Verneed); break;	      \
	  case ELF_T_VNAUX: fsize = sizeof (Elf32_Vernaux); break;	      \
	  case ELF_T_NHDR: fsize = sizeof (Elf32_Nhdr); break;		      \
	  case ELF_T_SYMINFO: fsize = sizeof (Elf32_Syminfo); break;	      \
	  case ELF_T_MOVE: fsize = sizeof (Elf32_Move); break;		      \
          default: fsize = 0; break;					      \
	  }								      \
        fsize * (cnt); })						      \
   : gelf_fsize (elf, type, cnt, EV_CURRENT))
#elif NATIVE_ELF == 64
/* 64-bit only.  */
# define XElf_Ehdr Elf64_Ehdr
# define XElf_Shdr Elf64_Shdr
# define XElf_Addr Elf64_Addr
# define XElf_Half Elf64_Half
# define XElf_Off Elf64_Off
# define XElf_Word Elf64_Word
# define XElf_Xword Elf64_Xword
# define XElf_Sxword Elf64_Sxword
# define XElf_Versym Elf64_Versym
# define XElf_Sym Elf64_Sym
# define XElf_Rel Elf64_Rel
# define XElf_Rela Elf64_Rela

# define XElf_Ehdr_vardef(name) Elf64_Ehdr *name
# define xelf_getehdr(elf, name) name = elf64_getehdr (elf)
# define xelf_getehdr_copy(elf, name, copy) \
  (copy) = *(name = elf64_getehdr (elf))
# define xelf_newehdr(elf, klass) elf64_newehdr (elf)
# define xelf_update_ehdr(elf, ehdr) \
  /* nothing */ ((void) (elf), (void) (ehdr), 1)

# define xelf_getclass(elf) ELFCLASS32

# define XElf_Phdr_vardef(name) Elf64_Phdr *name
# define xelf_newphdr(elf, n) elf64_newphdr (elf, n)
# define xelf_getphdr(elf, idx, name) name = elf64_getphdr (elf) + idx
# define xelf_getphdr_ptr(elf, idx, name) name = elf64_getphdr (elf) + idx
# define xelf_update_phdr(elf, idx, phdr) \
  /* nothing */ ((void) (elf), (void) (idx), (void) (phdr), 1)

# define XElf_Shdr_vardef(name) Elf64_Shdr *name
# define xelf_getshdr(scn, name) name = elf64_getshdr (scn)
# define xelf_getshdr_copy(scn, name, copy) \
  (copy) = *(name = elf64_getshdr (scn))
# define xelf_update_shdr(scn, shdr) \
  /* nothing */ ((void) (scn), (void) (shdr), 1)

# define XElf_Sym_vardef(name) Elf64_Sym *name
# define xelf_getsym(data, idx, name) \
  name = &((Elf64_Sym *) (data)->d_buf)[idx]
# define xelf_getsym_ptr(data, idx, name) \
  name = &((Elf64_Sym *) (data)->d_buf)[idx]
# define xelf_getsymshndx(data, ndxdata, idx, name1, name2) \
  (name1 = &((Elf64_Sym *) ((data)->d_buf))[idx]);			      \
  name2 = (unlikely ((ndxdata) != NULL)					      \
	   ? ((Elf32_Word *) ((ndxdata)->d_buf))[idx] : 0)
# define xelf_update_sym(data, idx, sym) \
  /* nothing */ ((void) (data), (void) (idx), (void) (sym), 1)
# define xelf_update_symshndx(data, ndxdata, idx, name1, name2, datachanged) \
  if (datachanged)							      \
    ((Elf64_Sym *) ((data)->d_buf))[idx] = *name1;			      \
  if (ndxdata != NULL)							      \
    (((Elf32_Word *) ((ndxdata)->d_buf))[idx] = name2)

# define XElf_Versym_vardef(name) Elf64_Versym name
# define xelf_getversym_copy(data, idx, name) \
  (name = ((Elf64_Versym *) ((data)->d_buf))[idx], (&name))

# define XElf_Dyn_vardef(name) Elf64_Dyn *name
# define xelf_getdyn(data, idx, name) \
  name = &((Elf64_Dyn *) ((data)->d_buf))[idx]
# define xelf_getdyn_ptr(data, idx, name) \
  name = &((Elf64_Dyn *) ((data)->d_buf))[idx]
# define xelf_update_dyn(data, idx, name) \
  /* nothing */ ((void) (data), (void) (idx), (void) (name), 1)

# define XElf_Rel_vardef(name) Elf64_Rel *name
# define xelf_getrel(data, idx, name) \
  name = &((Elf64_Rel *) ((data)->d_buf))[idx]
# define xelf_getrel_ptr(data, idx, name) \
  name = &((Elf64_Rel *) ((data)->d_buf))[idx]
# define xelf_update_rel(data, idx, name) \
  /* nothing */ ((void) (data), (void) (idx), (void) (name), 1)

# define XElf_Rela_vardef(name) Elf64_Rela *name
# define xelf_getrela(data, idx, name) \
  name = &((Elf64_Rela *) ((data)->d_buf))[idx]
# define xelf_getrela_ptr(data, idx, name) \
  name = &((Elf64_Rela *) ((data)->d_buf))[idx]
# define xelf_update_rela(data, idx, name) \
  /* nothing */ ((void) (data), (void) (idx), (void) (name), 1)

# define XElf_Verdef_vardef(name) Elf64_Verdef *name
# define xelf_getverdef(data, offset, name) \
  name = ((Elf64_Verdef *) ((char *) ((data)->d_buf) + (offset)))

# define XElf_Verdaux_vardef(name) Elf64_Verdaux *name
# define xelf_getverdaux(data, offset, name) \
  name = ((Elf64_Verdaux *) ((char *) ((data)->d_buf) + (offset)))

# define XELF_ST_TYPE(info) ELF64_ST_TYPE (info)
# define XELF_ST_BIND(info) ELF64_ST_BIND (info)
# define XELF_ST_INFO(bind, type) ELF64_ST_INFO (bind, type)
# define XELF_ST_VISIBILITY(info) ELF64_ST_VISIBILITY (info)

# define XELF_R_SYM(info) ELF64_R_SYM (info)
# define XELF_R_TYPE(info) ELF64_R_TYPE (info)
# define XELF_R_INFO(sym, type) ELF64_R_INFO (sym, type)

# define xelf_fsize(elf, type, cnt) \
  (__builtin_constant_p (type)						      \
   ? ({ size_t fsize;							      \
        switch (type)							      \
	  {								      \
	  case ELF_T_BYTE: fsize = 1; break;				      \
	  case ELF_T_ADDR: fsize = sizeof (Elf64_Addr); break;		      \
	  case ELF_T_DYN: fsize = sizeof (Elf64_Dyn); break;		      \
	  case ELF_T_EHDR: fsize = sizeof (Elf64_Ehdr); break;		      \
	  case ELF_T_HALF: fsize = sizeof (Elf64_Half); break;		      \
	  case ELF_T_OFF: fsize = sizeof (Elf64_Off); break;		      \
	  case ELF_T_PHDR: fsize = sizeof (Elf64_Phdr); break;		      \
	  case ELF_T_RELA: fsize = sizeof (Elf64_Rela); break;		      \
	  case ELF_T_REL: fsize = sizeof (Elf64_Rel); break;		      \
	  case ELF_T_SHDR: fsize = sizeof (Elf64_Shdr); break;		      \
	  case ELF_T_SWORD: fsize = sizeof (Elf64_Sword); break;	      \
	  case ELF_T_SYM: fsize = sizeof (Elf64_Sym); break;		      \
	  case ELF_T_WORD: fsize = sizeof (Elf64_Word); break;		      \
	  case ELF_T_XWORD: fsize = sizeof (Elf64_Xword); break;	      \
	  case ELF_T_SXWORD: fsize = sizeof (Elf64_Sxword); break;	      \
	  case ELF_T_VDEF: fsize = sizeof (Elf64_Verdef); break;	      \
	  case ELF_T_VDAUX: fsize = sizeof (Elf64_Verdaux); break;	      \
	  case ELF_T_VNEED: fsize = sizeof (Elf64_Verneed); break;	      \
	  case ELF_T_VNAUX: fsize = sizeof (Elf64_Vernaux); break;	      \
	  case ELF_T_NHDR: fsize = sizeof (Elf64_Nhdr); break;		      \
	  case ELF_T_SYMINFO: fsize = sizeof (Elf64_Syminfo); break;	      \
	  case ELF_T_MOVE: fsize = sizeof (Elf64_Move); break;		      \
          default: fsize = 0; break;					      \
	  }								      \
        fsize * (cnt); })						      \
   : gelf_fsize (elf, type, cnt, EV_CURRENT))
#else
# include <gelf.h>

/* Generic linker.  */
# define XElf_Ehdr GElf_Ehdr
# define XElf_Shdr GElf_Shdr
# define XElf_Addr GElf_Addr
# define XElf_Half GElf_Half
# define XElf_Off GElf_Off
# define XElf_Word GElf_Word
# define XElf_Xword GElf_Xword
# define XElf_Sxword GElf_Sxword
# define XElf_Versym GElf_Versym
# define XElf_Sym GElf_Sym
# define XElf_Rel GElf_Rel
# define XElf_Rela GElf_Rela

# define XElf_Ehdr_vardef(name) GElf_Ehdr name##_mem; GElf_Ehdr *name
# define xelf_getehdr(elf, name) name = gelf_getehdr (elf, &name##_mem)
# define xelf_getehdr_copy(elf, name, copy) \
  name = gelf_getehdr (elf, &(copy))
# define xelf_newehdr(elf, klass) gelf_newehdr (elf, klass)
# define xelf_update_ehdr(elf, ehdr) gelf_update_ehdr (elf, ehdr)

# define xelf_getclass(elf) gelf_getclass (elf)

# define XElf_Phdr_vardef(name) GElf_Phdr name##_mem; GElf_Phdr *name
# define xelf_newphdr(elf, n) gelf_newphdr (elf, n)
# define xelf_getphdr(elf, idx, name) \
  name = gelf_getphdr (elf, idx, &name##_mem)
# define xelf_getphdr_ptr(elf, idx, name) \
  name = &name##_mem
# define xelf_update_phdr(elf, idx, phdr) \
  gelf_update_phdr (elf, idx, phdr)

# define XElf_Shdr_vardef(name) GElf_Shdr name##_mem; GElf_Shdr *name
# define xelf_getshdr(scn, name) name = gelf_getshdr (scn, &name##_mem)
# define xelf_getshdr_copy(scn, name, copy) \
  name = gelf_getshdr (scn, &(copy))
# define xelf_update_shdr(scn, shdr) gelf_update_shdr (scn, shdr)

# define XElf_Sym_vardef(name) GElf_Sym name##_mem; GElf_Sym *name
# define xelf_getsym(data, idx, name) \
  name = gelf_getsym (data, idx, &name##_mem)
# define xelf_getsym_ptr(data, idx, name) \
  name = &name##_mem
# define xelf_getsymshndx(data, ndxdata, idx, name1, name2) \
  name1 = gelf_getsymshndx (data, ndxdata, idx, &name1##_mem, &(name2))
# define xelf_update_sym(data, idx, sym) gelf_update_sym (data, idx, sym)
# define xelf_update_symshndx(data, ndxdata, idx, name1, name2, datachanged) \
  gelf_update_symshndx (data, ndxdata, idx, name1, name2)

# define XElf_Versym_vardef(name) GElf_Versym name
# define xelf_getversym_copy(data, idx, name) \
  gelf_getversym (data, idx, &name)

# define XElf_Dyn_vardef(name) GElf_Dyn name##_mem; GElf_Dyn *name
# define xelf_getdyn(data, idx, name) \
  name = gelf_getdyn (data, idx, &name##_mem)
# define xelf_getdyn_ptr(data, idx, name) \
  name = &name##_mem
# define xelf_update_dyn(data, idx, name) \
  gelf_update_dyn (data, idx, name)

# define XElf_Rel_vardef(name) GElf_Rel name##_mem; GElf_Rel *name
# define xelf_getrel(data, idx, name) \
  name = gelf_getrel (data, idx, &name##_mem)
# define xelf_getrel_ptr(data, idx, name) \
  name = &name##_mem
# define xelf_update_rel(data, idx, name) \
  gelf_update_rel (data, idx, name)

# define XElf_Rela_vardef(name) GElf_Rela name##_mem; GElf_Rela *name
# define xelf_getrela(data, idx, name) \
  name = gelf_getrela (data, idx, &name##_mem)
# define xelf_getrela_ptr(data, idx, name) \
  name = &name##_mem
# define xelf_update_rela(data, idx, name) \
  gelf_update_rela (data, idx, name)

# define XElf_Verdef_vardef(name) GElf_Verdef name##_mem; GElf_Verdef *name
# define xelf_getverdef(data, offset, name) \
  name = gelf_getverdef (data, offset, &name##_mem)

# define XElf_Verdaux_vardef(name) GElf_Verdaux name##_mem; GElf_Verdaux *name
# define xelf_getverdaux(data, offset, name) \
  name = gelf_getverdaux (data, offset, &name##_mem)

# define XELF_ST_TYPE(info) GELF_ST_TYPE (info)
# define XELF_ST_BIND(info) GELF_ST_BIND (info)
# define XELF_ST_INFO(bind, type) GELF_ST_INFO (bind, type)
# define XELF_ST_VISIBILITY(info) GELF_ST_VISIBILITY (info)

# define XELF_R_SYM(info) GELF_R_SYM (info)
# define XELF_R_TYPE(info) GELF_R_TYPE (info)
# define XELF_R_INFO(sym, type) GELF_R_INFO (sym, type)

# define xelf_fsize(elf, type, cnt) \
  gelf_fsize (elf, type, cnt, EV_CURRENT)
#endif
