/* Return location expression list.
   Copyright (C) 2000-2010, 2013 Red Hat, Inc.
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

#include <dwarf.h>
#include <search.h>
#include <stdlib.h>
#include <assert.h>

#include <libdwP.h>


static bool
attr_ok (Dwarf_Attribute *attr)
{
  if (attr == NULL)
    return false;

  /* Must be one of the attributes listed below.  */
  switch (attr->code)
    {
    case DW_AT_location:
    case DW_AT_data_member_location:
    case DW_AT_vtable_elem_location:
    case DW_AT_string_length:
    case DW_AT_use_location:
    case DW_AT_frame_base:
    case DW_AT_return_addr:
    case DW_AT_static_link:
    case DW_AT_segment:
      break;

    default:
      __libdw_seterrno (DWARF_E_NO_LOCLIST);
      return false;
    }

  return true;
}


struct loclist
{
  uint8_t atom;
  Dwarf_Word number;
  Dwarf_Word number2;
  Dwarf_Word offset;
  struct loclist *next;
};


static int
loc_compare (const void *p1, const void *p2)
{
  const struct loc_s *l1 = (const struct loc_s *) p1;
  const struct loc_s *l2 = (const struct loc_s *) p2;

  if ((uintptr_t) l1->addr < (uintptr_t) l2->addr)
    return -1;
  if ((uintptr_t) l1->addr > (uintptr_t) l2->addr)
    return 1;

  return 0;
}

/* For each DW_OP_implicit_value, we store a special entry in the cache.
   This points us directly to the block data for later fetching.  */
static void
store_implicit_value (Dwarf *dbg, void **cache, Dwarf_Op *op)
{
  struct loc_block_s *block = libdw_alloc (dbg, struct loc_block_s,
					   sizeof (struct loc_block_s), 1);
  const unsigned char *data = (const unsigned char *) (uintptr_t) op->number2;
  (void) __libdw_get_uleb128 (&data); // Ignored, equal to op->number.
  block->addr = op;
  block->data = (unsigned char *) data;
  block->length = op->number;
  (void) tsearch (block, cache, loc_compare);
}

int
dwarf_getlocation_implicit_value (attr, op, return_block)
     Dwarf_Attribute *attr;
     const Dwarf_Op *op;
     Dwarf_Block *return_block;
{
  if (attr == NULL)
    return -1;

  struct loc_block_s fake = { .addr = (void *) op };
  struct loc_block_s **found = tfind (&fake, &attr->cu->locs, loc_compare);
  if (unlikely (found == NULL))
    {
      __libdw_seterrno (DWARF_E_NO_BLOCK);
      return -1;
    }

  return_block->length = (*found)->length;
  return_block->data = (*found)->data;
  return 0;
}

/* DW_AT_data_member_location can be a constant as well as a loclistptr.
   Only data[48] indicate a loclistptr.  */
static int
check_constant_offset (Dwarf_Attribute *attr,
		       Dwarf_Op **llbuf, size_t *listlen)
{
  if (attr->code != DW_AT_data_member_location)
    return 1;

  switch (attr->form)
    {
      /* Punt for any non-constant form.  */
    default:
      return 1;

    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_data8:
    case DW_FORM_sdata:
    case DW_FORM_udata:
      break;
    }

  /* Check whether we already cached this location.  */
  struct loc_s fake = { .addr = attr->valp };
  struct loc_s **found = tfind (&fake, &attr->cu->locs, loc_compare);

  if (found == NULL)
    {
      Dwarf_Word offset;
      if (INTUSE(dwarf_formudata) (attr, &offset) != 0)
	return -1;

      Dwarf_Op *result = libdw_alloc (attr->cu->dbg,
				      Dwarf_Op, sizeof (Dwarf_Op), 1);

      result->atom = DW_OP_plus_uconst;
      result->number = offset;
      result->number2 = 0;
      result->offset = 0;

      /* Insert a record in the search tree so we can find it again later.  */
      struct loc_s *newp = libdw_alloc (attr->cu->dbg,
					struct loc_s, sizeof (struct loc_s),
					1);
      newp->addr = attr->valp;
      newp->loc = result;
      newp->nloc = 1;

      found = tsearch (newp, &attr->cu->locs, loc_compare);
    }

  assert ((*found)->nloc == 1);

  if (llbuf != NULL)
    {
      *llbuf = (*found)->loc;
      *listlen = 1;
    }

  return 0;
}

int
internal_function
__libdw_intern_expression (Dwarf *dbg, bool other_byte_order,
			   unsigned int address_size, unsigned int ref_size,
			   void **cache, const Dwarf_Block *block,
			   bool cfap, bool valuep,
			   Dwarf_Op **llbuf, size_t *listlen, int sec_index)
{
  /* Empty location expressions don't have any ops to intern.  */
  if (block->length == 0)
    {
      *listlen = 0;
      return 0;
    }

  /* Check whether we already looked at this list.  */
  struct loc_s fake = { .addr = block->data };
  struct loc_s **found = tfind (&fake, cache, loc_compare);
  if (found != NULL)
    {
      /* We already saw it.  */
      *llbuf = (*found)->loc;
      *listlen = (*found)->nloc;

      if (valuep)
	{
	  assert (*listlen > 1);
	  assert ((*llbuf)[*listlen - 1].atom == DW_OP_stack_value);
	}

      return 0;
    }

  const unsigned char *data = block->data;
  const unsigned char *const end_data = data + block->length;

  const struct { bool other_byte_order; } bo = { other_byte_order };

  struct loclist *loclist = NULL;
  unsigned int n = 0;

  if (cfap)
    {
      /* Synthesize the operation to push the CFA before the expression.  */
      struct loclist *newloc;
      newloc = (struct loclist *) alloca (sizeof (struct loclist));
      newloc->atom = DW_OP_call_frame_cfa;
      newloc->number = 0;
      newloc->number2 = 0;
      newloc->offset = -1;
      newloc->next = loclist;
      loclist = newloc;
      ++n;
    }

  /* Decode the opcodes.  It is possible in some situations to have a
     block of size zero.  */
  while (data < end_data)
    {
      struct loclist *newloc;
      newloc = (struct loclist *) alloca (sizeof (struct loclist));
      newloc->number = 0;
      newloc->number2 = 0;
      newloc->offset = data - block->data;
      newloc->next = loclist;
      loclist = newloc;
      ++n;

      switch ((newloc->atom = *data++))
	{
	case DW_OP_addr:
	  /* Address, depends on address size of CU.  */
	  if (__libdw_read_address_inc (dbg, sec_index, &data,
					address_size, &newloc->number))
	    return -1;
	  break;

	case DW_OP_call_ref:
	  /* DW_FORM_ref_addr, depends on offset size of CU.  */
	  if (__libdw_read_offset_inc (dbg, sec_index, &data, ref_size,
				       &newloc->number, IDX_debug_info, 0))
	    return -1;
	  break;

	case DW_OP_deref:
	case DW_OP_dup:
	case DW_OP_drop:
	case DW_OP_over:
	case DW_OP_swap:
	case DW_OP_rot:
	case DW_OP_xderef:
	case DW_OP_abs:
	case DW_OP_and:
	case DW_OP_div:
	case DW_OP_minus:
	case DW_OP_mod:
	case DW_OP_mul:
	case DW_OP_neg:
	case DW_OP_not:
	case DW_OP_or:
	case DW_OP_plus:
	case DW_OP_shl:
	case DW_OP_shr:
	case DW_OP_shra:
	case DW_OP_xor:
	case DW_OP_eq:
	case DW_OP_ge:
	case DW_OP_gt:
	case DW_OP_le:
	case DW_OP_lt:
	case DW_OP_ne:
	case DW_OP_lit0 ... DW_OP_lit31:
	case DW_OP_reg0 ... DW_OP_reg31:
	case DW_OP_nop:
	case DW_OP_push_object_address:
	case DW_OP_call_frame_cfa:
	case DW_OP_form_tls_address:
	case DW_OP_GNU_push_tls_address:
	case DW_OP_stack_value:
	  /* No operand.  */
	  break;

	case DW_OP_const1u:
	case DW_OP_pick:
	case DW_OP_deref_size:
	case DW_OP_xderef_size:
	  if (unlikely (data >= end_data))
	    {
	    invalid:
	      __libdw_seterrno (DWARF_E_INVALID_DWARF);
	      return -1;
	    }

	  newloc->number = *data++;
	  break;

	case DW_OP_const1s:
	  if (unlikely (data >= end_data))
	    goto invalid;

	  newloc->number = *((int8_t *) data);
	  ++data;
	  break;

	case DW_OP_const2u:
	  if (unlikely (data + 2 > end_data))
	    goto invalid;

	  newloc->number = read_2ubyte_unaligned_inc (&bo, data);
	  break;

	case DW_OP_const2s:
	case DW_OP_skip:
	case DW_OP_bra:
	case DW_OP_call2:
	  if (unlikely (data + 2 > end_data))
	    goto invalid;

	  newloc->number = read_2sbyte_unaligned_inc (&bo, data);
	  break;

	case DW_OP_const4u:
	  if (unlikely (data + 4 > end_data))
	    goto invalid;

	  newloc->number = read_4ubyte_unaligned_inc (&bo, data);
	  break;

	case DW_OP_const4s:
	case DW_OP_call4:
	case DW_OP_GNU_parameter_ref:
	  if (unlikely (data + 4 > end_data))
	    goto invalid;

	  newloc->number = read_4sbyte_unaligned_inc (&bo, data);
	  break;

	case DW_OP_const8u:
	  if (unlikely (data + 8 > end_data))
	    goto invalid;

	  newloc->number = read_8ubyte_unaligned_inc (&bo, data);
	  break;

	case DW_OP_const8s:
	  if (unlikely (data + 8 > end_data))
	    goto invalid;

	  newloc->number = read_8sbyte_unaligned_inc (&bo, data);
	  break;

	case DW_OP_constu:
	case DW_OP_plus_uconst:
	case DW_OP_regx:
	case DW_OP_piece:
	case DW_OP_GNU_convert:
	case DW_OP_GNU_reinterpret:
	  /* XXX Check size.  */
	  get_uleb128 (newloc->number, data);
	  break;

	case DW_OP_consts:
	case DW_OP_breg0 ... DW_OP_breg31:
	case DW_OP_fbreg:
	  /* XXX Check size.  */
	  get_sleb128 (newloc->number, data);
	  break;

	case DW_OP_bregx:
	  /* XXX Check size.  */
	  get_uleb128 (newloc->number, data);
	  get_sleb128 (newloc->number2, data);
	  break;

	case DW_OP_bit_piece:
	case DW_OP_GNU_regval_type:
	  /* XXX Check size.  */
	  get_uleb128 (newloc->number, data);
	  get_uleb128 (newloc->number2, data);
	  break;

	case DW_OP_implicit_value:
	case DW_OP_GNU_entry_value:
	  /* This cannot be used in a CFI expression.  */
	  if (unlikely (dbg == NULL))
	    goto invalid;

	  /* start of block inc. len.  */
	  newloc->number2 = (Dwarf_Word) (uintptr_t) data;
	  /* XXX Check size.  */
	  get_uleb128 (newloc->number, data); /* Block length.  */
	  if (unlikely ((Dwarf_Word) (end_data - data) < newloc->number))
	    goto invalid;
	  data += newloc->number;		/* Skip the block.  */
	  break;

	case DW_OP_GNU_implicit_pointer:
	  /* DW_FORM_ref_addr, depends on offset size of CU.  */
	  if (__libdw_read_offset_inc (dbg, sec_index, &data, ref_size,
				       &newloc->number, IDX_debug_info, 0))
	    return -1;
	  /* XXX Check size.  */
	  get_uleb128 (newloc->number2, data); /* Byte offset.  */
	  break;

	case DW_OP_GNU_deref_type:
	  if (unlikely (data >= end_data))
	    goto invalid;
	  newloc->number = *data++;
	  get_uleb128 (newloc->number2, data);
	  break;

	case DW_OP_GNU_const_type:
	  {
	    size_t size;

	    /* XXX Check size.  */
	    get_uleb128 (newloc->number, data);
	    if (unlikely (data >= end_data))
	      goto invalid;

	    /* start of block inc. len.  */
	    newloc->number2 = (Dwarf_Word) (uintptr_t) data;
	    size = *data++;
	    if (unlikely ((Dwarf_Word) (end_data - data) < size))
	      goto invalid;
	    data += size;		/* Skip the block.  */
	  }
	  break;

	default:
	  goto invalid;
	}
    }

  if (unlikely (n == 0))
    {
      /* This is not allowed.
	 It would mean an empty location expression, which we handled
	 already as a special case above.  */
      goto invalid;
    }

  if (valuep)
    {
      struct loclist *newloc;
      newloc = (struct loclist *) alloca (sizeof (struct loclist));
      newloc->atom = DW_OP_stack_value;
      newloc->number = 0;
      newloc->number2 = 0;
      newloc->offset = data - block->data;
      newloc->next = loclist;
      loclist = newloc;
      ++n;
    }

  /* Allocate the array.  */
  Dwarf_Op *result;
  if (dbg != NULL)
    result = libdw_alloc (dbg, Dwarf_Op, sizeof (Dwarf_Op), n);
  else
    {
      result = malloc (sizeof *result * n);
      if (result == NULL)
	{
	nomem:
	  __libdw_seterrno (DWARF_E_NOMEM);
	  return -1;
	}
    }

  /* Store the result.  */
  *llbuf = result;
  *listlen = n;

  do
    {
      /* We populate the array from the back since the list is backwards.  */
      --n;
      result[n].atom = loclist->atom;
      result[n].number = loclist->number;
      result[n].number2 = loclist->number2;
      result[n].offset = loclist->offset;

      if (result[n].atom == DW_OP_implicit_value)
	store_implicit_value (dbg, cache, &result[n]);

      loclist = loclist->next;
    }
  while (n > 0);

  /* Insert a record in the search tree so that we can find it again later.  */
  struct loc_s *newp;
  if (dbg != NULL)
    newp = libdw_alloc (dbg, struct loc_s, sizeof (struct loc_s), 1);
  else
    {
      newp = malloc (sizeof *newp);
      if (newp == NULL)
	{
	  free (result);
	  goto nomem;
	}
    }

  newp->addr = block->data;
  newp->loc = result;
  newp->nloc = *listlen;
  (void) tsearch (newp, cache, loc_compare);

  /* We did it.  */
  return 0;
}

static int
getlocation (struct Dwarf_CU *cu, const Dwarf_Block *block,
	     Dwarf_Op **llbuf, size_t *listlen, int sec_index)
{
  return __libdw_intern_expression (cu->dbg, cu->dbg->other_byte_order,
				    cu->address_size, (cu->version == 2
						       ? cu->address_size
						       : cu->offset_size),
				    &cu->locs, block,
				    false, false,
				    llbuf, listlen, sec_index);
}

int
dwarf_getlocation (attr, llbuf, listlen)
     Dwarf_Attribute *attr;
     Dwarf_Op **llbuf;
     size_t *listlen;
{
  if (! attr_ok (attr))
    return -1;

  int result = check_constant_offset (attr, llbuf, listlen);
  if (result != 1)
    return result;

  /* If it has a block form, it's a single location expression.  */
  Dwarf_Block block;
  if (INTUSE(dwarf_formblock) (attr, &block) != 0)
    return -1;

  return getlocation (attr->cu, &block, llbuf, listlen, cu_sec_idx (attr->cu));
}

static int
attr_base_address (attr, basep)
     Dwarf_Attribute *attr;
     Dwarf_Addr *basep;
{
  /* Fetch the CU's base address.  */
  Dwarf_Die cudie = CUDIE (attr->cu);

  /* Find the base address of the compilation unit.  It will
     normally be specified by DW_AT_low_pc.  In DWARF-3 draft 4,
     the base address could be overridden by DW_AT_entry_pc.  It's
     been removed, but GCC emits DW_AT_entry_pc and not DW_AT_lowpc
     for compilation units with discontinuous ranges.  */
  Dwarf_Attribute attr_mem;
  if (unlikely (INTUSE(dwarf_lowpc) (&cudie, basep) != 0)
      && INTUSE(dwarf_formaddr) (INTUSE(dwarf_attr) (&cudie,
						     DW_AT_entry_pc,
						     &attr_mem),
				 basep) != 0)
    {
      if (INTUSE(dwarf_errno) () != 0)
	return -1;

      /* The compiler provided no base address when it should
	 have.  Buggy GCC does this when it used absolute
	 addresses in the location list and no DW_AT_ranges.  */
      *basep = 0;
    }
  return 0;
}

static int
initial_offset_base (attr, offset, basep)
     Dwarf_Attribute *attr;
     ptrdiff_t *offset;
     Dwarf_Addr *basep;
{
  if (attr_base_address (attr, basep) != 0)
    return -1;

  Dwarf_Word start_offset;
  if (__libdw_formptr (attr, IDX_debug_loc,
		       DWARF_E_NO_LOCLIST,
		       NULL, &start_offset) == NULL)
    return -1;

  *offset = start_offset;
  return 0;
}

static ptrdiff_t
getlocations_addr (attr, offset, basep, startp, endp, address,
		   locs, expr, exprlen)
     Dwarf_Attribute *attr;
     ptrdiff_t offset;
     Dwarf_Addr *basep;
     Dwarf_Addr *startp;
     Dwarf_Addr *endp;
     Dwarf_Addr address;
     Elf_Data *locs;
     Dwarf_Op **expr;
     size_t *exprlen;
{
  unsigned char *readp = locs->d_buf + offset;
  unsigned char *readendp = locs->d_buf + locs->d_size;

 next:
  if (readendp - readp < attr->cu->address_size * 2)
    {
    invalid:
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return -1;
    }

  Dwarf_Addr begin;
  Dwarf_Addr end;

  switch (__libdw_read_begin_end_pair_inc (attr->cu->dbg, IDX_debug_loc,
					   &readp, attr->cu->address_size,
					   &begin, &end, basep))
    {
    case 0: /* got location range. */
      break;
    case 1: /* base address setup. */
      goto next;
    case 2: /* end of loclist */
      return 0;
    default: /* error */
      return -1;
    }

  if (readendp - readp < 2)
    goto invalid;

  /* We have a location expression.  */
  Dwarf_Block block;
  block.length = read_2ubyte_unaligned_inc (attr->cu->dbg, readp);
  block.data = readp;
  if (readendp - readp < (ptrdiff_t) block.length)
    goto invalid;
  readp += block.length;

  *startp = *basep + begin;
  *endp = *basep + end;

  /* If address is minus one we want them all, otherwise only matching.  */
  if (address != (Dwarf_Word) -1 && (address < *startp || address >= *endp))
    goto next;

  if (getlocation (attr->cu, &block, expr, exprlen, IDX_debug_loc) != 0)
    return -1;

  return readp - (unsigned char *) locs->d_buf;
}

int
dwarf_getlocation_addr (attr, address, llbufs, listlens, maxlocs)
     Dwarf_Attribute *attr;
     Dwarf_Addr address;
     Dwarf_Op **llbufs;
     size_t *listlens;
     size_t maxlocs;
{
  if (! attr_ok (attr))
    return -1;

  if (llbufs == NULL)
    maxlocs = SIZE_MAX;

  /* If it has a block form, it's a single location expression.  */
  Dwarf_Block block;
  if (INTUSE(dwarf_formblock) (attr, &block) == 0)
    {
      if (maxlocs == 0)
	return 0;
      if (llbufs != NULL &&
	  getlocation (attr->cu, &block, &llbufs[0], &listlens[0],
		       cu_sec_idx (attr->cu)) != 0)
	return -1;
      return listlens[0] == 0 ? 0 : 1;
    }

  int error = INTUSE(dwarf_errno) ();
  if (unlikely (error != DWARF_E_NO_BLOCK))
    {
      __libdw_seterrno (error);
      return -1;
    }

  int result = check_constant_offset (attr, &llbufs[0], &listlens[0]);
  if (result != 1)
    return result ?: 1;

  Dwarf_Addr base, start, end;
  Dwarf_Op *expr;
  size_t expr_len;
  ptrdiff_t off = 0;
  size_t got = 0;

  /* This is a true loclistptr, fetch the initial base address and offset.  */
  if (initial_offset_base (attr, &off, &base) != 0)
    return -1;

  const Elf_Data *d = attr->cu->dbg->sectiondata[IDX_debug_loc];
  if (d == NULL)
    {
      __libdw_seterrno (DWARF_E_NO_LOCLIST);
      return -1;
    }

  while (got < maxlocs
         && (off = getlocations_addr (attr, off, &base, &start, &end,
				   address, d, &expr, &expr_len)) > 0)
    {
      /* This one matches the address.  */
      if (llbufs != NULL)
	{
	  llbufs[got] = expr;
	  listlens[got] = expr_len;
	}
      ++got;
    }

  /* We might stop early, so off can be zero or positive on success.  */
  if (off < 0)
    return -1;

  return got;
}

ptrdiff_t
dwarf_getlocations (attr, offset, basep, startp, endp, expr, exprlen)
     Dwarf_Attribute *attr;
     ptrdiff_t offset;
     Dwarf_Addr *basep;
     Dwarf_Addr *startp;
     Dwarf_Addr *endp;
     Dwarf_Op **expr;
     size_t *exprlen;
{
  if (! attr_ok (attr))
    return -1;

  /* 1 is an invalid offset, meaning no more locations. */
  if (offset == 1)
    return 0;

  if (offset == 0)
    {
      /* If it has a block form, it's a single location expression.  */
      Dwarf_Block block;
      if (INTUSE(dwarf_formblock) (attr, &block) == 0)
	{
	  if (getlocation (attr->cu, &block, expr, exprlen,
			   cu_sec_idx (attr->cu)) != 0)
	    return -1;

	  /* This is the one and only location covering everything. */
	  *startp = 0;
	  *endp = -1;
	  return 1;
	}

      int error = INTUSE(dwarf_errno) ();
      if (unlikely (error != DWARF_E_NO_BLOCK))
	{
	  __libdw_seterrno (error);
	  return -1;
	}

      int result = check_constant_offset (attr, expr, exprlen);
      if (result != 1)
	{
	  if (result == 0)
	    {
	      /* This is the one and only location covering everything. */
	      *startp = 0;
	      *endp = -1;
	      return 1;
	    }
	  return result;
	}

      /* We must be looking at a true loclistptr, fetch the initial
	 base address and offset.  */
      if (initial_offset_base (attr, &offset, basep) != 0)
	return -1;
    }

  const Elf_Data *d = attr->cu->dbg->sectiondata[IDX_debug_loc];
  if (d == NULL)
    {
      __libdw_seterrno (DWARF_E_NO_LOCLIST);
      return -1;
    }

  return getlocations_addr (attr, offset, basep, startp, endp,
			    (Dwarf_Word) -1, d, expr, exprlen);
}
