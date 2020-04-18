/* ELF string table handling.
   Copyright (C) 2000, 2001, 2002 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2000.

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
#include <inttypes.h>
#include <libelf.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <sys/param.h>

#include "libebl.h"
#include <system.h>

#ifndef MIN
# define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif


struct Ebl_WStrent
{
  const wchar_t *string;
  size_t len;
  struct Ebl_WStrent *next;
  struct Ebl_WStrent *left;
  struct Ebl_WStrent *right;
  size_t offset;
  wchar_t reverse[0];
};


struct memoryblock
{
  struct memoryblock *next;
  char memory[0];
};


struct Ebl_WStrtab
{
  struct Ebl_WStrent *root;
  struct memoryblock *memory;
  char *backp;
  size_t left;
  size_t total;
  bool nullstr;

  struct Ebl_WStrent null;
};


/* Cache for the pagesize.  We correct this value a bit so that `malloc'
   is not allocating more than a page.  */
static size_t ps;


struct Ebl_WStrtab *
ebl_wstrtabinit (bool nullstr)
{
  struct Ebl_WStrtab *ret;

  if (ps == 0)
    {
      ps = sysconf (_SC_PAGESIZE) - 2 * sizeof (void *);
      assert (sizeof (struct memoryblock) < ps);
    }

  ret = (struct Ebl_WStrtab *) calloc (1, sizeof (struct Ebl_WStrtab));
  if (ret != NULL)
    {
      ret->nullstr = nullstr;
      if (nullstr)
	{
	  ret->null.len = 1;
	  ret->null.string = L"";
	}
    }
  return ret;
}


static int
morememory (struct Ebl_WStrtab *st, size_t len)
{
  struct memoryblock *newmem;

  if (len < ps)
    len = ps;
  newmem = (struct memoryblock *) malloc (len);
  if (newmem == NULL)
    return 1;

  newmem->next = st->memory;
  st->memory = newmem;
  st->backp = newmem->memory;
  st->left = len - offsetof (struct memoryblock, memory);

  return 0;
}


void
ebl_wstrtabfree (struct Ebl_WStrtab *st)
{
  struct memoryblock *mb = st->memory;

  while (mb != NULL)
    {
      void *old = mb;
      mb = mb->next;
      free (old);
    }

  free (st);
}


static struct Ebl_WStrent *
newstring (struct Ebl_WStrtab *st, const wchar_t *str, size_t len)
{
  struct Ebl_WStrent *newstr;
  size_t align;
  int i;

  /* Compute the amount of padding needed to make the structure aligned.  */
  align = ((__alignof__ (struct Ebl_WStrent)
	    - (((uintptr_t) st->backp)
	       & (__alignof__ (struct Ebl_WStrent) - 1)))
	   & (__alignof__ (struct Ebl_WStrent) - 1));

  /* Make sure there is enough room in the memory block.  */
  if (st->left < align + sizeof (struct Ebl_WStrent) + len * sizeof (wchar_t))
    {
      if (morememory (st,
		      sizeof (struct Ebl_WStrent) + len * sizeof (wchar_t)))
	return NULL;

      align = 0;
    }

  /* Create the reserved string.  */
  newstr = (struct Ebl_WStrent *) (st->backp + align);
  newstr->string = str;
  newstr->len = len;
  newstr->next = NULL;
  newstr->left = NULL;
  newstr->right = NULL;
  newstr->offset = 0;
  for (i = len - 2; i >= 0; --i)
    newstr->reverse[i] = str[len - 2 - i];
  newstr->reverse[len - 1] = L'\0';
  st->backp += align + sizeof (struct Ebl_WStrent) + len * sizeof (wchar_t);
  st->left -= align + sizeof (struct Ebl_WStrent) + len * sizeof (wchar_t);

  return newstr;
}


/* XXX This function should definitely be rewritten to use a balancing
   tree algorith (AVL, red-black trees).  For now a simple, correct
   implementation is enough.  */
static struct Ebl_WStrent **
searchstring (struct Ebl_WStrent **sep, struct Ebl_WStrent *newstr)
{
  int cmpres;

  /* More strings?  */
  if (*sep == NULL)
    {
      *sep = newstr;
      return sep;
    }

  /* Compare the strings.  */
  cmpres = wmemcmp ((*sep)->reverse, newstr->reverse,
		    MIN ((*sep)->len, newstr->len) - 1);
  if (cmpres == 0)
    /* We found a matching string.  */
    return sep;
  else if (cmpres > 0)
    return searchstring (&(*sep)->left, newstr);
  else
    return searchstring (&(*sep)->right, newstr);
}


/* Add new string.  The actual string is assumed to be permanent.  */
struct Ebl_WStrent *
ebl_wstrtabadd (struct Ebl_WStrtab *st, const wchar_t *str, size_t len)
{
  struct Ebl_WStrent *newstr;
  struct Ebl_WStrent **sep;

  /* Compute the string length if the caller doesn't know it.  */
  if (len == 0)
    len = wcslen (str) + 1;

  /* Make sure all "" strings get offset 0 but only if the table was
     created with a special null entry in mind.  */
  if (len == 1 && st->null.string != NULL)
    return &st->null;

  /* Allocate memory for the new string and its associated information.  */
  newstr = newstring (st, str, len);
  if (newstr == NULL)
    return NULL;

  /* Search in the array for the place to insert the string.  If there
     is no string with matching prefix and no string with matching
     leading substring, create a new entry.  */
  sep = searchstring (&st->root, newstr);
  if (*sep != newstr)
    {
      /* This is not the same entry.  This means we have a prefix match.  */
      if ((*sep)->len > newstr->len)
	{
	  struct Ebl_WStrent *subs;

	  /* Check whether we already know this string.  */
	  for (subs = (*sep)->next; subs != NULL; subs = subs->next)
	    if (subs->len == newstr->len)
	      {
		/* We have an exact match with a substring.  Free the memory
		   we allocated.  */
		st->left += st->backp - (char *) newstr;
		st->backp = (char *) newstr;

		return subs;
	      }

	  /* We have a new substring.  This means we don't need the reverse
	     string of this entry anymore.  */
	  st->backp -= newstr->len;
	  st->left += newstr->len;

	  newstr->next = (*sep)->next;
	  (*sep)->next = newstr;
	}
      else if ((*sep)->len != newstr->len)
	{
	  /* When we get here it means that the string we are about to
	     add has a common prefix with a string we already have but
	     it is longer.  In this case we have to put it first.  */
	  st->total += newstr->len - (*sep)->len;
	  newstr->next = *sep;
	  newstr->left = (*sep)->left;
	  newstr->right = (*sep)->right;
	  *sep = newstr;
	}
      else
	{
	  /* We have an exact match.  Free the memory we allocated.  */
	  st->left += st->backp - (char *) newstr;
	  st->backp = (char *) newstr;

	  newstr = *sep;
	}
    }
  else
    st->total += newstr->len;

  return newstr;
}


static void
copystrings (struct Ebl_WStrent *nodep, wchar_t **freep, size_t *offsetp)
{
  struct Ebl_WStrent *subs;

  if (nodep->left != NULL)
    copystrings (nodep->left, freep, offsetp);

  /* Process the current node.  */
  nodep->offset = *offsetp;
  *freep = wmempcpy (*freep, nodep->string, nodep->len);
  *offsetp += nodep->len * sizeof (wchar_t);

  for (subs = nodep->next; subs != NULL; subs = subs->next)
    {
      assert (subs->len < nodep->len);
      subs->offset = nodep->offset + nodep->len - subs->len;
      assert (subs->offset != 0 || subs->string[0] == '\0');
    }

  if (nodep->right != NULL)
    copystrings (nodep->right, freep, offsetp);
}


void
ebl_wstrtabfinalize (struct Ebl_WStrtab *st, Elf_Data *data)
{
  size_t copylen;
  wchar_t *endp;
  size_t nulllen = st->nullstr ? 1 : 0;

  /* Fill in the information.  */
  data->d_buf = malloc ((st->total + nulllen) * sizeof (wchar_t));
  if (data->d_buf == NULL)
    abort ();

  /* The first byte must always be zero if we created the table with a
     null string.  */
  if (st->nullstr)
    *((wchar_t *) data->d_buf) = L'\0';

  data->d_type = ELF_T_BYTE;
  data->d_size = st->total + nulllen;
  data->d_off = 0;
  data->d_align = 1;
  data->d_version = EV_CURRENT;

  /* Now run through the tree and add all the string while also updating
     the offset members of the elfstrent records.  */
  endp = (wchar_t *) data->d_buf + nulllen;
  copylen = sizeof (wchar_t) * nulllen;
  copystrings (st->root, &endp, &copylen);
  assert (copylen == (st->total + nulllen) * sizeof (wchar_t));
}


size_t
ebl_wstrtaboffset (struct Ebl_WStrent *se)
{
  return se->offset;
}
