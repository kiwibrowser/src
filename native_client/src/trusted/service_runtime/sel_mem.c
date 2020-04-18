/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL) memory map.
 */

#include "native_client/src/include/portability.h"

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/service_runtime/sel_mem.h"
#include "native_client/src/trusted/service_runtime/sel_util.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/include/sys/mman.h"

#define START_ENTRIES   5   /* tramp+text, rodata, data, bss, stack */
#define REMOVE_MARKED_DEBUG 0


/*
 * The memory map structure is a simple array of memory regions which
 * may have different access protections.  We do not yet merge regions
 * with the same access protections together to reduce the region
 * number, but may do so in the future.
 *
 * Regions are described by (relative) starting page number, the
 * number of pages, and the protection that the pages should have.
 */
struct NaClVmmapEntry *NaClVmmapEntryMake(uintptr_t         page_num,
                                          size_t            npages,
                                          int               prot,
                                          int               flags,
                                          struct NaClDesc   *desc,
                                          nacl_off64_t      offset,
                                          nacl_off64_t      file_size) {
  struct NaClVmmapEntry *entry;

  NaClLog(4,
          "NaClVmmapEntryMake(0x%"NACL_PRIxPTR",0x%"NACL_PRIxS","
          "0x%x,0x%x,0x%"NACL_PRIxPTR",0x%"NACL_PRIx64")\n",
          page_num, npages, prot, flags, (uintptr_t) desc, offset);
  entry = (struct NaClVmmapEntry *) malloc(sizeof *entry);
  if (NULL == entry) {
    return 0;
  }
  NaClLog(4, "entry: 0x%"NACL_PRIxPTR"\n", (uintptr_t) entry);
  entry->page_num = page_num;
  entry->npages = npages;
  entry->prot = prot;
  entry->flags = flags;
  entry->removed = 0;
  entry->desc = desc;
  if (desc != NULL) {
    NaClDescRef(desc);
  }
  entry->offset = offset;
  entry->file_size = file_size;
  return entry;
}


void  NaClVmmapEntryFree(struct NaClVmmapEntry *entry) {
  NaClLog(4,
          ("NaClVmmapEntryFree(0x%08"NACL_PRIxPTR
           "): (0x%"NACL_PRIxPTR",0x%"NACL_PRIxS","
           "0x%x,0x%x,0x%"NACL_PRIxPTR",0x%"NACL_PRIx64")\n"),
          (uintptr_t) entry,
          entry->page_num, entry->npages, entry->prot,
          entry->flags, (uintptr_t) entry->desc, entry->offset);

  if (entry->desc != NULL) {
    NaClDescSafeUnref(entry->desc);
  }
  free(entry);
}


/*
 * Print debug.
 */
void NaClVmentryPrint(void                  *state,
                      struct NaClVmmapEntry *vmep) {
  UNREFERENCED_PARAMETER(state);

  printf("page num 0x%06x\n", (uint32_t)vmep->page_num);
  printf("num pages %d\n", (uint32_t)vmep->npages);
  printf("prot bits %x\n", vmep->prot);
  printf("flags %x\n", vmep->flags);
  fflush(stdout);
}


void NaClVmmapDebug(struct NaClVmmap *self,
                    char             *msg) {
  puts(msg);
  NaClVmmapVisit(self, NaClVmentryPrint, (void *) 0);
  fflush(stdout);
}


int NaClVmmapCtor(struct NaClVmmap *self) {
  self->size = START_ENTRIES;
  if (SIZE_T_MAX / sizeof *self->vmentry < self->size) {
    return 0;
  }
  self->vmentry = calloc(self->size, sizeof *self->vmentry);
  if (!self->vmentry) {
    return 0;
  }
  self->nvalid = 0;
  self->is_sorted = 1;
  return 1;
}


void NaClVmmapDtor(struct NaClVmmap *self) {
  size_t i;

  for (i = 0; i < self->nvalid; ++i) {
    NaClVmmapEntryFree(self->vmentry[i]);
  }
  free(self->vmentry);
  self->vmentry = 0;
}

/*
 * Comparison function for qsort.  Should never encounter a
 * removed/invalid entry.
 */

static int NaClVmmapCmpEntries(void const  *vleft,
                               void const  *vright) {
  struct NaClVmmapEntry const *const *left =
      (struct NaClVmmapEntry const *const *) vleft;
  struct NaClVmmapEntry const *const *right =
      (struct NaClVmmapEntry const *const *) vright;

  return (int) ((*left)->page_num - (*right)->page_num);
}


static void NaClVmmapRemoveMarked(struct NaClVmmap *self) {
  size_t  i;
  size_t  last;

  if (0 == self->nvalid)
    return;

#if REMOVE_MARKED_DEBUG
  NaClVmmapDebug(self, "Before RemoveMarked");
#endif
  /*
   * Linearly scan with a move-end-to-front strategy to get rid of
   * marked-to-be-removed entries.
   */

  /*
   * Invariant:
   *
   * forall j in [0, self->nvalid): NULL != self->vmentry[j]
   */
  for (last = self->nvalid; last > 0 && self->vmentry[--last]->removed; ) {
    NaClVmmapEntryFree(self->vmentry[last]);
    self->vmentry[last] = NULL;
  }
  if (last == 0 && self->vmentry[0]->removed) {
    NaClLog(LOG_FATAL, "No valid entries in VM map\n");
    return;
  }

  /*
   * Post condition of above loop:
   *
   * forall j in [0, last]: NULL != self->vmentry[j]
   *
   * 0 <= last < self->nvalid && !self->vmentry[last]->removed
   */
  CHECK(last < self->nvalid);
  CHECK(!self->vmentry[last]->removed);
  /*
   * and,
   *
   * forall j in (last, self->nvalid): NULL == self->vmentry[j]
   */

  /*
   * Loop invariant: forall j in [0, i):  !self->vmentry[j]->removed
   */
  for (i = 0; i < last; ++i) {
    if (!self->vmentry[i]->removed) {
      continue;
    }
    /*
     * post condition: self->vmentry[i]->removed
     *
     * swap with entry at self->vmentry[last].
     */

    NaClVmmapEntryFree(self->vmentry[i]);
    self->vmentry[i] = self->vmentry[last];
    self->vmentry[last] = NULL;

    /*
     * Invariants here:
     *
     * forall j in [last, self->nvalid): NULL == self->vmentry[j]
     *
     * forall j in [0, i]: !self->vmentry[j]->removed
     */

    while (--last > i && self->vmentry[last]->removed) {
      NaClVmmapEntryFree(self->vmentry[last]);
      self->vmentry[last] = NULL;
    }
    /*
     * since !self->vmentry[i]->removed, we are guaranteed that
     * !self->vmentry[last]->removed when the while loop terminates.
     *
     * forall j in (last, self->nvalid):
     *  NULL == self->vmentry[j]->removed
     */
  }
  /* i == last */
  /* forall j in [0, last]: !self->vmentry[j]->removed */
  /* forall j in (last, self->nvalid): NULL == self->vmentry[j] */
  self->nvalid = last + 1;

  self->is_sorted = 0;
#if REMOVE_MARKED_DEBUG
  NaClVmmapDebug(self, "After RemoveMarked");
#endif
}


void NaClVmmapMakeSorted(struct NaClVmmap  *self) {
  if (self->is_sorted)
    return;

  NaClVmmapRemoveMarked(self);

  qsort(self->vmentry,
        self->nvalid,
        sizeof *self->vmentry,
        NaClVmmapCmpEntries);
  self->is_sorted = 1;
#if REMOVE_MARKED_DEBUG
  NaClVmmapDebug(self, "After Sort");
#endif
}

void NaClVmmapAdd(struct NaClVmmap  *self,
                  uintptr_t         page_num,
                  size_t            npages,
                  int               prot,
                  int               flags,
                  struct NaClDesc   *desc,
                  nacl_off64_t      offset,
                  nacl_off64_t      file_size) {
  struct NaClVmmapEntry *entry;

  NaClLog(2,
          ("NaClVmmapAdd(0x%08"NACL_PRIxPTR", 0x%"NACL_PRIxPTR", "
           "0x%"NACL_PRIxS", 0x%x, 0x%x, 0x%"NACL_PRIxPTR", "
           "0x%"NACL_PRIx64")\n"),
          (uintptr_t) self, page_num, npages, prot, flags,
          (uintptr_t) desc, offset);
  if (self->nvalid == self->size) {
    size_t                    new_size = 2 * self->size;
    struct NaClVmmapEntry     **new_map;

    new_map = realloc(self->vmentry, new_size * sizeof *new_map);
    if (NULL == new_map) {
      NaClLog(LOG_FATAL, "NaClVmmapAdd: could not allocate memory\n");
      return;
    }
    self->vmentry = new_map;
    self->size = new_size;
  }
  /* self->nvalid < self->size */
  entry = NaClVmmapEntryMake(page_num, npages, prot, flags,
      desc, offset, file_size);

  self->vmentry[self->nvalid] = entry;
  self->is_sorted = 0;
  ++self->nvalid;
}

/*
 * Update the virtual memory map.  Deletion is handled by a remove
 * flag, since a NULL desc just means that the memory is backed by the
 * system paging file.
 */
static void NaClVmmapUpdate(struct NaClVmmap  *self,
                            uintptr_t         page_num,
                            size_t            npages,
                            int               prot,
                            int               flags,
                            int               remove,
                            struct NaClDesc   *desc,
                            nacl_off64_t      offset,
                            nacl_off64_t      file_size) {
  /* update existing entries or create new entry as needed */
  size_t                i;
  uintptr_t             new_region_end_page = page_num + npages;

  NaClLog(2,
          ("NaClVmmapUpdate(0x%08"NACL_PRIxPTR", 0x%"NACL_PRIxPTR", "
           "0x%"NACL_PRIxS", 0x%x, 0x%x, %d, 0x%"NACL_PRIxPTR", "
           "0x%"NACL_PRIx64")\n"),
          (uintptr_t) self, page_num, npages, prot, flags,
          remove, (uintptr_t) desc, offset);
  NaClVmmapMakeSorted(self);

  CHECK(npages > 0);

  for (i = 0; i < self->nvalid; i++) {
    struct NaClVmmapEntry *ent = self->vmentry[i];
    uintptr_t             ent_end_page = ent->page_num + ent->npages;
    nacl_off64_t          additional_offset =
        (new_region_end_page - ent->page_num) << NACL_PAGESHIFT;


    if (ent->page_num < page_num && new_region_end_page < ent_end_page) {
      /*
       * Split existing mapping into two parts, with new mapping in
       * the middle.
       */
      NaClVmmapAdd(self,
                   new_region_end_page,
                   ent_end_page - new_region_end_page,
                   ent->prot,
                   ent->flags,
                   ent->desc,
                   ent->offset + additional_offset,
                   ent->file_size);
      ent->npages = page_num - ent->page_num;
      break;
    } else if (ent->page_num < page_num && page_num < ent_end_page) {
      /* New mapping overlaps end of existing mapping. */
      ent->npages = page_num - ent->page_num;
    } else if (ent->page_num < new_region_end_page &&
               new_region_end_page < ent_end_page) {
      /* New mapping overlaps start of existing mapping. */
      ent->page_num = new_region_end_page;
      ent->npages = ent_end_page - new_region_end_page;
      ent->offset += additional_offset;
      break;
    } else if (page_num <= ent->page_num &&
               ent_end_page <= new_region_end_page) {
      /* New mapping covers all of the existing mapping. */
      ent->removed = 1;
    } else {
      /* No overlap */
      assert(new_region_end_page <= ent->page_num || ent_end_page <= page_num);
    }
  }

  if (!remove) {
    NaClVmmapAdd(self, page_num, npages, prot, flags, desc, offset, file_size);
  }

  NaClVmmapRemoveMarked(self);
}

void NaClVmmapAddWithOverwrite(struct NaClVmmap   *self,
                               uintptr_t          page_num,
                               size_t             npages,
                               int                prot,
                               int                flags,
                               struct NaClDesc    *desc,
                               nacl_off64_t       offset,
                               nacl_off64_t       file_size) {
  NaClVmmapUpdate(self,
                  page_num,
                  npages,
                  prot,
                  flags,
                  /* remove= */ 0,
                  desc,
                  offset,
                  file_size);
}

void NaClVmmapRemove(struct NaClVmmap   *self,
                     uintptr_t          page_num,
                     size_t             npages) {
  NaClVmmapUpdate(self,
                  page_num,
                  npages,
                  /* prot= */ 0,
                  /* flags= */ 0,
                  /* remove= */ 1,
                  /* desc= */NULL,
                  /* offset= */0,
                  /* file_size= */0);
}

/*
 * NaClVmmapCheckMapping checks whether there is an existing mapping with
 * maximum protection equivalent or higher to the given one.
 */
static int NaClVmmapCheckExistingMapping(struct NaClVmmap  *self,
                                         uintptr_t         page_num,
                                         size_t            npages,
                                         int               prot) {
  size_t      i;
  uintptr_t   region_end_page = page_num + npages;

  NaClLog(2,
          ("NaClVmmapCheckExistingMapping(0x%08"NACL_PRIxPTR", 0x%"NACL_PRIxPTR
           ", 0x%"NACL_PRIxS", 0x%x)\n"),
          (uintptr_t) self, page_num, npages, prot);

  if (0 == self->nvalid) {
    return 0;
  }
  NaClVmmapMakeSorted(self);

  for (i = 0; i < self->nvalid; ++i) {
    struct NaClVmmapEntry   *ent = self->vmentry[i];
    uintptr_t               ent_end_page = ent->page_num + ent->npages;
    int                     flags = NaClVmmapEntryMaxProt(ent);

    if (ent->page_num <= page_num && region_end_page <= ent_end_page) {
      /* The mapping is inside existing entry. */
      return 0 == (prot & (~flags));
    } else if (ent->page_num <= page_num && page_num < ent_end_page) {
      /* The mapping overlaps the entry. */
      if (0 != (prot & (~flags))) {
        return 0;
      }
      page_num = ent_end_page;
      npages = region_end_page - ent_end_page;
    } else if (page_num < ent->page_num) {
      /* The mapping without backing store. */
      return 0;
    }
  }
  return 0;
}

int NaClVmmapChangeProt(struct NaClVmmap   *self,
                        uintptr_t          page_num,
                        size_t             npages,
                        int                prot) {
  size_t      i;
  size_t      nvalid;
  uintptr_t   new_region_end_page = page_num + npages;

  /*
   * NaClVmmapCheckExistingMapping should be always called before
   * NaClVmmapChangeProt proceeds to ensure that valid mapping exists
   * as modifications cannot be rolled back.
   */
  if (!NaClVmmapCheckExistingMapping(self, page_num, npages, prot)) {
    return 0;
  }

  NaClLog(2,
          ("NaClVmmapChangeProt(0x%08"NACL_PRIxPTR", 0x%"NACL_PRIxPTR
           ", 0x%"NACL_PRIxS", 0x%x)\n"),
          (uintptr_t) self, page_num, npages, prot);
  NaClVmmapMakeSorted(self);

  /*
   * This loop & interval boundary tests closely follow those in
   * NaClVmmapUpdate. When updating those, do not forget to update them
   * at both places where appropriate.
   * TODO(phosek): use better data structure which will support intervals
   */

  for (i = 0, nvalid = self->nvalid; i < nvalid && npages > 0; i++) {
    struct NaClVmmapEntry *ent = self->vmentry[i];
    uintptr_t             ent_end_page = ent->page_num + ent->npages;
    nacl_off64_t          additional_offset =
        (new_region_end_page - ent->page_num) << NACL_PAGESHIFT;

    if (ent->page_num < page_num && new_region_end_page < ent_end_page) {
      /* Split existing mapping into two parts */
      NaClVmmapAdd(self,
                   new_region_end_page,
                   ent_end_page - new_region_end_page,
                   ent->prot,
                   ent->flags,
                   ent->desc,
                   ent->offset + additional_offset,
                   ent->file_size);
      ent->npages = page_num - ent->page_num;
      /* Add the new mapping into the middle. */
      NaClVmmapAdd(self,
                   page_num,
                   npages,
                   prot,
                   ent->flags,
                   ent->desc,
                   ent->offset + (page_num - ent->page_num),
                   ent->file_size);
      break;
    } else if (ent->page_num < page_num && page_num < ent_end_page) {
      /* New mapping overlaps end of existing mapping. */
      ent->npages = page_num - ent->page_num;
      /* Add the overlapping part of the mapping. */
      NaClVmmapAdd(self,
                   page_num,
                   ent_end_page - page_num,
                   prot,
                   ent->flags,
                   ent->desc,
                   ent->offset + (page_num - ent->page_num),
                   ent->file_size);
      /* The remaining part (if any) will be added in other iteration. */
      page_num = ent_end_page;
      npages = new_region_end_page - ent_end_page;
    } else if (ent->page_num < new_region_end_page &&
               new_region_end_page < ent_end_page) {
      /* New mapping overlaps start of existing mapping, split it. */
      NaClVmmapAdd(self,
                   page_num,
                   npages,
                   prot,
                   ent->flags,
                   ent->desc,
                   ent->offset,
                   ent->file_size);
      ent->page_num = new_region_end_page;
      ent->npages = ent_end_page - new_region_end_page;
      ent->offset += additional_offset;
      break;
    } else if (page_num <= ent->page_num &&
               ent_end_page <= new_region_end_page) {
      /* New mapping covers all of the existing mapping. */
      page_num = ent_end_page;
      npages = new_region_end_page - ent_end_page;
      ent->prot = prot;
    } else {
      /* No overlap */
      assert(new_region_end_page <= ent->page_num || ent_end_page <= page_num);
    }
  }
  return 1;
}

int NaClVmmapEntryMaxProt(struct NaClVmmapEntry *entry) {
  int flags = PROT_NONE;

  if (entry->desc != NULL && 0 == (entry->flags & NACL_ABI_MAP_PRIVATE)) {
    int o_flags = (*NACL_VTBL(NaClDesc, entry->desc)->GetFlags)(entry->desc);
    switch (o_flags & NACL_ABI_O_ACCMODE) {
      case NACL_ABI_O_RDONLY:
        flags = NACL_ABI_PROT_READ;
        break;
      case NACL_ABI_O_WRONLY:
        flags = NACL_ABI_PROT_WRITE;
        break;
      case NACL_ABI_O_RDWR:
        flags = NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE;
        break;
      default:
        NaClLog(LOG_FATAL, "Internal error: illegal O_ACCMODE\n");
        break;
    }
  } else {
    flags = NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE;
  }

  return flags;
}

static int NaClVmmapContainCmpEntries(void const *vkey,
                                      void const *vent) {
  struct NaClVmmapEntry const *const *key =
      (struct NaClVmmapEntry const *const *) vkey;
  struct NaClVmmapEntry const *const *ent =
      (struct NaClVmmapEntry const *const *) vent;

  NaClLog(5, "key->page_num   = 0x%05"NACL_PRIxPTR"\n", (*key)->page_num);

  NaClLog(5, "entry->page_num = 0x%05"NACL_PRIxPTR"\n", (*ent)->page_num);
  NaClLog(5, "entry->npages   = 0x%"NACL_PRIxS"\n", (*ent)->npages);

  if ((*key)->page_num < (*ent)->page_num) return -1;
  if ((*key)->page_num < (*ent)->page_num + (*ent)->npages) return 0;
  return 1;
}

struct NaClVmmapEntry const *NaClVmmapFindPage(struct NaClVmmap *self,
                                               uintptr_t        pnum) {
  struct NaClVmmapEntry key;
  struct NaClVmmapEntry *kptr;
  struct NaClVmmapEntry *const *result_ptr;

  NaClVmmapMakeSorted(self);
  key.page_num = pnum;
  kptr = &key;
  result_ptr = ((struct NaClVmmapEntry *const *)
                bsearch(&kptr,
                        self->vmentry,
                        self->nvalid,
                        sizeof self->vmentry[0],
                        NaClVmmapContainCmpEntries));
  return result_ptr ? *result_ptr : NULL;
}


struct NaClVmmapIter *NaClVmmapFindPageIter(struct NaClVmmap      *self,
                                            uintptr_t             pnum,
                                            struct NaClVmmapIter  *space) {
  struct NaClVmmapEntry key;
  struct NaClVmmapEntry *kptr;
  struct NaClVmmapEntry **result_ptr;

  NaClVmmapMakeSorted(self);
  key.page_num = pnum;
  kptr = &key;
  result_ptr = ((struct NaClVmmapEntry **)
                bsearch(&kptr,
                        self->vmentry,
                        self->nvalid,
                        sizeof self->vmentry[0],
                        NaClVmmapContainCmpEntries));
  space->vmmap = self;
  if (NULL == result_ptr) {
    space->entry_ix = self->nvalid;
  } else {
    space->entry_ix = result_ptr - self->vmentry;
  }
  return space;
}


int NaClVmmapIterAtEnd(struct NaClVmmapIter *nvip) {
  return nvip->entry_ix >= nvip->vmmap->nvalid;
}


/*
 * IterStar only permissible if not AtEnd
 */
struct NaClVmmapEntry *NaClVmmapIterStar(struct NaClVmmapIter *nvip) {
  return nvip->vmmap->vmentry[nvip->entry_ix];
}


void NaClVmmapIterIncr(struct NaClVmmapIter *nvip) {
  ++nvip->entry_ix;
}


/*
 * Iterator becomes invalid after Erase.  We could have a version that
 * keep the iterator valid by copying forward, but it is unclear
 * whether that is needed.
 */
void NaClVmmapIterErase(struct NaClVmmapIter *nvip) {
  struct NaClVmmap  *nvp;

  nvp = nvip->vmmap;
  free(nvp->vmentry[nvip->entry_ix]);
  nvp->vmentry[nvip->entry_ix] = nvp->vmentry[--nvp->nvalid];
  nvp->is_sorted = 0;
}


void  NaClVmmapVisit(struct NaClVmmap *self,
                     void             (*fn)(void                  *state,
                                            struct NaClVmmapEntry *entry),
                     void             *state) {
  size_t i;
  size_t nentries;

  NaClVmmapMakeSorted(self);
  for (i = 0, nentries = self->nvalid; i < nentries; ++i) {
    (*fn)(state, self->vmentry[i]);
  }
}


/*
 * Linear search, from high addresses down.
 */
uintptr_t NaClVmmapFindSpace(struct NaClVmmap *self,
                             size_t           num_pages) {
  size_t                i;
  struct NaClVmmapEntry *vmep;
  uintptr_t             end_page;
  uintptr_t             start_page;

  if (0 == self->nvalid)
    return 0;
  NaClVmmapMakeSorted(self);
  for (i = self->nvalid; --i > 0; ) {
    vmep = self->vmentry[i-1];
    end_page = vmep->page_num + vmep->npages;  /* end page from previous */
    start_page = self->vmentry[i]->page_num;  /* start page from current */
    if (start_page - end_page >= num_pages) {
      return start_page - num_pages;
    }
  }
  return 0;
  /*
   * in user addresses, page 0 is always trampoline, and user
   * addresses are contained in system addresses, so returning a
   * system page number of 0 can serve as error indicator: it is at
   * worst the trampoline page, and likely to be below it.
   */
}


/*
 * Linear search, from high addresses down.  For mmap, so the starting
 * address of the region found must be NACL_MAP_PAGESIZE aligned.
 *
 * For general mmap it is better to use as high an address as
 * possible, since the stack size for the main thread is currently
 * fixed, and the heap is the only thing that grows.
 */
uintptr_t NaClVmmapFindMapSpace(struct NaClVmmap *self,
                                size_t           num_pages) {
  size_t                i;
  struct NaClVmmapEntry *vmep;
  uintptr_t             end_page;
  uintptr_t             start_page;

  if (0 == self->nvalid)
    return 0;
  NaClVmmapMakeSorted(self);
  num_pages = NaClRoundPageNumUpToMapMultiple(num_pages);

  for (i = self->nvalid; --i > 0; ) {
    vmep = self->vmentry[i-1];
    end_page = vmep->page_num + vmep->npages;  /* end page from previous */
    end_page = NaClRoundPageNumUpToMapMultiple(end_page);

    start_page = self->vmentry[i]->page_num;  /* start page from current */
    if (NACL_MAP_PAGESHIFT > NACL_PAGESHIFT) {

      start_page = NaClTruncPageNumDownToMapMultiple(start_page);

      if (start_page <= end_page) {
        continue;
      }
    }
    if (start_page - end_page >= num_pages) {
      return start_page - num_pages;
    }
  }
  return 0;
  /*
   * in user addresses, page 0 is always trampoline, and user
   * addresses are contained in system addresses, so returning a
   * system page number of 0 can serve as error indicator: it is at
   * worst the trampoline page, and likely to be below it.
   */
}


/*
 * Linear search, from uaddr up.
 */
uintptr_t NaClVmmapFindMapSpaceAboveHint(struct NaClVmmap *self,
                                         uintptr_t        uaddr,
                                         size_t           num_pages) {
  size_t                nvalid;
  size_t                i;
  struct NaClVmmapEntry *vmep;
  uintptr_t             usr_page;
  uintptr_t             start_page;
  uintptr_t             end_page;

  NaClVmmapMakeSorted(self);

  usr_page = uaddr >> NACL_PAGESHIFT;
  num_pages = NaClRoundPageNumUpToMapMultiple(num_pages);

  nvalid = self->nvalid;

  for (i = 1; i < nvalid; ++i) {
    vmep = self->vmentry[i-1];
    end_page = vmep->page_num + vmep->npages;
    end_page = NaClRoundPageNumUpToMapMultiple(end_page);

    start_page = self->vmentry[i]->page_num;
    if (NACL_MAP_PAGESHIFT > NACL_PAGESHIFT) {

      start_page = NaClTruncPageNumDownToMapMultiple(start_page);

      if (start_page <= end_page) {
        continue;
      }
    }
    if (end_page <= usr_page && usr_page < start_page) {
      end_page = usr_page;
    }
    if (usr_page <= end_page && (start_page - end_page) >= num_pages) {
      /* found a gap at or after uaddr that's big enough */
      return end_page;
    }
  }
  return 0;
}
