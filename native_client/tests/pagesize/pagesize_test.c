/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Check that mmap always returns page size aligned memory.
 * Check that the requested length must be a multiple of page size.
 *
 * We use random.  The seed can be specified on the command line.  If
 * not, we use the name service API to get access to the secure random
 * number source to seed the generator, outputting the seed so the
 * test is (hopefully) reproducible.
 */

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/untrusted/nacl/nacl_random.h"

/* globals */
int g_verbosity = 0;

/* default values */
static const uint32_t k_num_test_cycles = 64;
static const uint32_t k_max_mmaps_per_cycle = 16;
static const uint32_t k_max_num_pages = 128;
static const double   k_bad_size_probability = 0.10;
static const double   k_release_probability = 0.05;
static const uint32_t k_max_memory_carried_forward = (1 << 20);
/* allow at most 2**24 or 16M to be carried forward between test cycles */

static const uint32_t k_nacl_page_size = (1 << 16);
static const uint32_t k_probe_stride = (4 << 10);

/*
 * do not allow release probabilities that are too small, since we end
 * up spinning through the alloc list over and over again.
 */
static const double   k_min_release_probability = 1.0e-4;

/*
 * In this experiment, we want to allocate memory using mmap of
 * various request sizes and deallocate it in multiple test cycles.
 * The choice of memory sizes for each mmap and whether or not to
 * deallocate is probabilistic.  Essentially:
 *
 * run test cycle num_test_cycles times:
 *
 *   run a random number of allocations, up to max_mmaps_per_cycle
 *   times where each mmap may allocate a random non-zero number of
 *   64k pages up to max_num_pages (the product, if greater than some
 *   value just shy of 1G, would imply mmap failure due to address
 *   space exhaustion).  each mmap allocation may ask for a memory
 *   block the size of which is bad (i.e., not a multiple of 64k) with
 *   probability bad_size_probability.  in this case, expect either a
 *   failure with EINVAL or a rounded length is used.
 *
 *   for each mmap allocation, verify that the returned address
 *   (non-MAP_FIXED) is 64k page aligned.
 *
 *   deallocate some of the allocated memory.  do it randomly, but
 *   simply: scan a free list and deallocate each block (which
 *   corresponds to the allocation earlier -- we aren't testing for
 *   munmaps that fragment a larger allocation in this test) with
 *   probability release_probability, wrapping around as needed until
 *   the total amount of memory still allocated is less than or equal
 *   to max_memory_carried_forward.
 *
 * at end, release all mmap allocated memory.
 */

unsigned long get_good_seed(void) {
  unsigned long seed;
  size_t got = 0;
  int rc = nacl_secure_random(&seed, sizeof(seed), &got);
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(got, sizeof(seed));
  return seed;
}

struct MemInfo {
  struct MemInfo  *next;
  void            *mem_addr;
  size_t          mem_bytes;
};

struct TestState {
  uint32_t            num_test_cycles;
  uint32_t            max_mmaps_per_cycle;
  uint32_t            max_num_pages;
  double              bad_size_probability;
  double              release_probability;

  size_t              max_memory_carried_forward;
  size_t              total_bytes_allocated;

  /*
   * struct drand48_data rng_state; newlib has drand48_r, but it's
   * different from glibc's -- newlibs take an REENT object as
   * argument, as per newlib convention to hang reentrant state off of
   * an explicit object used throughout reentrant versions of libc
   * functions.
   */

  struct MemInfo      *alloc_list;
  struct MemInfo      **alloc_list_end;
};

int TestStateCtor(struct TestState  *self,
                  uint32_t          num_test_cycles,
                  uint32_t          max_mmaps_per_cycle,
                  uint32_t          max_num_pages,
                  double            bad_size_probability,
                  double            release_probability,
                  size_t            max_memory_carried_forward,
                  unsigned long     seed) {
  /* validate ctor inputs */
  if (0 == num_test_cycles ||
      0 == max_mmaps_per_cycle ||
      0 == max_num_pages ||
      bad_size_probability < 0.0 ||
      1.0 < bad_size_probability ||
      release_probability < k_min_release_probability ||
      1.0 < release_probability) {
    return 0;
  }
  self->num_test_cycles = num_test_cycles;
  self->max_mmaps_per_cycle = max_mmaps_per_cycle;
  self->max_num_pages = max_num_pages;
  self->bad_size_probability = bad_size_probability;
  self->release_probability = release_probability;
  self->max_memory_carried_forward = max_memory_carried_forward;

  self->total_bytes_allocated = 0;

  (void) srand48(seed);

  self->alloc_list = NULL;
  self->alloc_list_end = &self->alloc_list;

  return 1;
}

uint32_t RunTest(struct TestState *self) {
  uint32_t        error_count = 0;
  uint32_t        cycle;
  uint32_t        num_allocations_this_cycle;
  long int        lrand;
  uint32_t        alloc_num;
  size_t          num_bytes;
  double          drand;
  int             bad_request_size;
  uintptr_t       addr;
  struct MemInfo  *mip;
  struct MemInfo  **mipp;
  struct MemInfo  *tmp;

  for (cycle = 0; cycle < self->num_test_cycles; ++cycle) {
    if (g_verbosity > 0) {
      printf("Test Cycle %u\n", cycle);
    }

    if (self->max_mmaps_per_cycle > 1) {
      lrand = lrand48();
      num_allocations_this_cycle = (lrand % (self->max_mmaps_per_cycle - 1) +
                                    1);
    } else {
      num_allocations_this_cycle = 1;
    }

    if (g_verbosity > 0) {
      printf("Will allocate %d times\n", num_allocations_this_cycle);
    }

    for (alloc_num = 0; alloc_num < num_allocations_this_cycle; ++alloc_num) {
      do {
        lrand = lrand48();
        num_bytes = lrand % (self->max_num_pages * k_nacl_page_size);

        if (g_verbosity > 0) {
          printf("random choice: %zd (0x%zx) bytes\n", num_bytes, num_bytes);
        }

        /* clear low order bits with probabilty 1-bad_size_probability */
        drand = drand48();
        bad_request_size = (drand < self->bad_size_probability);

        if (g_verbosity > 0) {
          printf("We will%s make the size not a multipe of page size\n",
                 bad_request_size ? "" : " not");
        }

        if (bad_request_size) {
          if (0 == (num_bytes & (k_nacl_page_size - 1))) {
            ++num_bytes;  /* make sure it's bad */
          }
        } else {
          num_bytes = num_bytes & ~(k_nacl_page_size - 1);
        }
      } while (0 == num_bytes);

      if (g_verbosity > 0) {
        printf("Now will allocate %zd (0x%zx) bytes\n", num_bytes, num_bytes);
      }

      mip = malloc(sizeof *mip);
      if (0 == mip) {
        perror("pagesize_test");
        abort();
      }
      mip->next = NULL;
      mip->mem_bytes = num_bytes;
      mip->mem_addr = mmap(NULL, num_bytes, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, (off_t) 0);
      if (g_verbosity > 1) {
        printf("mmap returned %p\n", mip->mem_addr);
      }
      if (MAP_FAILED == mip->mem_addr) {
        fprintf(stderr, "mmap 0x%zx bytes failed\n", num_bytes);
        ++error_count;
        free(mip);
        continue;
      }
      if (g_verbosity > 1) {
        printf("readable test\n");
      }
      /* ensure memory region is readable */
      for (addr = (uintptr_t) mip->mem_addr;
           addr < (uintptr_t) mip->mem_addr + num_bytes;
           addr += k_probe_stride) {
        *(char volatile *) addr;
      }

      if (0 != ((uintptr_t) mip->mem_addr & (k_nacl_page_size - 1))) {
        fprintf(stderr, "address %p not page aligned\n",
                (void *) mip->mem_addr);
        ++error_count;
      }

      /* save allocation to memory list */
      *self->alloc_list_end = mip;
      self->alloc_list_end = &mip->next;
      self->total_bytes_allocated += num_bytes;
    }
    /* free most(?) of allocated memory */
    while (self->total_bytes_allocated > self->max_memory_carried_forward) {
      if (g_verbosity > 0) {
        printf("release probability %f\n", self->release_probability);
        printf("allocated %zx, max %zx\n",
               self->total_bytes_allocated, self->max_memory_carried_forward);
        printf("head of list %p\n", (void *) self->alloc_list);
      }
      for (mipp = &self->alloc_list; NULL != (mip = *mipp); mipp = &mip->next) {
        double drand = drand48();

        if (g_verbosity > 0) {
          printf("at %p, prob %f, addr %p, %zx\n", (void *) mip, drand,
                 (void *) mip->mem_addr, mip->mem_bytes);
        }
        if (drand < self->release_probability) {
          if (-1 == munmap(mip->mem_addr, mip->mem_bytes)) {
            fprintf(stderr, "munmap 0x%p 0x%zx failed\n",
                    (void *) mip->mem_addr, mip->mem_bytes);
            ++error_count;
          }
          assert(self->total_bytes_allocated >= mip->mem_bytes);
          self->total_bytes_allocated -= mip->mem_bytes;
          *mipp = mip->next;
          if (NULL == mip->next) {
            self->alloc_list_end = mipp;
          }
          free(mip);
          break;
        }
      }
    }
  }
  /* free any memory not already munmapped */
  for (mip = self->alloc_list; NULL != mip; mip = tmp) {
    if (-1 == munmap(mip->mem_addr, mip->mem_bytes)) {
      fprintf(stderr, "cleanup munmap 0x%p 0x%zx failed\n",
              (void *) mip->mem_addr, mip->mem_bytes);
      ++error_count;
    }
    tmp = mip->next;
    free(mip);
  }
  return error_count;
}

int main(int ac, char **av) {
  int               opt;
  unsigned long     seed = 0;
  int               seed_provided = 0;
  uint32_t          max_num_pages = k_max_num_pages;
  uint32_t          max_mmaps_per_cycle = k_max_mmaps_per_cycle;
  uint32_t          num_test_cycles = k_num_test_cycles;
  double            bad_size_probability = k_bad_size_probability;
  double            release_probability = k_release_probability;
  size_t            max_memory_carried_forward = k_max_memory_carried_forward;

  unsigned          num_failures = 0;

  struct TestState  tstate;

  while (-1 != (opt = getopt(ac, av, "c:m:M:n:p:r:s:v"))) {
    switch (opt) {
      case 'c':
        max_memory_carried_forward = strtoull(optarg, (char **) NULL, 0);
        break;
      case 'm':
        max_num_pages = strtoul(optarg, (char **) NULL, 0);
        break;
      case 'M':
        max_mmaps_per_cycle = strtoul(optarg, (char **) NULL, 0);
        break;
      case 'n':
        num_test_cycles = strtoul(optarg, (char **) NULL, 0);
        break;
      case 'p':
        bad_size_probability = strtod(optarg, (char **) NULL);
        break;
      case 'r':
        release_probability = strtod(optarg, (char **) NULL);
        break;
      case 's':
        seed_provided = 1;
        seed = strtoul(optarg, (char **) NULL, 0);
        break;
      case 'v':
        ++g_verbosity;
        break;
      default:
        fprintf(stderr,
                "Usage: pagesize_test [-v] [-m max_pages_per_allocation]\n"
                "       [-M mmaps_per_test_cycle] [-n num_test_cycles]\n"
                "       [-p bad_size_probability] [-r release_probability]\n"
                "       [-s seed]\n");
    }
  }
  if (!seed_provided) {
    seed = get_good_seed();
  }
  printf("seed = %lu (0x%lx)\n", seed, seed);

  if (!TestStateCtor(&tstate,
                     num_test_cycles,
                     max_mmaps_per_cycle,
                     max_num_pages,
                     bad_size_probability,
                     release_probability,
                     max_memory_carried_forward,
                     seed)) {
    fprintf(stderr, "Test State ctor failure\n");
    return 1;
  }

  if (g_verbosity > 0) {
    printf("TestStateCtor succeeded, starting tests\n");
  }

  num_failures = RunTest(&tstate);

  printf("Tests finished; %u errors\n", num_failures);
  if (0 == num_failures) {
    printf("PASSED\n");
  } else {
    printf("FAILED\n");
  }

  return num_failures;
}
