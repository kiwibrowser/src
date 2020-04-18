/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#if defined(__GLIBC__)
# include <link.h>
#endif
#include <nacl/nacl_list_mappings.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/trusted/service_runtime/include/sys/nacl_list_mappings.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

#if defined(__GLIBC__) && __GLIBC__ == 2 && __GLIBC_MINOR__ == 9
static const uint32_t kDynamicTextEnd = 1 << 28;
#endif

static void test_list_mappings_read_write(void) {
  void *blk;

  printf("Testing list_mappings on a read-write area.\n");

  blk = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  ASSERT_NE(blk, MAP_FAILED);

  struct NaClMemMappingInfo map[0x10000];
  size_t capacity = sizeof(map) / sizeof(*map);
  size_t size;
  int result = nacl_list_mappings(map, capacity, &size);
  ASSERT_EQ(0, result);
  ASSERT_LE(size, capacity);

  bool found = false;
  for (size_t i = 0; i < size; ++i) {
    if (map[i].start == (uint32_t) blk && map[i].size == 0x10000 &&
        map[i].prot == (PROT_READ | PROT_WRITE)) {
      found = true;
      break;
    }
  }
  ASSERT(found);
}

static void test_list_mappings_bad_destination(void) {
  printf("Testing list_mappings on a bad destination.\n");

  size_t got;
  int result = nacl_list_mappings(NULL, 1024 * 1024, &got);
  ASSERT_NE(0, result);
  ASSERT_EQ(EFAULT, errno);
}

#if defined(__GLIBC__)

struct TestListMappingsPhdrs {
  struct dl_phdr_info dl_objs[16];
  size_t i;
  size_t max_phnum;
};

static int visit_phdrs(struct dl_phdr_info *info, size_t size, void *data) {
  struct TestListMappingsPhdrs *state = data;

  if (info->dlpi_phnum > state->max_phnum)
    state->max_phnum = info->dlpi_phnum;

  ASSERT(state->i < sizeof(state->dl_objs) / sizeof(state->dl_objs[0]));
  state->dl_objs[state->i++] = *info;

  return 0;
}

static uint32_t trunc_page(uint32_t addr) {
  return addr & -NACL_MAP_PAGESIZE;
}

static uint32_t round_page(uint32_t addr) {
  return trunc_page(addr + NACL_MAP_PAGESIZE - 1);
}

static void test_list_mappings_phdrs(void) {
  struct NaClMemMappingInfo map[0x10000];
  size_t nmaps;

  printf("Testing list_mappings on all the phdrs.\n");

  size_t capacity = sizeof(map) / sizeof(map[0]);
  int result = nacl_list_mappings(map, capacity, &nmaps);
  ASSERT_EQ(0, result);
  ASSERT_LE(nmaps, capacity);

  struct TestListMappingsPhdrs state = { .i = 0, .max_phnum = 0 };
  dl_iterate_phdr(visit_phdrs, &state);
  ASSERT_GT(state.i, 0);
  ASSERT_LT(state.i, nmaps);

  bool bad = false;
  bool phdrs_found[state.i * state.max_phnum];
  memset(phdrs_found, 0, sizeof(phdrs_found));
  for (size_t i = 0; i < nmaps; ++i) {
    const struct NaClMemMappingInfo *m = &map[i];

    for (size_t j = 0; j < state.i; ++j) {
      const struct dl_phdr_info *info = &state.dl_objs[j];

      for (size_t k = 0; k < info->dlpi_phnum; ++k) {
        const ElfW(Phdr) *ph = &info->dlpi_phdr[k];
        if (ph->p_type != PT_LOAD)
          continue;

        uint32_t start = trunc_page(info->dlpi_addr + ph->p_vaddr);
        uint32_t memsz = round_page(info->dlpi_addr + ph->p_vaddr +
                                    ph->p_memsz) - start;
        int prot = 0;
        if (ph->p_flags & PF_R)
          prot |= PROT_READ;
        if (ph->p_flags & PF_W)
          prot |= PROT_WRITE;
        if (ph->p_flags & PF_X) {
# if __GLIBC__ == 2 && __GLIBC_MINOR__ == 9
          /*
           * In the old glibc, some regions appear in the phdr as PF_X
           * when they cannot actually be PROT_EXEC.
           */
          if (start < kDynamicTextEnd)
# endif
            prot |= PROT_EXEC;
        }

        if (m->start >= start &&
            m->size <= start + memsz &&
            start + memsz - m->size >= m->start) {
          phdrs_found[j * state.max_phnum + k] = true;
          if (m->prot != prot) {
            bool prot_bad = true;
            if (m->prot == PROT_READ && prot == (PROT_READ | PROT_WRITE)) {
              /*
               * This mismatch is kosher in the case of PT_GNU_RELRO pages.
               * These were read-write when mapped, but then got mprotect'd
               * to read-only.
               */
              for (size_t r = 0; r < info->dlpi_phnum; ++r) {
                const ElfW(Phdr) *rph = &info->dlpi_phdr[r];
                if (rph->p_type == PT_GNU_RELRO) {
                  uint32_t rstart = trunc_page(info->dlpi_addr + ph->p_vaddr);
                  uint32_t rmemsz = trunc_page(info->dlpi_addr + ph->p_vaddr +
                                               ph->p_memsz) - start;
                  if (m->start >= rstart &&
                      m->size <= rstart + rmemsz &&
                      rstart + rmemsz - m->size >= m->start) {
                    prot_bad = false;
                    break;
                  }
                }
              }
            }
            if (prot_bad) {
              bad = true;
              printf("Mapping %zu %#.8x+%#x prot %#x vs %s phdr %zu %#x\n",
                     i, m->start, m->size, m->prot, info->dlpi_name, k, prot);
            }
          }
        }
      }
    }
  }

  for (size_t i = 0; i < state.i; ++i) {
    const struct dl_phdr_info *info = &state.dl_objs[i];
    bool *found = &phdrs_found[i * state.max_phnum];
    for (size_t j = 0; j < info->dlpi_phnum; ++j) {
      if (!found[j] && info->dlpi_phdr[j].p_type == PT_LOAD) {
        bad = true;
        printf("No mapping found for %s phdr %zu\n",
               state.dl_objs[i].dlpi_name, j);
      }
    }
  }

  if (bad) {
    puts("Mappings:");
    for (size_t i = 0; i < nmaps; ++i) {
      const struct NaClMemMappingInfo *m = &map[i];
      printf("\t[%zu] %#.8x+%#x prot %#x/%#x %s\n",
             i, m->start, m->size, m->prot, m->max_prot,
             m->vmmap_type ? "file" : "anon");
    }

    puts("Dynamic objects:");
    for (size_t i = 0; i < state.i; ++i) {
      const struct dl_phdr_info *info = &state.dl_objs[i];
      printf("\t%u phdrs, addr %#.8x in \"%s\"\n",
             info->dlpi_phnum,
             (unsigned int) info->dlpi_addr,
             info->dlpi_name);
      for (size_t j = 0; j < info->dlpi_phnum; ++j) {
        const ElfW(Phdr) *ph = &info->dlpi_phdr[j];
        if (ph->p_type == PT_LOAD) {
          printf("\t\t[%zu] %#.8x+%#x flags %#x\n",
                 j,
                 (unsigned int) (ph->p_vaddr + info->dlpi_addr),
                 (unsigned int) ph->p_memsz,
                 (unsigned int) ph->p_flags);
        }
      }
    }
  }

  ASSERT(!bad);
}

#endif

static void test_list_mappings_order_and_overlap(void) {
  printf("Testing list_mappings order and lack of overlap.\n");

  struct NaClMemMappingInfo map[0x10000];
  size_t capacity = sizeof(map) / sizeof(*map);
  size_t size;
  int result = nacl_list_mappings(map, capacity, &size);
  ASSERT_EQ(0, result);
  ASSERT_LE(size, capacity);

  for (uint32_t i = 0; i < size - 1; ++i) {
    ASSERT_LE(map[i].start + map[i].size, map[i + 1].start);
  }
}

int main(void) {
  test_list_mappings_read_write();
  test_list_mappings_bad_destination();
#if defined(__GLIBC__)
  test_list_mappings_phdrs();
#endif
  test_list_mappings_order_and_overlap();
  return 0;
}
