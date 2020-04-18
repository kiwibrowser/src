/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This is a custom linker script used to build nacl_helper_bootstrap.
 * It has a very special layout.  This script will only work with input
 * that is kept extremely minimal.  If there are unexpected input sections
 * not named here, the result will not be correct.
 *
 * We need to use a standalone loader program rather than just using a
 * dynamically-linked program here because its entire address space will be
 * taken over for the NaCl untrusted address space.  A normal program would
 * cause dynamic linker data structures to point to its .dynamic section,
 * which is no longer available after startup.
 *
 * We need this special layout (and the nacl_helper_bootstrap_munge_phdr
 * step) because simply having bss space large enough to reserve the
 * address space would cause the kernel loader to think we're using that
 * much anonymous memory and refuse to execute the program on a machine
 * with not much memory available.
 */

/*
 * Set the entry point to the symbol called _start, which we define in assembly.
 */
ENTRY(_start)

/*
 * This is the address where the program text starts.
 * We set this as low as we think we can get away with.
 * The common settings for sysctl vm.mmap_min_addr range from 4k to 64k.
 */
TEXT_START = 0x10000;

/*
 * The symbol RESERVE_TOP is the top of the range we are trying to reserve.
 * This is set via --defsym on the linker command line, because the correct
 * value differs for each machine.  It is defined to be 0x0 if we do not
 * actually need any space reserved for this configuration.
 */

/*
 * We specify the program headers we want explicitly, to get the layout
 * exactly right and to give the "reserve" segment p_flags of zero, so
 * that it gets mapped as PROT_NONE.
 */
PHDRS {
  text PT_LOAD FILEHDR PHDRS;
  data PT_LOAD;
  reserve PT_LOAD FLAGS(0);
  r_debug PT_LOAD;
  note PT_NOTE;
  stack PT_GNU_STACK FLAGS(6);  /* RW, no E */
}

/*
 * Now we lay out the sections across those segments.
 */
SECTIONS {
  . = TEXT_START + SIZEOF_HEADERS;

  /*
   * The build ID note usually comes first.
   * It's both part of the text PT_LOAD segment (like other rodata) and
   * it's what the PT_NOTE header points to.
   */
  .note.gnu.build-id : {
    *(.note.gnu.build-id)
  } :text :note

  /*
   * Here is the program itself.
   */
  .text : {
    *(.text*)
  } :text
  .rodata : {
    *(.rodata*)
    *(.eh_frame*)
  }

  etext = .;

  /*
   * Adjust the address for the data segment.  We want to adjust up to
   * the same address within the page on the next page up.
   */
  . = (ALIGN(CONSTANT(MAXPAGESIZE)) -
       ((CONSTANT(MAXPAGESIZE) - .) & (CONSTANT(MAXPAGESIZE) - 1)));
  . = DATA_SEGMENT_ALIGN(CONSTANT(MAXPAGESIZE), CONSTANT(COMMONPAGESIZE));

  .data : {
    *(.data*)
  } :data
  .bss : {
    *(.bss*)
  }

  /*
   * Now we move up to the next p_align increment, and place the dummy
   * segment there.  The linker emits this segment with the p_vaddr and
   * p_memsz we want, which reserves the address space.  But the linker
   * gives it a p_filesz of zero.  We have to edit the phdr after link
   * time to give it a p_filesz matching its p_memsz.  That way, the
   * kernel doesn't think we are preallocating a huge amount of memory.
   * It just maps it from the file, i.e. way off the end of the file,
   * which is perfect for reserving the address space.
   */
  . = ALIGN(CONSTANT(COMMONPAGESIZE));
  RESERVE_START = .;
  .reserve : {
    . += (RESERVE_TOP > RESERVE_START) ? (RESERVE_TOP - RESERVE_START) : 0;
  } :reserve

  /*
   * This must be placed above the reserved address space, so it won't
   * be clobbered by NaCl.  We want this to be visible at its fixed address
   * in the memory image so the debugger can make sense of things.
   */
  .r_debug : {
    *(.r_debug)
  } :r_debug

  /*
   * These are empty input sections the linker generates.
   * If we don't discard them, they pollute the flags in the output segment.
   */
  /DISCARD/ : {
    *(.iplt)
    *(.reginfo)
    *(.rel*)
    *(.igot.plt)
    *(.ARM.exidx)
  }
}
