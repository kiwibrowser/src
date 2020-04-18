/* Routines required for instrumenting a program.  */
/* Compile this one with gcc.  */
/* Copyright (C) 1989-2014 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

#include "libgcov.h"

#if defined(inhibit_libc)
/* If libc and its header files are not available, provide dummy functions.  */

#ifdef L_gcov_merge_add
void __gcov_merge_add (gcov_type *counters  __attribute__ ((unused)),
                       unsigned n_counters __attribute__ ((unused))) {}
#endif

#ifdef L_gcov_merge_single
void __gcov_merge_single (gcov_type *counters  __attribute__ ((unused)),
                          unsigned n_counters __attribute__ ((unused))) {}
#endif

#ifdef L_gcov_merge_delta
void __gcov_merge_delta (gcov_type *counters  __attribute__ ((unused)),
                         unsigned n_counters __attribute__ ((unused))) {}
#endif

#else

#ifdef L_gcov_merge_add
/* The profile merging function that just adds the counters.  It is given
   an array COUNTERS of N_COUNTERS old counters and it reads the same number
   of counters from the gcov file.  */
void
__gcov_merge_add (gcov_type *counters, unsigned n_counters)
{
  for (; n_counters; counters++, n_counters--)
    *counters += gcov_get_counter ();
}
#endif /* L_gcov_merge_add */

#ifdef L_gcov_merge_ior
/* The profile merging function that just adds the counters.  It is given
   an array COUNTERS of N_COUNTERS old counters and it reads the same number
   of counters from the gcov file.  */
void
__gcov_merge_ior (gcov_type *counters, unsigned n_counters)
{
  for (; n_counters; counters++, n_counters--)
    *counters |= gcov_get_counter_target ();
}
#endif


#ifdef L_gcov_merge_dc

/* Returns 1 if the function global id GID is not valid.  */

static int
__gcov_is_gid_insane (gcov_type gid)
{
  if (EXTRACT_MODULE_ID_FROM_GLOBAL_ID (gid) == 0
      || EXTRACT_FUNC_ID_FROM_GLOBAL_ID (gid) == 0)
    return 1;
  return 0;
}

/* The profile merging function used for merging direct call counts
   This function is given array COUNTERS of N_COUNTERS old counters and it
   reads the same number of counters from the gcov file.  */

void
__gcov_merge_dc (gcov_type *counters, unsigned n_counters)
{
  unsigned i;

  gcc_assert (!(n_counters % 2));
  for (i = 0; i < n_counters; i += 2)
    {
      gcov_type global_id = gcov_get_counter_target ();
      gcov_type call_count = gcov_get_counter ();

      /* Note that global id counter may never have been set if no calls were
	 made from this call-site.  */
      if (counters[i] && global_id)
        {
          /* TODO race condition requires us do the following correction.  */
          if (__gcov_is_gid_insane (counters[i]))
            counters[i] = global_id;
          else if (__gcov_is_gid_insane (global_id))
            global_id = counters[i];

#if !defined(__KERNEL__)
          /* In the case of inconsistency, use the src's target.  */
          if (counters[i] != global_id)
            fprintf (stderr, "Warning: Inconsistent call targets in"
                     " direct-call profile.\n");
#endif
        }
      else if (global_id)
	counters[i] = global_id;

      counters[i + 1] += call_count;

      /* Reset. */
      if (__gcov_is_gid_insane (counters[i]))
        counters[i] = counters[i + 1] = 0;

      /* Assert that the invariant (global_id == 0) <==> (call_count == 0)
	 holds true after merging.  */
      if (counters[i] == 0)
        counters[i+1] = 0;
      if (counters[i + 1] == 0)
        counters[i] = 0;
    }
}
#endif


#ifdef L_gcov_merge_icall_topn
/* The profile merging function used for merging indirect call counts
   This function is given array COUNTERS of N_COUNTERS old counters and it
   reads the same number of counters from the gcov file.  */

void
__gcov_merge_icall_topn (gcov_type *counters, unsigned n_counters)
{
  unsigned i, j, k, m;

  gcc_assert (!(n_counters % GCOV_ICALL_TOPN_NCOUNTS));
  for (i = 0; i < n_counters; i += GCOV_ICALL_TOPN_NCOUNTS)
    {
      gcov_type *value_array = &counters[i + 1];
      unsigned tmp_size = 2 * (GCOV_ICALL_TOPN_NCOUNTS - 1);
      gcov_type *tmp_array 
          = (gcov_type *) alloca (tmp_size * sizeof (gcov_type));

      for (j = 0; j < tmp_size; j++)
        tmp_array[j] = 0;

      for (j = 0; j < GCOV_ICALL_TOPN_NCOUNTS - 1; j += 2)
        {
          tmp_array[j] = value_array[j];
          tmp_array[j + 1] = value_array [j + 1];
        }

      /* Skip the number_of_eviction entry.  */
      gcov_get_counter ();
      for (k = 0; k < GCOV_ICALL_TOPN_NCOUNTS - 1; k += 2)
        {
          int found = 0;
          gcov_type global_id = gcov_get_counter_target ();
          gcov_type call_count = gcov_get_counter ();
          for (m = 0; m < j; m += 2)
            {
              if (tmp_array[m] == global_id)
                {
                  found = 1;
                  tmp_array[m + 1] += call_count;
                  break;
                }
            }
          if (!found)
            {
              tmp_array[j] = global_id;
              tmp_array[j + 1] = call_count;
              j += 2;
            }
        }
      /* Now sort the temp array */
      gcov_sort_n_vals (tmp_array, j);

      /* Now copy back the top half of the temp array */
      for (k = 0; k < GCOV_ICALL_TOPN_NCOUNTS - 1; k += 2)
        {
          value_array[k] = tmp_array[k];
          value_array[k + 1] = tmp_array[k + 1];
        }
    }
}
#endif


#ifdef L_gcov_merge_time_profile
/* Time profiles are merged so that minimum from all valid (greater than zero)
   is stored. There could be a fork that creates new counters. To have
   the profile stable, we chosen to pick the smallest function visit time.  */
void
__gcov_merge_time_profile (gcov_type *counters, unsigned n_counters)
{
  unsigned int i;
  gcov_type value;

  for (i = 0; i < n_counters; i++)
    {
      value = gcov_get_counter_target ();

      if (value && (!counters[i] || value < counters[i]))
        counters[i] = value;
    }
}
#endif /* L_gcov_merge_time_profile */

#ifdef L_gcov_merge_single
/* The profile merging function for choosing the most common value.
   It is given an array COUNTERS of N_COUNTERS old counters and it
   reads the same number of counters from the gcov file.  The counters
   are split into 3-tuples where the members of the tuple have
   meanings:

   -- the stored candidate on the most common value of the measured entity
   -- counter
   -- total number of evaluations of the value  */
void
__gcov_merge_single (gcov_type *counters, unsigned n_counters)
{
  unsigned i, n_measures;
  gcov_type value, counter, all;

  gcc_assert (!(n_counters % 3));
  n_measures = n_counters / 3;
  for (i = 0; i < n_measures; i++, counters += 3)
    {
      value = gcov_get_counter_target ();
      counter = gcov_get_counter ();
      all = gcov_get_counter ();

      if (counters[0] == value)
        counters[1] += counter;
      else if (counter > counters[1])
        {
          counters[0] = value;
          counters[1] = counter - counters[1];
        }
      else
        counters[1] -= counter;
      counters[2] += all;
    }
}
#endif /* L_gcov_merge_single */

#ifdef L_gcov_merge_delta
/* The profile merging function for choosing the most common
   difference between two consecutive evaluations of the value.  It is
   given an array COUNTERS of N_COUNTERS old counters and it reads the
   same number of counters from the gcov file.  The counters are split
   into 4-tuples where the members of the tuple have meanings:

   -- the last value of the measured entity
   -- the stored candidate on the most common difference
   -- counter
   -- total number of evaluations of the value  */
void
__gcov_merge_delta (gcov_type *counters, unsigned n_counters)
{
  unsigned i, n_measures;
  gcov_type value, counter, all;

  gcc_assert (!(n_counters % 4));
  n_measures = n_counters / 4;
  for (i = 0; i < n_measures; i++, counters += 4)
    {
      /* last = */ gcov_get_counter ();
      value = gcov_get_counter_target ();
      counter = gcov_get_counter ();
      all = gcov_get_counter ();

      if (counters[1] == value)
        counters[2] += counter;
      else if (counter > counters[2])
        {
          counters[1] = value;
          counters[2] = counter - counters[2];
        }
      else
        counters[2] -= counter;
      counters[3] += all;
    }
}
#endif /* L_gcov_merge_delta */
#endif /* inhibit_libc */
