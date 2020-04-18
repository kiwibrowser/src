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


/* A utility function for outputing errors.  */

static int __attribute__((format(printf, 1, 2)))
gcov_error (const char *fmt, ...)
{
  int ret;
  va_list argp;
  va_start (argp, fmt);
  ret = vprintk (fmt, argp);
  va_end (argp);
  return ret;
}

static void
allocate_filename_struct (struct gcov_filename_aux *gf)
{
  const char *gcov_prefix;
  int gcov_prefix_strip = 0;
  size_t prefix_length = 0;
  char *gi_filename_up;

  /* Allocate and initialize the filename scratch space plus one.  */
  gi_filename = (char *) xmalloc (prefix_length + gcov_max_filename + 2);
  if (prefix_length)
    memcpy (gi_filename, gcov_prefix, prefix_length);
  gi_filename_up = gi_filename + prefix_length;

  gf->gi_filename_up = gi_filename_up;
  gf->prefix_length = prefix_length;
  gf->gcov_prefix_strip = gcov_prefix_strip;
}

static int
gcov_open_by_filename (char *gi_filename)
{
  gcov_open (gi_filename);
  return 0;
}


/* Strip GCOV_PREFIX_STRIP levels of leading '/' from FILENAME and
   put the result into GI_FILENAME_UP.  */

static void
gcov_strip_leading_dirs (int prefix_length, int gcov_prefix_strip,
      			 const char *filename, char *gi_filename_up)
{
  strcpy (gi_filename_up, filename);
}

/* Current virual gcda file. This is for kernel use only.  */
gcov_kernel_vfile *gcov_current_file;

/* Set current virutal gcda file. It needs to be set before dumping
   profile data.  */

void
gcov_set_vfile (gcov_kernel_vfile *file)
{
  gcov_current_file = file;
}

/* File fclose operation in kernel mode.  */

int
kernel_file_fclose (gcov_kernel_vfile *fp)
{
  return 0;
}

/* File ftell operation in kernel mode. It currently should not
   be called.  */

long
kernel_file_ftell (gcov_kernel_vfile *fp)
{
  return 0;
}

/* File fseek operation in kernel mode. It should only be called
   with OFFSET==0 and WHENCE==0 to a freshly opened file.  */

int
kernel_file_fseek (gcov_kernel_vfile *fp, long offset, int whence)
{
  gcc_assert (offset == 0 && whence == 0 && fp->count == 0);
  return 0;
}

/* File ftruncate operation in kernel mode. It currently should not
   be called.  */

int
kernel_file_ftruncate (gcov_kernel_vfile *fp, off_t value)
{
  gcc_assert (0);  /* should not reach here */
  return 0;
}

/* File fread operation in kernel mode. It currently should not
   be called.  */

int
kernel_file_fread (void *ptr, size_t size, size_t nitems,
                  gcov_kernel_vfile *fp)
{
  gcc_assert (0);  /* should not reach here */
  return 0;
}

/* File fwrite operation in kernel mode. It outputs the data
   to a buffer in the virual file.  */

int
kernel_file_fwrite (const void *ptr, size_t size,
                   size_t nitems, gcov_kernel_vfile *fp)
{
  char *vbuf;
  unsigned vsize, vpos;
  unsigned len;

  if (!fp) return 0;

  vbuf = fp->buf;
  vsize = fp->size;
  vpos = fp->count;


  if (vsize < vpos)
    {
      printk (KERN_ERR
         "GCOV_KERNEL: something wrong in file %s: vbuf=%p vsize=%u"
         " vpos=%u\n",
          fp->info->filename, vbuf, vsize, vpos);
      return 0;
    }

  len = vsize - vpos;
  len /= size;

  /* Increase the virtual file size if it is not suffcient. */
  while (len < nitems)
    {
      vsize *= 2;
      len = vsize - vpos;
      len /= size;
    }

  if (vsize != fp->size)
    {
      vbuf = fp->buf = (char *) gcov_realloc_file_buf(vsize, vpos);
      fp->size = vsize;
    }

  if (len > nitems)
	  len = nitems;

  memcpy (vbuf+vpos, ptr, size*len);
  fp->count += len*size;

  if (len != nitems)
    printk (KERN_ERR
        "GCOV_KERNEL: something wrong in file %s: size=%lu nitems=%lu"
        " len=%d vsize=%u vpos=%u \n",
        fp->info->filename, size, nitems, len, vsize, vpos);
  return len;
}

/* File fileno operation in kernel mode. It currently should not
   be called.  */

int
kernel_file_fileno (gcov_kernel_vfile *fp)
{
  gcc_assert (0);  /* should not reach here */
  return 0;
}
