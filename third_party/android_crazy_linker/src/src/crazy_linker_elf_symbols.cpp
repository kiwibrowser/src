// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_elf_symbols.h"

#include "crazy_linker_debug.h"
#include "crazy_linker_elf_view.h"

namespace crazy {

namespace {

// Compute the ELF hash of a given symbol.
unsigned ElfHash(const char* name) {
  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(name);
  unsigned h = 0;
  while (*ptr) {
    h = (h << 4) + *ptr++;
    unsigned g = h & 0xf0000000;
    h ^= g;
    h ^= g >> 24;
  }
  return h;
}

}  // namespace

bool ElfSymbols::Init(const ElfView* view) {
  LOG("Parsing dynamic table");
  ElfView::DynamicIterator dyn(view);
  for (; dyn.HasNext(); dyn.GetNext()) {
    uintptr_t dyn_addr = dyn.GetAddress(view->load_bias());
    switch (dyn.GetTag()) {
      case DT_HASH:
        LOG("  DT_HASH addr=%p", dyn_addr);
        {
          ELF::Word* data = reinterpret_cast<ELF::Word*>(dyn_addr);
          hash_bucket_size_ = data[0];
          hash_chain_size_ = data[1];
          hash_bucket_ = data + 2;
          hash_chain_ = data + 2 + hash_bucket_size_;
        }
        break;
      case DT_STRTAB:
        LOG("  DT_STRTAB addr=%p", dyn_addr);
        string_table_ = reinterpret_cast<const char*>(dyn_addr);
        break;
      case DT_SYMTAB:
        LOG("  DT_SYMTAB addr=%p", dyn_addr);
        symbol_table_ = reinterpret_cast<const ELF::Sym*>(dyn_addr);
        break;
      default:
        ;
    }
  }
  if (symbol_table_ == NULL || string_table_ == NULL || hash_bucket_ == NULL)
    return false;

  return true;
}

const ELF::Sym* ElfSymbols::LookupByAddress(void* address,
                                            size_t load_bias) const {
  ELF::Addr elf_addr =
      reinterpret_cast<ELF::Addr>(address) - static_cast<ELF::Addr>(load_bias);

  for (size_t n = 0; n < hash_chain_size_; ++n) {
    const ELF::Sym* sym = &symbol_table_[n];
    if (sym->st_shndx != SHN_UNDEF && elf_addr >= sym->st_value &&
        elf_addr < sym->st_value + sym->st_size) {
      return sym;
    }
  }
  return NULL;
}

bool ElfSymbols::LookupNearestByAddress(void* address,
                                        size_t load_bias,
                                        const char** sym_name,
                                        void** sym_addr,
                                        size_t* sym_size) const {
  ELF::Addr elf_addr =
      reinterpret_cast<ELF::Addr>(address) - static_cast<ELF::Addr>(load_bias);

  const ELF::Sym* nearest_sym = NULL;
  size_t nearest_diff = ~size_t(0);

  for (size_t n = 0; n < hash_chain_size_; ++n) {
    const ELF::Sym* sym = &symbol_table_[n];
    if (sym->st_shndx == SHN_UNDEF)
      continue;

    if (elf_addr >= sym->st_value && elf_addr < sym->st_value + sym->st_size) {
      // This is a perfect match.
      nearest_sym = sym;
      break;
    }

    // Otherwise, compute distance.
    size_t diff;
    if (elf_addr < sym->st_value)
      diff = sym->st_value - elf_addr;
    else
      diff = elf_addr - sym->st_value - sym->st_size;

    if (diff < nearest_diff) {
      nearest_sym = sym;
      nearest_diff = diff;
    }
  }

  if (!nearest_sym)
    return false;

  *sym_name = string_table_ + nearest_sym->st_name;
  *sym_addr = reinterpret_cast<void*>(nearest_sym->st_value + load_bias);
  *sym_size = nearest_sym->st_size;
  return true;
}

const ELF::Sym* ElfSymbols::LookupByName(const char* symbol_name) const {
  unsigned hash = ElfHash(symbol_name);

  for (unsigned n = hash_bucket_[hash % hash_bucket_size_]; n != 0;
       n = hash_chain_[n]) {
    const ELF::Sym* symbol = &symbol_table_[n];
    // Check that the symbol has the appropriate name.
    if (strcmp(string_table_ + symbol->st_name, symbol_name))
      continue;
    // Ignore undefined symbols.
    if (symbol->st_shndx == SHN_UNDEF)
      continue;
    // Ignore anything that isn't a global or weak definition.
    switch (ELF_ST_BIND(symbol->st_info)) {
      case STB_GLOBAL:
      case STB_WEAK:
        return symbol;
      default:
        ;
    }
  }
  return NULL;
}

}  // namespace crazy
