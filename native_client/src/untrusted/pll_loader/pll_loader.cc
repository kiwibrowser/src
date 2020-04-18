// Copyright 2016 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "native_client/src/untrusted/pll_loader/pll_loader.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <algorithm>
#include <string>
#include <vector>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/untrusted/pnacl_dynloader/dynloader.h"

namespace {

// Array of modules, used for instantiating their TLS blocks.  This is used
// concurrently and is protected by g_modules_mutex.
std::vector<const PLLRoot *> *g_modules;

pthread_mutex_t g_modules_mutex = PTHREAD_MUTEX_INITIALIZER;

// Array of modules' TLS blocks for the current thread.  An entry may be
// NULL if no TLS block has been instantiated for the module yet on this
// thread.
__thread void **thread_modules_array = NULL;
__thread uint32_t thread_modules_array_size = 0;

void *TLSBlockBase(uint32_t module_index) {
  // Fast path: Handles the case when the TLS block has been instantiated
  // already for the module.
  size_t array_size = thread_modules_array_size;
  if (module_index < array_size) {
    void *tls_block = thread_modules_array[module_index];
    if (tls_block != NULL)
      return tls_block;
  }

  // Slow path: This allocates the TLS block for the module on demand, and
  // it may need to grow the thread's module array too.
  CHECK(pthread_mutex_lock(&g_modules_mutex) == 0);
  size_t new_size = g_modules->size();
  const PLLRoot *pll_root = (*g_modules)[module_index];
  CHECK(pthread_mutex_unlock(&g_modules_mutex) == 0);

  // Use of the memory allocator here assumes that the allocator --
  // i.e. malloc()/realloc()/posix_memalign() -- does not depend on the TLS
  // facility we are providing here.  This is true when the allocator is
  // linked into the PLL loader.  If this assumption were broken, we could
  // get re-entrancy problems such as deadlocks.  This assumption could get
  // broken if the allocator were implemented by a dynamically-loaded libc
  // (e.g. by having libc override the PLL loader's use of malloc()).

  if (module_index >= array_size) {
    // Grow the array.  We grow it to the number of currently loaded
    // modules (rather than to module_index + 1) so that we won't need to
    // grow it again unless further modules are loaded.
    void **new_array = (void **) realloc(thread_modules_array,
                                         new_size * sizeof(void *));
    CHECK(new_array != NULL);
    memset(&new_array[array_size], 0,
           (new_size - array_size) * sizeof(void *));
    thread_modules_array = new_array;
    thread_modules_array_size = new_size;
  }

  void *tls_block = PLLModule(pll_root).InstantiateTLSBlock();
  thread_modules_array[module_index] = tls_block;
  return tls_block;
}

void *TLSVarGetter(PLLTLSVarGetter *closure) {
  uint32_t module_index = (uint32_t) closure->arg1;
  uintptr_t var_offset = (uintptr_t) closure->arg2;
  uintptr_t block_base = (uintptr_t) TLSBlockBase(module_index);
  return (void *) (block_base + var_offset);
}

void *TLSBlockGetter(PLLTLSBlockGetter *closure) {
  uint32_t module_index = (uint32_t) closure->arg;
  return TLSBlockBase(module_index);
}

}  // namespace

uint32_t PLLModule::HashString(const char *sp) {
  uint32_t h = 5381;
  for (unsigned char c = *sp; c != '\0'; c = *++sp)
    h = h * 33 + c;
  return h;
}

bool PLLModule::IsMaybeExported(uint32_t hash1) {
  const int kWordSizeBits = 32;
  uint32_t hash2 = hash1 >> root_->bloom_filter_shift2;

  uint32_t word_num = (hash1 / kWordSizeBits) &
      root_->bloom_filter_maskwords_bitmask;
  uint32_t bitmask =
      (1 << (hash1 % kWordSizeBits)) | (1 << (hash2 % kWordSizeBits));
  return (root_->bloom_filter_data[word_num] & bitmask) == bitmask;
}

bool PLLModule::GetExportedSym(const char *name, void **sym) {
  uint32_t hash = HashString(name);

  // Use the bloom filter to quickly reject symbols that aren't exported.
  if (!IsMaybeExported(hash))
    return false;

  uint32_t bucket_index = hash % root_->bucket_count;
  int32_t chain_index = root_->hash_buckets[bucket_index];
  // Bucket empty -- value not found.
  if (chain_index == -1)
    return false;

  for (; chain_index < root_->export_count; chain_index++) {
    uint32_t chain_value = root_->hash_chains[chain_index];
    if ((hash & ~1) == (chain_value & ~1) &&
        strcmp(name, GetExportedSymbolName(chain_index)) == 0) {
      *sym = root_->exported_ptrs[chain_index];
      return true;
    }

    // End of chain -- value not found.
    if ((chain_value & 1) == 1)
      return false;
  }

  NaClLog(LOG_FATAL, "GetExportedSym: "
          "Bad hash table in PLL: chain not terminated\n");
  return false;
}

void *PLLModule::InstantiateTLSBlock() {
  // posix_memalign() requires its alignment arg to be at least sizeof(void *).
  size_t alignment = std::max(root_->tls_template_alignment, sizeof(void *));
  void *base;
  if (posix_memalign(&base, alignment, root_->tls_template_total_size) != 0) {
    NaClLog(LOG_FATAL, "InstantiateTLSBlock: Allocation failed\n");
  }
  memcpy(base, root_->tls_template, root_->tls_template_data_size);
  size_t bss_size = (root_->tls_template_total_size -
                     root_->tls_template_data_size);
  memset((char *) base + root_->tls_template_data_size, 0, bss_size);
  return base;
}

void PLLModule::InitializeTLS() {
  if (PLLTLSBlockGetter *tls_block_getter = root_->tls_block_getter) {
    CHECK(pthread_mutex_lock(&g_modules_mutex) == 0);
    if (g_modules == NULL)
      g_modules = new std::vector<const PLLRoot *>();
    uint32_t module_index = g_modules->size();
    g_modules->push_back(root_);
    CHECK(pthread_mutex_unlock(&g_modules_mutex) == 0);

    tls_block_getter->func = TLSBlockGetter;
    tls_block_getter->arg = (void *) module_index;
  }
}

void ModuleSet::SetSonameSearchPath(const std::vector<std::string> &dir_list) {
  search_path_ = dir_list;
}

PLLRoot *ModuleSet::AddBySoname(const char *soname) {
  if (sonames_.count(soname) != 0) {
    // This module has already been added to the ModuleSet.
    return NULL;
  }
  sonames_.insert(soname);

  // Actually load the module implied by the soname.
  for (auto path : search_path_) {
    // Appending "/" might be unnecessary, but "foo/bar" and "foo//bar" should
    // point to the same file.
    path.append("/");
    path.append(soname);
    path.append(".translated");
    struct stat buf;
    if (stat(path.c_str(), &buf) == 0) {
      return AddByFilename(path.c_str());
    }
  }

  NaClLog(LOG_FATAL, "PLL Loader cannot find shared object: %s\n", soname);
  return NULL;
}

PLLRoot *ModuleSet::AddByFilename(const char *filename) {
  void *pso_root;
  int err = pnacl_load_elf_file(filename, &pso_root);
  if (err != 0) {
    NaClLog(LOG_FATAL,
            "pll_loader could not open %s: errno=%d\n", filename, err);
  }
  PLLModule module((const PLLRoot *) pso_root);
  modules_.push_back(module);
  const char *dependencies_list = module.root()->dependencies_list;
  size_t dependencies_count = module.root()->dependencies_count;
  size_t string_offset = 0;
  for (size_t i = 0; i < dependencies_count; i++) {
    std::string dependency_filename(dependencies_list + string_offset);
    string_offset += dependency_filename.length() + 1;
    AddBySoname(dependency_filename.c_str());
  }
  return (PLLRoot *) pso_root;
}

void *ModuleSet::GetSym(const char *name) {
  for (auto &module : modules_) {
    void *sym;
    if (module.GetExportedSym(name, &sym))
      return sym;
  }
  return NULL;
}

bool ModuleSet::GetTlsSym(const char *name, uint32_t *module_id,
                          uintptr_t *offset) {
  for (auto &module : modules_) {
    void *sym;
    if (module.GetExportedSym(name, &sym)) {
      *module_id = module.module_index();
      *offset = (uintptr_t) sym;
      return true;
    }
  }
  return false;
}

void ModuleSet::ResolveRefs() {
  for (auto &module : modules_) {
    for (size_t index = 0, count = module.root()->import_count;
         index < count; ++index) {
      const char *sym_name = module.GetImportedSymbolName(index);
      uintptr_t sym_value = (uintptr_t) GetSym(sym_name);
      if (sym_value == 0) {
        NaClLog(LOG_FATAL, "Undefined symbol: \"%s\"\n", sym_name);
      }
      *(uintptr_t *) module.root()->imported_ptrs[index] += sym_value;
    }

    module.InitializeTLS();
  }

  // The resolution of imported TLS variables requires that each module has an
  // assigned "module_index". For this to be the case, all modules must have
  // called "InitializeTLS" prior to resolving imported TLS variables.
  for (auto &module : modules_) {
    for (size_t index = 0, count = module.root()->import_tls_count;
         index < count; ++index) {
      const char *sym_name = module.GetImportedTlsSymbolName(index);
      uint32_t module_index;
      uintptr_t offset;
      if (!GetTlsSym(sym_name, &module_index, &offset)) {
        NaClLog(LOG_FATAL, "Undefined TLS symbol: \"%s\"\n", sym_name);
      }
      PLLTLSVarGetter *var_getter = &module.root()->imported_tls_ptrs[index];
      var_getter->func = TLSVarGetter;
      var_getter->arg1 = (void *) module_index;
      var_getter->arg2 = (void *) offset;
    }
  }
}
