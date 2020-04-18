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

#if defined(L_gcov)
void __gcov_init (struct gcov_info *p __attribute__ ((unused))) {}
#endif

#else /* inhibit_libc */

#if !defined(__KERNEL__)
#include <string.h>
#if GCOV_LOCKED
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#endif
#endif /* __KERNEL__ */

#ifdef L_gcov
#include "gcov-io.c"

/* Unique identifier assigned to each module (object file).  */
static gcov_unsigned_t gcov_cur_module_id = 0;


/* Dynamic call graph build and form module groups.  */
int __gcov_compute_module_groups (char **zero_counts) ATTRIBUTE_HIDDEN;
void __gcov_finalize_dyn_callgraph (void) ATTRIBUTE_HIDDEN;

/* The following functions can be called from outside of this file.  */
extern void gcov_clear (void) ATTRIBUTE_HIDDEN;
extern void gcov_exit (void) ATTRIBUTE_HIDDEN;
extern void set_gcov_dump_complete (void) ATTRIBUTE_HIDDEN;
extern void reset_gcov_dump_complete (void) ATTRIBUTE_HIDDEN;
extern int get_gcov_dump_complete (void) ATTRIBUTE_HIDDEN;
extern void set_gcov_list (struct gcov_info *) ATTRIBUTE_HIDDEN;
__attribute__((weak)) void __coverage_callback (gcov_type, int); 

#if !defined(IN_GCOV_TOOL) && !defined(__KERNEL__)
extern gcov_unsigned_t __gcov_sampling_period;
extern gcov_unsigned_t __gcov_has_sampling;
static int gcov_sampling_period_initialized = 0;

/* Create a strong reference to these symbols so that they are
   unconditionally pulled into the instrumented binary, even when
   the only reference is a weak reference. This is necessary because
   we are using weak references to enable references from code that
   may not be linked with libgcov. These are the only symbols that
   should be accessed via link references from application code!

   A subtlety of the linker is that it will only resolve weak references
   defined within archive libraries when there is a strong reference to
   something else defined within the same object file. Since these functions
   are defined within their own object files, they would not automatically
   get resolved. Since there are symbols within the main L_gcov
   section that are strongly referenced during -fprofile-generate and
   -ftest-coverage builds, these dummy symbols will always need to be
   resolved.  */
void (*__gcov_dummy_ref1)(void) = &__gcov_reset;
void (*__gcov_dummy_ref2)(void) = &__gcov_dump;
extern char *__gcov_get_profile_prefix (void);
char *(*__gcov_dummy_ref3)(void) = &__gcov_get_profile_prefix;
extern void __gcov_set_sampling_period (unsigned int period);
char *(*__gcov_dummy_ref4)(void) = &__gcov_set_sampling_period;
extern unsigned int __gcov_sampling_enabled (void);
char *(*__gcov_dummy_ref5)(void) = &__gcov_sampling_enabled;
extern void __gcov_flush (void);
char *(*__gcov_dummy_ref6)(void) = &__gcov_flush;
extern unsigned int __gcov_profiling_for_test_coverage (void);
char *(*__gcov_dummy_ref7)(void) = &__gcov_profiling_for_test_coverage;
#endif

/* Default callback function for profile instrumentation callback.  */
__attribute__((weak)) void
__coverage_callback (gcov_type funcdef_no __attribute__ ((unused)),
                     int edge_no __attribute__ ((unused)))
{
   /* nothing */
}

struct gcov_fn_buffer
{
  struct gcov_fn_buffer *next;
  unsigned fn_ix;
  struct gcov_fn_info info;
  /* note gcov_fn_info ends in a trailing array.  */
};

struct gcov_summary_buffer
{
  struct gcov_summary_buffer *next;
  struct gcov_summary summary;
};

/* Chain of per-object gcov structures.  */
extern struct gcov_info *__gcov_list;

/* Set the head of gcov_list.  */
void
set_gcov_list (struct gcov_info *head)
{
  __gcov_list = head;
}

/* Flag if the current function being read was marked as having fixed-up
   zero counters.  */
static int __gcov_curr_fn_fixed_up;

/* Set function fixed up flag.  */
void
set_gcov_fn_fixed_up (int fixed_up)
{
  __gcov_curr_fn_fixed_up = fixed_up;
}

/* Return function fixed up flag.  */
int
get_gcov_fn_fixed_up (void)
{
  return __gcov_curr_fn_fixed_up;
}

/* Size of the longest file name. */
/* We need to expose this static variable when compiling for gcov-tool.  */
#ifndef IN_GCOV_TOOL
static
#endif
size_t gcov_max_filename = 0;

/* Flag when the profile has already been dumped via __gcov_dump().  */
static int gcov_dump_complete;

/* A global function that get the vaule of gcov_dump_complete.  */

int
get_gcov_dump_complete (void)
{
  return gcov_dump_complete;
}

/* A global functino that set the vaule of gcov_dump_complete. Will
   be used in __gcov_dump() in libgcov-interface.c.  */

void
set_gcov_dump_complete (void)
{
  gcov_dump_complete = 1;
}

/* A global functino that set the vaule of gcov_dump_complete. Will
   be used in __gcov_reset() in libgcov-interface.c.  */

void
reset_gcov_dump_complete (void)
{
  gcov_dump_complete = 0;
}

/* A utility function for outputing errors.  */
static int gcov_error (const char *, ...);

static struct gcov_fn_buffer *
free_fn_data (const struct gcov_info *gi_ptr, struct gcov_fn_buffer *buffer,
              unsigned limit)
{
  struct gcov_fn_buffer *next;
  unsigned ix, n_ctr = 0;

  if (!buffer)
    return 0;
  next = buffer->next;

  for (ix = 0; ix != limit; ix++)
    if (gi_ptr->merge[ix])
      xfree (buffer->info.ctrs[n_ctr++].values);
  xfree (buffer);
  return next;
}

static struct gcov_fn_buffer **
buffer_fn_data (const char *filename, const struct gcov_info *gi_ptr,
                struct gcov_fn_buffer **end_ptr, unsigned fn_ix)
{
  unsigned n_ctrs = 0, ix = 0;
  struct gcov_fn_buffer *fn_buffer;
  unsigned len;

  for (ix = GCOV_COUNTERS; ix--;)
    if (gi_ptr->merge[ix])
      n_ctrs++;

  len = sizeof (*fn_buffer) + sizeof (fn_buffer->info.ctrs[0]) * n_ctrs;
  fn_buffer = (struct gcov_fn_buffer *) xmalloc (len);

  if (!fn_buffer)
    goto fail;

  fn_buffer->next = 0;
  fn_buffer->fn_ix = fn_ix;
  fn_buffer->info.ident = gcov_read_unsigned ();
  fn_buffer->info.lineno_checksum = gcov_read_unsigned ();
  fn_buffer->info.cfg_checksum = gcov_read_unsigned ();

  for (n_ctrs = ix = 0; ix != GCOV_COUNTERS; ix++)
    {
      gcov_unsigned_t length;
      gcov_type *values;

      if (!gi_ptr->merge[ix])
        continue;

      if (gcov_read_unsigned () != GCOV_TAG_FOR_COUNTER (ix))
        {
          len = 0;
          goto fail;
        }

      length = GCOV_TAG_COUNTER_NUM (gcov_read_unsigned ());
      len = length * sizeof (gcov_type);
      values = (gcov_type *) xmalloc (len);
      if (!values)
        goto fail;

      fn_buffer->info.ctrs[n_ctrs].num = length;
      fn_buffer->info.ctrs[n_ctrs].values = values;

      while (length--)
        *values++ = gcov_read_counter ();
      n_ctrs++;
    }

  *end_ptr = fn_buffer;
  return &fn_buffer->next;

fail:
  gcov_error ("profiling:%s:Function %u %s %u \n", filename, fn_ix,
              len ? "cannot allocate" : "counter mismatch", len ? len : ix);

  return (struct gcov_fn_buffer **)free_fn_data (gi_ptr, fn_buffer, ix);
}

/* Determine whether a counter is active.  */

static inline int
gcov_counter_active (const struct gcov_info *info, unsigned int type)
{
  return (info->merge[type] != 0);
}

/* Add an unsigned value to the current crc */

static gcov_unsigned_t
crc32_unsigned (gcov_unsigned_t crc32, gcov_unsigned_t value)
{
  unsigned ix;

  for (ix = 32; ix--; value <<= 1)
    {
      unsigned feedback;

      feedback = (value ^ crc32) & 0x80000000 ? 0x04c11db7 : 0;
      crc32 <<= 1;
      crc32 ^= feedback;
    }

  return crc32;
}

/* Check if VERSION of the info block PTR matches libgcov one.
   Return 1 on success, or zero in case of versions mismatch.
   If FILENAME is not NULL, its value used for reporting purposes
   instead of value from the info block.  */

static int
gcov_version (struct gcov_info *ptr, gcov_unsigned_t version,
              const char *filename)
{
  if (version != GCOV_VERSION)
    {
      char v[4], e[4];

      GCOV_UNSIGNED2STRING (v, version);
      GCOV_UNSIGNED2STRING (e, GCOV_VERSION);

      if (filename)
        gcov_error ("profiling:%s:Version mismatch - expected %.4s got %.4s\n",
                   filename? filename : ptr->filename, e, v);
      else
        gcov_error ("profiling:Version mismatch - expected %.4s got %.4s\n", e, v);

      return 0;
    }
  return 1;
}

/* Insert counter VALUE into HISTOGRAM.  */

static void
gcov_histogram_insert(gcov_bucket_type *histogram, gcov_type value)
{
  unsigned i;

  i = gcov_histo_index(value);
  histogram[i].num_counters++;
  histogram[i].cum_value += value;
  if (value < histogram[i].min_value)
    histogram[i].min_value = value;
}

/* Computes a histogram of the arc counters to place in the summary SUM.  */

static void
gcov_compute_histogram (struct gcov_summary *sum)
{
  struct gcov_info *gi_ptr;
  const struct gcov_fn_info *gfi_ptr;
  const struct gcov_ctr_info *ci_ptr;
  struct gcov_ctr_summary *cs_ptr;
  unsigned t_ix, f_ix, ctr_info_ix, ix;
  int h_ix;

  /* This currently only applies to arc counters.  */
  t_ix = GCOV_COUNTER_ARCS;

  /* First check if there are any counts recorded for this counter.  */
  cs_ptr = &(sum->ctrs[t_ix]);
  if (!cs_ptr->num)
    return;

  for (h_ix = 0; h_ix < GCOV_HISTOGRAM_SIZE; h_ix++)
    {
      cs_ptr->histogram[h_ix].num_counters = 0;
      cs_ptr->histogram[h_ix].min_value = cs_ptr->run_max;
      cs_ptr->histogram[h_ix].cum_value = 0;
    }

  /* Walk through all the per-object structures and record each of
     the count values in histogram.  */
  for (gi_ptr = __gcov_list; gi_ptr; gi_ptr = gi_ptr->next)
    {
      if (!gi_ptr->merge[t_ix])
        continue;

      /* Find the appropriate index into the gcov_ctr_info array
         for the counter we are currently working on based on the
         existence of the merge function pointer for this object.  */
      for (ix = 0, ctr_info_ix = 0; ix < t_ix; ix++)
        {
          if (gi_ptr->merge[ix])
            ctr_info_ix++;
        }
      for (f_ix = 0; f_ix != gi_ptr->n_functions; f_ix++)
        {
          gfi_ptr = gi_ptr->functions[f_ix];

          if (!gfi_ptr || gfi_ptr->key != gi_ptr)
            continue;

          ci_ptr = &gfi_ptr->ctrs[ctr_info_ix];
          for (ix = 0; ix < ci_ptr->num; ix++)
            gcov_histogram_insert (cs_ptr->histogram, ci_ptr->values[ix]);
        }
    }
}

/* gcda filename.  */
static char *gi_filename;
/* buffer for the fn_data from another program.  */
static struct gcov_fn_buffer *fn_buffer;
/* buffer for summary from other programs to be written out. */
static struct gcov_summary_buffer *sum_buffer;
/* If application calls fork or exec multiple times, we end up storing
   profile repeadely.  We should not account this as multiple runs or
   functions executed once may mistakely become cold.  */
static int run_accounted = 0;

/* This funtions computes the program level summary and the histo-gram.
   It computes and returns CRC32 and stored summary in THIS_PRG.  */

#if !IN_GCOV_TOOL
static 
#endif
gcov_unsigned_t
gcov_exit_compute_summary (struct gcov_summary *this_prg)
{
  struct gcov_info *gi_ptr;
  const struct gcov_fn_info *gfi_ptr;
  struct gcov_ctr_summary *cs_ptr;
  const struct gcov_ctr_info *ci_ptr;
  int f_ix;
  unsigned t_ix;
  gcov_unsigned_t c_num;
  gcov_unsigned_t crc32 = 0;

  /* Find the totals for this execution.  */
  memset (this_prg, 0, sizeof (*this_prg));
  for (gi_ptr = __gcov_list; gi_ptr; gi_ptr = gi_ptr->next)
    {
      crc32 = crc32_unsigned (crc32, gi_ptr->stamp);
      crc32 = crc32_unsigned (crc32, gi_ptr->n_functions);

      for (f_ix = 0; (unsigned)f_ix != gi_ptr->n_functions; f_ix++)
        {
          gfi_ptr = gi_ptr->functions[f_ix];

          if (gfi_ptr && gfi_ptr->key != gi_ptr)
            gfi_ptr = 0;

          crc32 = crc32_unsigned (crc32, gfi_ptr ? gfi_ptr->cfg_checksum : 0);
          crc32 = crc32_unsigned (crc32,
                                  gfi_ptr ? gfi_ptr->lineno_checksum : 0);
          if (!gfi_ptr)
            continue;

          ci_ptr = gfi_ptr->ctrs;
          for (t_ix = 0; t_ix != GCOV_COUNTERS_SUMMABLE; t_ix++)
            {
              if (!gi_ptr->merge[t_ix])
                continue;

              cs_ptr = &(this_prg->ctrs[t_ix]);
              cs_ptr->num += ci_ptr->num;
              crc32 = crc32_unsigned (crc32, ci_ptr->num);

              for (c_num = 0; c_num < ci_ptr->num; c_num++)
                {
                  cs_ptr->sum_all += ci_ptr->values[c_num];
                  if (cs_ptr->run_max < ci_ptr->values[c_num])
                    cs_ptr->run_max = ci_ptr->values[c_num];
                }
              ci_ptr++;
            }
        }
    }
  gcov_compute_histogram (this_prg);
  return crc32;
}

/* A struct that bundles all the related information about the
   gcda filename.  */
struct gcov_filename_aux{
  char *gi_filename_up;
  int gcov_prefix_strip;
  size_t prefix_length;
};

/* Including system dependent components. */
#if !defined (__KERNEL__)
#include "libgcov-driver-system.c"
#else
#include "libgcov-driver-kernel.c"
#endif

static int
scan_build_info (struct gcov_info *gi_ptr)
{
  gcov_unsigned_t i, length;
  gcov_unsigned_t num_strings = 0;
  char **build_info_strings;

  length = gcov_read_unsigned ();
  build_info_strings = gcov_read_build_info (length, &num_strings);
  if (!build_info_strings)
    {
      gcov_error ("profiling:%s:Error reading build info\n", gi_filename);
      return -1;
    }
  if (!gi_ptr->build_info)
    {
      gcov_error ("profiling:%s:Mismatched build info sections, expected "
                  "none, found %u strings)\n", gi_filename, num_strings);
      return -1;
    }

  for (i = 0; i < num_strings; i++)
    {
      if (strcmp (build_info_strings[i], gi_ptr->build_info[i]))
        {
          gcov_error ("profiling:%s:Mismatched build info string "
                      "(expected %s, read %s)\n",
                      gi_filename, gi_ptr->build_info[i],
                      build_info_strings[i]);
          return -1;
        }
      xfree (build_info_strings[i]);
    }
  xfree (build_info_strings);
  return 0;
}

#if !defined(__KERNEL__)
/* Scan through the current open gcda file corresponding to GI_PTR
   to locate the end position just before function data should be rewritten,
   returned in SUMMARY_END_POS_P. E.g. scan past the last summary and other
   sections that won't be rewritten, like the build info.  Return 0 on success,
   -1 on error.  */
static int
gcov_scan_to_function_data (struct gcov_info *gi_ptr,
                            gcov_position_t *summary_end_pos_p)
{
  gcov_unsigned_t tag, version, stamp;
  tag = gcov_read_unsigned ();
  if (tag != GCOV_DATA_MAGIC)
    {
      gcov_error ("profiling:%s:Not a gcov data file\n", gi_filename);
      return -1;
    }

  version = gcov_read_unsigned ();
  if (!gcov_version (gi_ptr, version, gi_filename))
    return -1;

  stamp = gcov_read_unsigned ();
  if (stamp != gi_ptr->stamp)
    /* Read from a different compilation.  Overwrite the file.  */
    return -1;

  /* Look for program summary.  */
  while (1)
    {
      struct gcov_summary tmp;

      *summary_end_pos_p = gcov_position ();
      tag = gcov_read_unsigned ();
      if (tag != GCOV_TAG_PROGRAM_SUMMARY)
        break;

      gcov_read_unsigned ();
      gcov_read_summary (&tmp);
      if (gcov_is_error ())
        return -1;
    }

  /* If there is a build info section, scan past it as well.  */
  if (tag == GCOV_TAG_BUILD_INFO)
    {
      if (scan_build_info (gi_ptr) < 0)
        return -1;

      *summary_end_pos_p = gcov_position ();
      tag = gcov_read_unsigned ();
    }
  /* The next section should be the function counters.  */
  gcc_assert (tag == GCOV_TAG_FUNCTION);

  return 0;
}
#endif /* __KERNEL__ */

/* This function merges counters in GI_PTR to an existing gcda file.
   Return 0 on success.
   Return -1 on error. In this case, caller will goto read_fatal.  */

static int
gcov_exit_merge_gcda (struct gcov_info *gi_ptr,
                      struct gcov_summary *prg_p,
                      struct gcov_summary *this_prg,
                      gcov_position_t *summary_pos_p,
                      gcov_position_t *eof_pos_p,
		      gcov_unsigned_t crc32)
{
  gcov_unsigned_t tag, length;
  unsigned t_ix;
  int f_ix;
  int error = 0;
  struct gcov_fn_buffer **fn_tail = &fn_buffer;
  struct gcov_summary_buffer **sum_tail = &sum_buffer;
  int *zero_fixup_flags = NULL;

  length = gcov_read_unsigned ();
  if (!gcov_version (gi_ptr, length, gi_filename))
    return -1;

  length = gcov_read_unsigned ();
  if (length != gi_ptr->stamp)
    /* Read from a different compilation. Overwrite the file.  */
    return 0;

  /* Look for program summary.  */
  for (f_ix = 0;;)
    {
      struct gcov_summary tmp;

      *eof_pos_p = gcov_position ();
      tag = gcov_read_unsigned ();
      if (tag != GCOV_TAG_PROGRAM_SUMMARY)
        break;

      f_ix--;
      length = gcov_read_unsigned ();
      gcov_read_summary (&tmp);
      if ((error = gcov_is_error ()))
        goto read_error;
      if (*summary_pos_p)
        {
          /* Save all summaries after the one that will be
             merged into below. These will need to be rewritten
             as histogram merging may change the number of non-zero
             histogram entries that will be emitted, and thus the
             size of the merged summary.  */
          (*sum_tail) = (struct gcov_summary_buffer *)
              xmalloc (sizeof(struct gcov_summary_buffer));
          (*sum_tail)->summary = tmp;
          (*sum_tail)->next = 0;
          sum_tail = &((*sum_tail)->next);
          goto next_summary;
        }
      if (tmp.checksum != crc32)
        goto next_summary;

      for (t_ix = 0; t_ix != GCOV_COUNTERS_SUMMABLE; t_ix++)
        if (tmp.ctrs[t_ix].num != this_prg->ctrs[t_ix].num)
          goto next_summary;
      *prg_p = tmp;
      *summary_pos_p = *eof_pos_p;

    next_summary:;
    }

  if (tag == GCOV_TAG_BUILD_INFO)
    {
      if (scan_build_info (gi_ptr) < 0)
        return -1;

      /* Since the stamps matched if we got here, this should be from
         the same compilation and the build info strings should match.  */
      tag = gcov_read_unsigned ();
    }

  if (tag == GCOV_TAG_COMDAT_ZERO_FIXUP)
    {
      gcov_unsigned_t num_fns = 0;
      length = gcov_read_unsigned ();
      zero_fixup_flags = gcov_read_comdat_zero_fixup (length, &num_fns);
      if (!zero_fixup_flags)
        {
          gcov_error ("profiling:%s:Error reading zero fixup flags\n",
                      gi_filename);
          return -1;
        }

      tag = gcov_read_unsigned ();
    }

  /* Merge execution counts for each function.  */
  for (f_ix = 0; (unsigned)f_ix != gi_ptr->n_functions;
       f_ix++, tag = gcov_read_unsigned ())
    {
      const struct gcov_ctr_info *ci_ptr;
      const struct gcov_fn_info *gfi_ptr = gi_ptr->functions[f_ix];

      if (tag != GCOV_TAG_FUNCTION)
        goto read_mismatch;

      length = gcov_read_unsigned ();
      if (!length)
        /* This function did not appear in the other program.
           We have nothing to merge.  */
        continue;

      if (length != GCOV_TAG_FUNCTION_LENGTH)
        goto read_mismatch;

      if (!gfi_ptr || gfi_ptr->key != gi_ptr)
        {
          /* This function appears in the other program.  We
             need to buffer the information in order to write
             it back out -- we'll be inserting data before
             this point, so cannot simply keep the data in the
             file.  */
          fn_tail = buffer_fn_data (gi_filename,
                                    gi_ptr, fn_tail, f_ix);
          if (!fn_tail)
            goto read_mismatch;
          continue;
        }

      if (zero_fixup_flags)
        set_gcov_fn_fixed_up (zero_fixup_flags[f_ix]);

      length = gcov_read_unsigned ();
      if (length != gfi_ptr->ident)
        goto read_mismatch;

      length = gcov_read_unsigned ();
      if (length != gfi_ptr->lineno_checksum)
        goto read_mismatch;

      length = gcov_read_unsigned ();
      if (length != gfi_ptr->cfg_checksum)
        goto read_mismatch;

      ci_ptr = gfi_ptr->ctrs;
      for (t_ix = 0; t_ix < GCOV_COUNTERS; t_ix++)
        {
          gcov_merge_fn merge = gi_ptr->merge[t_ix];

          if (!merge)
            continue;

          tag = gcov_read_unsigned ();
          length = gcov_read_unsigned ();
          if (tag != GCOV_TAG_FOR_COUNTER (t_ix)
              || length != GCOV_TAG_COUNTER_LENGTH (ci_ptr->num))
            goto read_mismatch;
          (*merge) (ci_ptr->values, ci_ptr->num);
          ci_ptr++;
        }
      if ((error = gcov_is_error ()))
        goto read_error;
    }
  xfree (zero_fixup_flags);

  if (tag && tag != GCOV_TAG_MODULE_INFO)
    {
    read_mismatch:;
      gcov_error ("profiling:%s:Merge mismatch for %s %u\n",
                  gi_filename, f_ix >= 0 ? "function" : "summary",
                  f_ix < 0 ? -1 - f_ix : f_ix);
      return -1;
    }
  return 0;

read_error:
  gcov_error ("profiling:%s:%s merging\n", gi_filename,
              error < 0 ? "Overflow": "Error");
  return -1;
}

#if !defined(__KERNEL__)
/* Write NUM_FNS ZERO_COUNTS fixup flags to a gcda file starting from its
   current location.  */

static void
gcov_write_comdat_zero_fixup (char *zero_counts, unsigned num_fns)
{
  unsigned f_ix;
  gcov_unsigned_t len = GCOV_TAG_COMDAT_ZERO_FIXUP_LENGTH (num_fns);
  gcov_unsigned_t bitvector = 0, b_ix = 0;
  gcov_write_tag_length (GCOV_TAG_COMDAT_ZERO_FIXUP, len);

  gcov_write_unsigned (num_fns);
  for (f_ix = 0; f_ix != num_fns; f_ix++)
    {
      if (zero_counts[f_ix])
        bitvector |= 1 << b_ix;
      if (++b_ix == 32)
        {
          gcov_write_unsigned (bitvector);
          b_ix = 0;
          bitvector = 0;
        }
    }
  if (b_ix > 0)
    gcov_write_unsigned (bitvector);
}
#endif /* __KERNEL__ */

/* Write build_info strings from GI_PTR to a gcda file starting from its current
   location.  */

static void
gcov_write_build_info (struct gcov_info *gi_ptr)
{
  gcov_unsigned_t num = 0;
  gcov_unsigned_t len = 1;

  if (!gi_ptr->build_info)
    return;

  /* Count the number of strings, which is terminated with an empty string.  */
  while (gi_ptr->build_info[num][0])
    num++;

  len += gcov_compute_string_array_len (gi_ptr->build_info, num);
  gcov_write_tag_length (GCOV_TAG_BUILD_INFO, len);
  gcov_write_unsigned (num);
  gcov_write_string_array (gi_ptr->build_info, num);
}

/* Write counters in GI_PTR to a gcda file starting from its current
   location.  */

static void
gcov_write_func_counters (struct gcov_info *gi_ptr)
{
  unsigned f_ix;

  /* Write execution counts for each function.  */
  for (f_ix = 0; f_ix != gi_ptr->n_functions; f_ix++)
    {
      unsigned buffered = 0;
      const struct gcov_fn_info *gfi_ptr;
      const struct gcov_ctr_info *ci_ptr;
      gcov_unsigned_t length;
      unsigned t_ix;

      if (fn_buffer && fn_buffer->fn_ix == f_ix)
        {
          /* Buffered data from another program.  */
          buffered = 1;
          gfi_ptr = &fn_buffer->info;
          length = GCOV_TAG_FUNCTION_LENGTH;
        }
      else
        {
          gfi_ptr = gi_ptr->functions[f_ix];
          if (gfi_ptr && gfi_ptr->key == gi_ptr)
            length = GCOV_TAG_FUNCTION_LENGTH;
          else
            length = 0;
        }

      gcov_write_tag_length (GCOV_TAG_FUNCTION, length);
      if (!length)
        continue;

      gcov_write_unsigned (gfi_ptr->ident);
      gcov_write_unsigned (gfi_ptr->lineno_checksum);
      gcov_write_unsigned (gfi_ptr->cfg_checksum);

      ci_ptr = gfi_ptr->ctrs;
      for (t_ix = 0; t_ix < GCOV_COUNTERS; t_ix++)
        {
          gcov_unsigned_t n_counts;
          gcov_type *c_ptr;

          if (!gi_ptr->merge[t_ix])
            continue;

          n_counts = ci_ptr->num;
          gcov_write_tag_length (GCOV_TAG_FOR_COUNTER (t_ix),
                                 GCOV_TAG_COUNTER_LENGTH (n_counts));
          c_ptr = ci_ptr->values;
          while (n_counts--)
            gcov_write_counter (*c_ptr++);
          ci_ptr++;
        }
#if !defined(__KERNEL__)
      if (buffered)
        fn_buffer = free_fn_data (gi_ptr, fn_buffer, GCOV_COUNTERS);
#endif /* __KERNEL__ */
    }

  gi_ptr->eof_pos = gcov_position ();
  gcov_write_unsigned (0);
}

/* Write counters in GI_PTR and the summary in PRG to a gcda file.  In
   the case of appending to an existing file, SUMMARY_POS will be non-zero.
   We will write the file starting from SUMMAY_POS.  */

static void
gcov_exit_write_gcda (struct gcov_info *gi_ptr,
                      const struct gcov_summary *prg_p,
                      const gcov_position_t eof_pos,
                      const gcov_position_t summary_pos)

{
  struct gcov_summary_buffer *next_sum_buffer;

  /* Write out the data.  */
  if (!eof_pos)
    {
      gcov_write_tag_length (GCOV_DATA_MAGIC, GCOV_VERSION);
      gcov_write_unsigned (gi_ptr->stamp);
    }

  if (summary_pos)
     gcov_seek (summary_pos);
  gcc_assert (!summary_pos || summary_pos == gcov_position ());

  /* Generate whole program statistics.  */
  gcov_write_summary (GCOV_TAG_PROGRAM_SUMMARY, prg_p);

  /* Rewrite all the summaries that were after the summary we merged
     into.  This is necessary as the merged summary may have a different
     size due to the number of non-zero histogram entries changing after
     merging.  */

  while (sum_buffer)
    {
      gcov_write_summary (GCOV_TAG_PROGRAM_SUMMARY, &sum_buffer->summary);
      next_sum_buffer = sum_buffer->next;
      xfree (sum_buffer);
      sum_buffer = next_sum_buffer;
    }

  gcov_write_build_info (gi_ptr);

  /* Write the counters.  */
  gcov_write_func_counters (gi_ptr);
}

/* Helper function for merging summary.
   Return -1 on error. Return 0 on success.  */

static int
gcov_exit_merge_summary (const struct gcov_info *gi_ptr, struct gcov_summary *prg,
                         struct gcov_summary *this_prg, gcov_unsigned_t crc32,
			 struct gcov_summary *all_prg __attribute__ ((unused)))
{
  struct gcov_ctr_summary *cs_prg, *cs_tprg;
  unsigned t_ix;
#if !GCOV_LOCKED 
  /* summary for all instances of program.  */ 
  struct gcov_ctr_summary *cs_all;
#endif 

  /* Merge the summaries.  */
  for (t_ix = 0; t_ix < GCOV_COUNTERS_SUMMABLE; t_ix++)
    {
      cs_prg = &(prg->ctrs[t_ix]);
      cs_tprg = &(this_prg->ctrs[t_ix]);

      if (gi_ptr->merge[t_ix])
        {
	  int first = !cs_prg->runs;

	  if (!run_accounted)
	    cs_prg->runs++;
          if (first)
            cs_prg->num = cs_tprg->num;
          cs_prg->sum_all += cs_tprg->sum_all;
          if (cs_prg->run_max < cs_tprg->run_max)
            cs_prg->run_max = cs_tprg->run_max;
          cs_prg->sum_max += cs_tprg->run_max;
          if (first)
            memcpy (cs_prg->histogram, cs_tprg->histogram,
                   sizeof (gcov_bucket_type) * GCOV_HISTOGRAM_SIZE);
          else
            gcov_histogram_merge (cs_prg->histogram, cs_tprg->histogram);
        }
      else if (cs_prg->runs)
        {
          gcov_error ("profiling:%s:Merge mismatch for summary.\n",
                      gi_filename);
          return -1;
        }
#if !GCOV_LOCKED
      cs_all = &all_prg->ctrs[t_ix];
      if (!cs_all->runs && cs_prg->runs)
        {
          cs_all->num = cs_prg->num;
          cs_all->runs = cs_prg->runs;
          cs_all->sum_all = cs_prg->sum_all;
          cs_all->run_max = cs_prg->run_max;
          cs_all->sum_max = cs_prg->sum_max;
        }
      else if (!all_prg->checksum
               /* Don't compare the histograms, which may have slight
                  variations depending on the order they were updated
                  due to the truncating integer divides used in the
                  merge.  */
               && (cs_all->num != cs_prg->num
                   || cs_all->runs != cs_prg->runs
                   || cs_all->sum_all != cs_prg->sum_all
                   || cs_all->run_max != cs_prg->run_max
                   || cs_all->sum_max != cs_prg->sum_max))
             {
               gcov_error ("profiling:%s:Data file mismatch - some "
                           "data files may have been concurrently "
                           "updated without locking support\n", gi_filename);
               all_prg->checksum = ~0u;
             }
#endif
    }
  
  prg->checksum = crc32;

  return 0;
}

__attribute__((weak)) gcov_unsigned_t __gcov_lipo_sampling_period;

/* Sort N entries in VALUE_ARRAY in descending order.
   Each entry in VALUE_ARRAY has two values. The sorting
   is based on the second value.  */

GCOV_LINKAGE  void
gcov_sort_n_vals (gcov_type *value_array, int n)
{
  int j, k;
  for (j = 2; j < n; j += 2)
    {
      gcov_type cur_ent[2];
      cur_ent[0] = value_array[j];
      cur_ent[1] = value_array[j + 1];
      k = j - 2;
      while (k >= 0 && value_array[k + 1] < cur_ent[1])
        {
          value_array[k + 2] = value_array[k];
          value_array[k + 3] = value_array[k+1];
          k -= 2;
        }
      value_array[k + 2] = cur_ent[0];
      value_array[k + 3] = cur_ent[1];
    }
}

/* Sort the profile counters for all indirect call sites. Counters
   for each call site are allocated in array COUNTERS.  */

static void
gcov_sort_icall_topn_counter (const struct gcov_ctr_info *counters)
{
  int i;
  gcov_type *values;
  int n = counters->num;
  gcc_assert (!(n % GCOV_ICALL_TOPN_NCOUNTS));

  values = counters->values;

  for (i = 0; i < n; i += GCOV_ICALL_TOPN_NCOUNTS)
    {
      gcov_type *value_array = &values[i + 1];
      gcov_sort_n_vals (value_array, GCOV_ICALL_TOPN_NCOUNTS - 1);
    }
}

static void
gcov_sort_topn_counter_arrays (const struct gcov_info *gi_ptr)
{
  unsigned int i;
  int f_ix;
  const struct gcov_fn_info *gfi_ptr;
  const struct gcov_ctr_info *ci_ptr;

  for (f_ix = 0; (unsigned)f_ix != gi_ptr->n_functions; f_ix++)
    {
      gfi_ptr = gi_ptr->functions[f_ix];
      ci_ptr = gfi_ptr->ctrs;
      for (i = 0; i < GCOV_COUNTERS; i++)
        {
          if (!gcov_counter_active (gi_ptr, i))
            continue;
          if (i == GCOV_COUNTER_ICALL_TOPNV)
            {
              gcov_sort_icall_topn_counter (ci_ptr);
              break;
            }
          ci_ptr++;
        }
     }
}

/* Scaling LIPO sampled profile counters.  */
static void
gcov_scaling_lipo_counters (const struct gcov_info *gi_ptr)
{
  unsigned int i,j,k;
  int f_ix;
  const struct gcov_fn_info *gfi_ptr;
  const struct gcov_ctr_info *ci_ptr;

  if (__gcov_lipo_sampling_period <= 1)
    return;

  for (f_ix = 0; (unsigned)f_ix != gi_ptr->n_functions; f_ix++)
    {
      gfi_ptr = gi_ptr->functions[f_ix];
      ci_ptr = gfi_ptr->ctrs;
      for (i = 0; i < GCOV_COUNTERS; i++)
        {
          if (!gcov_counter_active (gi_ptr, i))
            continue;
          if (i == GCOV_COUNTER_ICALL_TOPNV)
            {
              for (j = 0; j < ci_ptr->num; j += GCOV_ICALL_TOPN_NCOUNTS)
                for (k = 2; k < GCOV_ICALL_TOPN_NCOUNTS; k += 2)
                  ci_ptr->values[j+k] *= __gcov_lipo_sampling_period;
            }
          if (i == GCOV_COUNTER_DIRECT_CALL)
            {
              for (j = 0; j < ci_ptr->num; j += 2)
                ci_ptr->values[j+1] *= __gcov_lipo_sampling_period;
            }
          ci_ptr++;
        }
    }
}

/* Open a gcda file specified by GI_FILENAME.
   Return -1 on error.  Return 0 on success.  */

static int 
gcov_exit_open_gcda_file (struct gcov_info *gi_ptr, struct gcov_filename_aux *gf)
{
  int gcov_prefix_strip;
  size_t prefix_length;
  char *gi_filename_up;

  gcov_prefix_strip = gf->gcov_prefix_strip;
  gi_filename_up = gf->gi_filename_up;
  prefix_length = gf->prefix_length;

  gcov_strip_leading_dirs (prefix_length, gcov_prefix_strip, gi_ptr->filename,
                           gi_filename_up);

  return gcov_open_by_filename (gi_filename);
}

/* Dump the coverage counts for one gcov_info object. We merge with existing
   counts when possible, to avoid growing the .da files ad infinitum. We use
   this program's checksum to make sure we only accumulate whole program
   statistics to the correct summary. An object file might be embedded
   in two separate programs, and we must keep the two program
   summaries separate.  */

static void
gcov_exit_dump_gcov (struct gcov_info *gi_ptr, struct gcov_filename_aux *gf,
		     gcov_unsigned_t crc32, struct gcov_summary *all_prg,
                     struct gcov_summary *this_prg)
{
/* We have to make the decl static as kernel has limited stack size.
   If we put prg to stack, we will running into nasty stack overflow.  */
#if defined(__KERNEL__)
  static
#endif
  struct gcov_summary prg; /* summary for this object over all program.  */
  int error;
  gcov_unsigned_t tag = 0;
  gcov_position_t summary_pos = 0;
  gcov_position_t eof_pos = 0;

  fn_buffer = 0;
  sum_buffer = 0;

  gcov_sort_topn_counter_arrays (gi_ptr);
  gcov_scaling_lipo_counters (gi_ptr);

  error = gcov_exit_open_gcda_file (gi_ptr, gf);
  if (error == -1)
    return;

#if !defined(__KERNEL__)
  tag = gcov_read_unsigned ();
#endif
  if (tag)
    {
      /* Merge data from file.  */
      if (tag != GCOV_DATA_MAGIC)
        {
          gcov_error ("profiling:%s:Not a gcov data file\n", gi_filename);
          goto read_fatal;
        }
      error = gcov_exit_merge_gcda (gi_ptr, &prg, this_prg, &summary_pos, &eof_pos,
				    crc32);
      if (error == -1)
        goto read_fatal;
    }

  gcov_rewrite ();

  if (!summary_pos)
    {
      memset (&prg, 0, sizeof (prg));
      summary_pos = eof_pos;
    }

  error = gcov_exit_merge_summary (gi_ptr, &prg, this_prg, crc32, all_prg);
  if (error == -1)
    goto read_fatal;

  gcov_exit_write_gcda (gi_ptr, &prg, eof_pos, summary_pos);
  /* fall through */

read_fatal:;
#if !defined(__KERNEL__)
  while (fn_buffer)
    fn_buffer = free_fn_data (gi_ptr, fn_buffer, GCOV_COUNTERS);
#else

      /* In LIPO mode, dump the primary module info.  */
      if (gi_ptr->mod_info && gi_ptr->mod_info->is_primary)
        {
          /* Overwrite the zero word at the of the file.  */
          gcov_seek (gi_ptr->eof_pos);
          gcov_write_module_info (gi_ptr, 1);
          /* Write the end marker  */
          gcov_write_unsigned (0); 
        }
#endif

  if ((error = gcov_close ()))
    gcov_error (error  < 0 ?
                "profiling:%s:Overflow writing\n" :
                "profiling:%s:Error writing\n",
                gi_filename);
}

#if !defined (__KERNEL__)
/* Write imported files (auxiliary modules) for primary module GI_PTR
   into file GI_FILENAME.  */

static void
gcov_write_import_file (char *gi_filename, struct gcov_info *gi_ptr)
{
  char  *gi_imports_filename;
  const char *gcov_suffix;
  FILE *imports_file;
  size_t prefix_length, suffix_length;

  gcov_suffix = getenv ("GCOV_IMPORTS_SUFFIX");
  if (!gcov_suffix || !strlen (gcov_suffix))
    gcov_suffix = ".imports";
  suffix_length = strlen (gcov_suffix);
  prefix_length = strlen (gi_filename);
  gi_imports_filename = (char *) alloca (prefix_length + suffix_length + 1);
  memset (gi_imports_filename, 0, prefix_length + suffix_length + 1);
  memcpy (gi_imports_filename, gi_filename, prefix_length);
  memcpy (gi_imports_filename + prefix_length, gcov_suffix, suffix_length);
  imports_file = fopen (gi_imports_filename, "w");
  if (imports_file)
    {
      const struct dyn_imp_mod **imp_mods;
      unsigned i, imp_len;
      imp_mods = gcov_get_sorted_import_module_array (gi_ptr, &imp_len);
      if (imp_mods)
        {
          for (i = 0; i < imp_len; i++)
            {
              fprintf (imports_file, "%s\n",
                       imp_mods[i]->imp_mod->mod_info->source_filename);
              fprintf (imports_file, "%s%s\n",
                       imp_mods[i]->imp_mod->mod_info->da_filename, GCOV_DATA_SUFFIX);
            }
          xfree (imp_mods);
        }
      fclose (imports_file);
    }
}

static void
gcov_dump_module_info (struct gcov_filename_aux *gf)
{
  struct gcov_info *gi_ptr;

  unsigned max_module_id = 0;
  for (gi_ptr = __gcov_list; gi_ptr; gi_ptr = gi_ptr->next)
    {
      unsigned mod_id = gi_ptr->mod_info->ident;
      if (max_module_id < mod_id)
        max_module_id = mod_id;
    }
  char **zero_counts = (char **) xcalloc (max_module_id, sizeof (char *));
  for (gi_ptr = __gcov_list; gi_ptr; gi_ptr = gi_ptr->next)
    {
      unsigned mod_id = gi_ptr->mod_info->ident;
      zero_counts[mod_id-1] = (char *) xcalloc (gi_ptr->n_functions,
                                               sizeof (char));
    }

  /* Compute the module groups and record whether there were any
     counter fixups applied that require rewriting the counters.  */
  int changed = __gcov_compute_module_groups (zero_counts);

  /* Now write out module group info.  */
  for (gi_ptr = __gcov_list; gi_ptr; gi_ptr = gi_ptr->next)
    {
      int error; 

      if (gcov_exit_open_gcda_file (gi_ptr, gf) == -1)
	continue;

      if (changed)
        {
          /* Scan file to find the start of the function section, which is
             where we will start re-writing the counters.  */
          gcov_position_t summary_end_pos;
          if (gcov_scan_to_function_data (gi_ptr, &summary_end_pos) == -1)
            gcov_error ("profiling:%s:Error scanning summaries\n",
                        gi_filename);
          else
            {
              gcov_position_t eof_pos = gi_ptr->eof_pos;
              gcov_rewrite ();
              gcov_seek (summary_end_pos);

              unsigned mod_id = gi_ptr->mod_info->ident;
              gcov_write_comdat_zero_fixup (zero_counts[mod_id-1],
                                            gi_ptr->n_functions);
              gcov_position_t zero_fixup_eof_pos = gcov_position ();

              gcov_write_func_counters (gi_ptr);
              gcc_assert (eof_pos + (zero_fixup_eof_pos - summary_end_pos)
                          == gi_ptr->eof_pos);
            }
        }
      else
        gcov_rewrite ();

      /* Overwrite the zero word at the of the file.  */
      gcov_seek (gi_ptr->eof_pos);

      gcov_write_module_infos (gi_ptr);
      /* Write the end marker  */
      gcov_write_unsigned (0);
      gcov_truncate (); 
      
      if ((error = gcov_close ()))
        gcov_error (error  < 0 ?  "profiling:%s:Overflow writing\n" :
                                  "profiling:%s:Error writing\n",
                                  gi_filename);
      gcov_write_import_file (gi_filename, gi_ptr);
      free (zero_counts[gi_ptr->mod_info->ident-1]);
    }

  free (zero_counts);

  __gcov_finalize_dyn_callgraph ();
}

/* Dump all the coverage counts for the program. It first computes program
   summary and then traverses gcov_list list and dumps the gcov_info
   objects one by one.  */

void
gcov_exit (void)
{
  struct gcov_info *gi_ptr;
  struct gcov_filename_aux gf;
  gcov_unsigned_t crc32;
  int dump_module_info = 0;
  struct gcov_summary all_prg;
  struct gcov_summary this_prg;

  /* Prevent the counters from being dumped a second time on exit when the
     application already wrote out the profile using __gcov_dump().  */
  if (gcov_dump_complete)
    return;

  crc32 = gcov_exit_compute_summary (&this_prg);

  allocate_filename_struct (&gf);
#if !GCOV_LOCKED
  memset (&all_prg, 0, sizeof (all_prg));
#endif

  /* Now merge each file.  */
  for (gi_ptr = __gcov_list; gi_ptr; gi_ptr = gi_ptr->next)
    {
      gcov_exit_dump_gcov (gi_ptr, &gf, crc32, &all_prg, &this_prg);

      /* The IS_PRIMARY field is overloaded to indicate if this module
       is FDO/LIPO.  */
      if (gi_ptr->mod_info)
        dump_module_info |= gi_ptr->mod_info->is_primary;
    }
  run_accounted = 1;

  if (dump_module_info)
    gcov_dump_module_info (&gf);

  if (gi_filename)
    xfree (gi_filename);
}

/* Add a new object file onto the bb chain.  Invoked automatically
  when running an object file's global ctors.  */

void
__gcov_init (struct gcov_info *info)
{
#ifndef IN_GCOV_TOOL
   if (!gcov_sampling_period_initialized)
    {
      const char* env_value_str = getenv ("GCOV_SAMPLING_PERIOD");
      if (env_value_str)
        {
          int env_value_int = atoi(env_value_str);
          if (env_value_int >= 1)
            __gcov_sampling_period = env_value_int;
        }
      env_value_str = getenv ("GCOV_LIPO_SAMPLING_PERIOD");
      if (env_value_str)
        {
          int env_value_int = atoi(env_value_str);
          if (env_value_int >= 0)
            __gcov_lipo_sampling_period = env_value_int;
        }
      gcov_sampling_period_initialized = 1;
    }
#endif

  if (!info->version || !info->n_functions)
    return;
  if (gcov_version (info, info->version, 0))
    {
      size_t filename_length = strlen(info->filename);

      /* Refresh the longest file name information */
      if (filename_length > gcov_max_filename)
        gcov_max_filename = filename_length;

      /* Assign the module ID (starting at 1).  */
      info->mod_info->ident = (++gcov_cur_module_id);
      gcc_assert (EXTRACT_MODULE_ID_FROM_GLOBAL_ID (GEN_FUNC_GLOBAL_ID (
                                                       info->mod_info->ident, 0))
                  == info->mod_info->ident);

      if (!__gcov_list)
        atexit (gcov_exit);

      info->next = __gcov_list;
      __gcov_list = info;
    }
  info->version = 0;
}

#else /* __KERNEL__ */

static struct gcov_filename_aux gf;
static gcov_unsigned_t crc32;
static struct gcov_summary all_prg;
static struct gcov_summary this_prg;
void
gcov_kernel_dump_gcov_init (void)
{
  crc32 = gcov_exit_compute_summary (&this_prg);
  allocate_filename_struct (&gf);
  memset (&all_prg, 0, sizeof (all_prg));
}

void
gcov_kernel_dump_one_gcov(struct gcov_info *info)
{
  gcov_exit_dump_gcov (info, &gf, crc32, &all_prg, &this_prg);
}

#endif /* __KERNEL__ */

/* Reset all counters to zero.  */

void
gcov_clear (void)
{
  const struct gcov_info *gi_ptr;

  for (gi_ptr = __gcov_list; gi_ptr; gi_ptr = gi_ptr->next)
    {
      unsigned f_ix;

      for (f_ix = 0; f_ix < gi_ptr->n_functions; f_ix++)
        {
          unsigned t_ix;
          const struct gcov_fn_info *gfi_ptr = gi_ptr->functions[f_ix];
          const struct gcov_ctr_info *ci_ptr;

          if (!gfi_ptr || gfi_ptr->key != gi_ptr)
            continue;
          ci_ptr = gfi_ptr->ctrs;
          for (t_ix = 0; t_ix != GCOV_COUNTERS; t_ix++)
            {
              if (!gi_ptr->merge[t_ix])
                continue;

              memset (ci_ptr->values, 0, sizeof (gcov_type) * ci_ptr->num);
              ci_ptr++;
            }
        }
    }
}

/* Write out MOD_INFO into the gcda file. IS_PRIMARY is a flag
   indicating if the module is the primary module in the group.  */

void
gcov_write_module_info (const struct gcov_info *mod_info,
                        unsigned is_primary)
{
  gcov_unsigned_t len = 0, filename_len = 0, src_filename_len = 0, i;
  gcov_unsigned_t num_strings;
  gcov_unsigned_t *aligned_fname;
  struct gcov_module_info  *module_info = mod_info->mod_info;
  filename_len = (strlen (module_info->da_filename) +
		  sizeof (gcov_unsigned_t)) / sizeof (gcov_unsigned_t);
  src_filename_len = (strlen (module_info->source_filename) +
		      sizeof (gcov_unsigned_t)) / sizeof (gcov_unsigned_t);
  len = filename_len + src_filename_len;
  len += 2; /* each name string is led by a length.  */

  num_strings = module_info->num_quote_paths + module_info->num_bracket_paths
    + module_info->num_system_paths
    + module_info->num_cpp_defines + module_info->num_cpp_includes
    + module_info->num_cl_args;
  len += gcov_compute_string_array_len (module_info->string_array,
                                        num_strings);

  len += 11; /* 11 more fields */

  gcov_write_tag_length (GCOV_TAG_MODULE_INFO, len);
  gcov_write_unsigned (module_info->ident);
  gcov_write_unsigned (is_primary);
  gcov_write_unsigned (module_info->flags);
  gcov_write_unsigned (module_info->lang);
  gcov_write_unsigned (module_info->ggc_memory);
  gcov_write_unsigned (module_info->num_quote_paths);
  gcov_write_unsigned (module_info->num_bracket_paths);
  gcov_write_unsigned (module_info->num_system_paths);
  gcov_write_unsigned (module_info->num_cpp_defines);
  gcov_write_unsigned (module_info->num_cpp_includes);
  gcov_write_unsigned (module_info->num_cl_args);

  /* Now write the filenames */
  aligned_fname = (gcov_unsigned_t *) alloca ((filename_len + src_filename_len + 2) *
					      sizeof (gcov_unsigned_t));
  memset (aligned_fname, 0,
          (filename_len + src_filename_len + 2) * sizeof (gcov_unsigned_t));
  aligned_fname[0] = filename_len;
  strcpy ((char*) (aligned_fname + 1), module_info->da_filename);
  aligned_fname[filename_len + 1] = src_filename_len;
  strcpy ((char*) (aligned_fname + filename_len + 2), module_info->source_filename);

  for (i = 0; i < (filename_len + src_filename_len + 2); i++)
    gcov_write_unsigned (aligned_fname[i]);

  /* Now write the string array.  */
  gcov_write_string_array (module_info->string_array, num_strings);
}

#endif /* L_gcov */
#endif /* inhibit_libc */
