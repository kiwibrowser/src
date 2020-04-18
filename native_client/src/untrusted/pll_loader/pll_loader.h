// Copyright 2016 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_PLL_LOADER_PLL_LOADER_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_PLL_LOADER_PLL_LOADER_H_ 1

#include <string>
#include <unordered_set>
#include <vector>

#include "native_client/src/untrusted/pll_loader/pll_root.h"

// This helper class wraps the PLLRoot data structure and provides methods
// for accessing parts of PLLRoot.
class PLLModule {
 public:
  explicit PLLModule(const PLLRoot *root) : root_(root) {}

  static uint32_t HashString(const char *sp);

  const PLLRoot *root() { return root_; }

  uint32_t module_index() { return (uint32_t) root_->tls_block_getter->arg; }

  const char *GetExportedSymbolName(size_t i) {
    return root_->string_table + root_->exported_names[i];
  }

  const char *GetImportedSymbolName(size_t i) {
    return root_->string_table + root_->imported_names[i];
  }

  const char *GetImportedTlsSymbolName(size_t i) {
    return root_->string_table + root_->imported_tls_names[i];
  }

  // If this function returns "false", the symbol is definitely not exported.
  // Otherwise, the symbol may or may not be exported. This is public so the
  // bloom filter can be tested.
  bool IsMaybeExported(uint32_t hash);

  // Given the name of a symbol, sets *sym to the exported value of that symbol.
  // Returns whether a symbol is successfully found.
  bool GetExportedSym(const char *name, void **sym);

  void *InstantiateTLSBlock();
  void InitializeTLS();

 private:
  const PLLRoot *root_;
};

// ModuleSet represents a set of loaded PLLs.
class ModuleSet {
 public:
  // Load a PLL by filename. Does not add the filename to a known set of loaded
  // modules, and will not de-duplicate loading modules.
  // Returns a pointer to the PLL's pso_root.
  PLLRoot *AddByFilename(const char *filename);

  // Change the search path used by the linker when looking for PLLs by soname.
  void SetSonameSearchPath(const std::vector<std::string> &dir_list);

  // Load a PLL by soname and add the soname to the set of loaded modules.
  // Returns a pointer to the PLL's pso_root if it is loaded, NULL otherwise.
  PLLRoot *AddBySoname(const char *soname);

  // Looks up a symbol in the set of modules.  This does a linear search of
  // the modules, in the order that they were added using AddByFilename().
  // This function is intended for non-TLS variables, though it currently won't
  // return any error if used on a TLS variable.
  // Returns NULL when symbol is not found.
  void *GetSym(const char *name);

  // Same as GetSym, but returns whether the symbol was found as a boolean.
  // This is intended for TLS variables, but it currently doesn't (and can't)
  // check whether it was used on a variable of the correct type.
  // Rather than returning the address of the symbol, sets *module_id to the
  // module it was found in and sets *offset to the offset in the TLS block at
  // which it was found.
  bool GetTlsSym(const char *name, uint32_t *module_id, uintptr_t *offset);

  // Applies relocations to the modules, resolving references between them.
  void ResolveRefs();

 private:
  // The search path used to look for "sonames".
  std::vector<std::string> search_path_;
  // An unordered set of "sonames" (to see if a module has been loaded).
  std::unordered_set<std::string> sonames_;
  std::vector<PLLModule> modules_;
};

#endif
