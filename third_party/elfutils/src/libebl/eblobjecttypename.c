/* Return object file type name.
   Copyright (C) 2001, 2002 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2001.

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

#include <stdio.h>
#include <libeblP.h>


const char *
ebl_object_type_name (ebl, object, buf, len)
     Ebl *ebl;
     int object;
     char *buf;
     size_t len;
{
  const char *res;

  res = ebl != NULL ? ebl->object_type_name (object, buf, len) : NULL;
  if (res == NULL)
    {
      /* Handle OS-specific section names.  */
      if (object >= ET_LOOS && object <= ET_HIOS)
	snprintf (buf, len, "LOOS+%x", object - ET_LOOS);
      /* Handle processor-specific section names.  */
      else if (object >= ET_LOPROC && object <= ET_HIPROC)
	snprintf (buf, len, "LOPROC+%x", object - ET_LOPROC);
      else
	snprintf (buf, len, "%s: %d", gettext ("<unknown>"), object);

      res = buf;
    }

  return res;
}
