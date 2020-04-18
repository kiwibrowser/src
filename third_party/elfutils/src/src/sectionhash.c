/* Section hash table implementation.
   Copyright (C) 2001, 2002, 2005 Red Hat, Inc.
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

#include <string.h>

#include <elf-knowledge.h>
#include <ld.h>


/* Comparison function for sections.  */
static int
scnhead_compare (struct scnhead *one, struct scnhead *two)
{
  int result = strcmp (one->name, two->name);

  if (result == 0)
    {
      result = one->type - two->type;

      if (result == 0)
	{
	  GElf_Sxword diff = (SH_FLAGS_IMPORTANT (one->flags)
			     - SH_FLAGS_IMPORTANT (two->flags));
	  result = diff < 0 ? -1 : diff == 0 ? 0 : 1;

	  if (result == 0)
	    {
	      result = one->entsize - two->entsize;

	      if (result == 0)
		{
		  result = (one->grp_signature == NULL
			    ? (two->grp_signature == NULL ? 0 : -1)
			    : (two->grp_signature == NULL
			       ? 1 : strcmp (one->grp_signature,
					     two->grp_signature)));

		  if (result == 0)
		    result = one->kind - two->kind;
		}
	    }
	}
    }

  return result;
}

/* Definitions for the section hash table.  */
#define TYPE struct scnhead *
#define NAME ld_section_tab
#define ITERATE 1
#define COMPARE(a, b) scnhead_compare (a, b)

#include "../lib/dynamicsizehash.c"
