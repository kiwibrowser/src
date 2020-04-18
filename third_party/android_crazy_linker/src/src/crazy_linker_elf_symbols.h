// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_ELF_SYMBOLS_H
#define CRAZY_LINKER_ELF_SYMBOLS_H

#include <string.h>

#include "elf_traits.h"

namespace crazy {

class ElfView;

// An ElfSymbols instance holds information about symbols in a mapped ELF
// binary.
class ElfSymbols {
 public:
  ElfSymbols() { ::memset(this, 0, sizeof(*this)); }
  ~ElfSymbols() {}

  bool Init(const ElfView* view);

  const ELF::Sym* LookupByName(const char* symbol_name) const;

  const ELF::Sym* LookupById(size_t symbol_id) const {
    return &symbol_table_[symbol_id];
  }

  const ELF::Sym* LookupByAddress(void* address, size_t load_bias) const;

  // Returns true iff symbol with id |symbol_id| is weak.
  bool IsWeakById(size_t symbol_id) const {
    return ELF_ST_BIND(symbol_table_[symbol_id].st_info) == STB_WEAK;
  }

  const char* LookupNameById(size_t symbol_id) const {
    const ELF::Sym* sym = LookupById(symbol_id);
    if (!sym)
      return NULL;
    return string_table_ + sym->st_name;
  }

  void* LookupAddressByName(const char* symbol_name, size_t load_bias) const {
    const ELF::Sym* sym = LookupByName(symbol_name);
    if (!sym)
      return NULL;
    return reinterpret_cast<void*>(load_bias + sym->st_value);
  }

  bool LookupNearestByAddress(void* address,
                              size_t load_bias,
                              const char** sym_name,
                              void** sym_addr,
                              size_t* sym_size) const;

  const char* GetStringById(size_t str_id) const {
    return string_table_ + str_id;
  }

  // TODO(digit): Remove this once ElfRelocator is gone.
  const ELF::Sym* symbol_table() const { return symbol_table_; }
  const char* string_table() const { return string_table_; }

 private:
  const ELF::Sym* symbol_table_;
  const char* string_table_;
  ELF::Word* hash_bucket_;
  size_t hash_bucket_size_;
  ELF::Word* hash_chain_;
  size_t hash_chain_size_;
};

}  // namespace crazy

#endif  // CRAZY_LINKER_ELF_SYMBOLS_H
