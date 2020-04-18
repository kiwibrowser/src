/* Return note type name.
   Copyright (C) 2002, 2007, 2009, 2011 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2002.

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

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <libeblP.h>


const char *
ebl_object_note_type_name (ebl, name, type, buf, len)
     Ebl *ebl;
     const char *name;
     uint32_t type;
     char *buf;
     size_t len;
{
  const char *res = ebl->object_note_type_name (name, type, buf, len);

  if (res == NULL)
    {
      if (strcmp (name, "stapsdt") == 0)
	{
	  snprintf (buf, len, "Version: %" PRIu32, type);
	  return buf;
	}

      static const char *knowntypes[] =
	{
#define KNOWNSTYPE(name) [NT_##name] = #name
	  KNOWNSTYPE (VERSION),
	  KNOWNSTYPE (GNU_HWCAP),
	  KNOWNSTYPE (GNU_BUILD_ID),
	  KNOWNSTYPE (GNU_GOLD_VERSION),
	};

      /* Handle standard names.  */
      if (type < sizeof (knowntypes) / sizeof (knowntypes[0])
	  && knowntypes[type] != NULL)
	res = knowntypes[type];
      else
	{
	  snprintf (buf, len, "%s: %" PRIu32, gettext ("<unknown>"), type);

	  res = buf;
	}
    }

  return res;
}
