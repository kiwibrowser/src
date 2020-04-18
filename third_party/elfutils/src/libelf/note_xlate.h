/* Conversion functions for notes.
   Copyright (C) 2007, 2009 Red Hat, Inc.
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

static void
elf_cvt_note (void *dest, const void *src, size_t len, int encode)
{
  assert (sizeof (Elf32_Nhdr) == sizeof (Elf64_Nhdr));

  while (len >= sizeof (Elf32_Nhdr))
    {
      (1 ? Elf32_cvt_Nhdr : Elf64_cvt_Nhdr) (dest, src, sizeof (Elf32_Nhdr),
					     encode);
      const Elf32_Nhdr *n = encode ? src : dest;
      Elf32_Word namesz = NOTE_ALIGN (n->n_namesz);
      Elf32_Word descsz = NOTE_ALIGN (n->n_descsz);

      len -= sizeof *n;
      src += sizeof *n;
      dest += sizeof *n;

      if (namesz > len)
	break;
      len -= namesz;
      if (descsz > len)
	break;
      len -= descsz;

      if (src != dest)
	memcpy (dest, src, namesz + descsz);

      src += namesz + descsz;
      dest += namesz + descsz;
    }
}
