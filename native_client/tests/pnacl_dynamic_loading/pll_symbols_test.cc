// Copyright (c) 2015 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <string.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/untrusted/pll_loader/pll_loader.h"
#include "native_client/src/untrusted/pll_loader/pll_root.h"
#include "native_client/src/untrusted/pnacl_dynloader/dynloader.h"

// This tests the symbol tables created by the ConvertToPSO pass.

namespace {

const unsigned kFormatVersion = 2;

const char *GetExportedSymbolName(const PLLRoot *root, size_t i) {
  return root->string_table + root->exported_names[i];
}

const char *GetImportedSymbolName(const PLLRoot *root, size_t i) {
  return root->string_table + root->imported_names[i];
}

const char *GetImportedTlsSymbolName(const PLLRoot *root, size_t i) {
  return root->string_table + root->imported_tls_names[i];
}

void DumpExportedSymbols(const PLLRoot *root) {
  for (size_t i = 0; i < root->export_count; i++) {
    printf("exported symbol: %s\n", GetExportedSymbolName(root, i));
  }
}

void DumpImportedSymbols(const PLLRoot *root) {
  for (size_t i = 0; i < root->import_count; i++) {
    printf("imported symbol: %s\n", GetImportedSymbolName(root, i));
  }
}

void DumpImportedTlsSymbols(const PLLRoot *root) {
  for (size_t i = 0; i < root->import_tls_count; i++) {
    printf("imported TLS symbol: %s\n", GetImportedTlsSymbolName(root, i));
  }
}

bool GetExportedSymSlow(const PLLRoot *root, const char *name, void **sym) {
  for (size_t i = 0; i < root->export_count; i++) {
    if (strcmp(GetExportedSymbolName(root, i), name) == 0) {
      *sym = root->exported_ptrs[i];
      return true;
    }
  }
  return false;
}

void VerifyHashTable(const PLLRoot *root) {
  // Confirm that each entry in hash_chains[] contains a hash
  // corresponding with the hashes of "exported_names", in order.
  for (uint32_t chain_index = 0; chain_index < root->export_count;
       chain_index++) {
    uint32_t hash = PLLModule::HashString(
        GetExportedSymbolName(root, chain_index));
    ASSERT_EQ(hash & ~1, root->hash_chains[chain_index] & ~1);
  }

  // Confirm that each entry in hash_buckets[] is either -1 or a valid index
  // into hash_chains[].
  for (uint32_t bucket_index = 0; bucket_index < root->bucket_count;
       bucket_index++) {
    int32_t chain_index = root->hash_buckets[bucket_index];
    ASSERT_GE(chain_index, -1);
    ASSERT_LT(chain_index, (int32_t) root->export_count);

    if (chain_index != -1) {
      // For each chain marked in hash_buckets[], confirm that it is terminated
      // and the hash matches the hash_buckets[] index.
      bool chain_terminated = false;
      for (; chain_index < root->export_count; chain_index++) {
        uint32_t hash = PLLModule::HashString(
            GetExportedSymbolName(root, chain_index));
        ASSERT_EQ(bucket_index, hash % root->bucket_count);
        if ((root->hash_chains[chain_index] & 1) == 1) {
          chain_terminated = true;
          break;
        }
      }
      ASSERT(chain_terminated);
    }
  }
}

void *GetExportedSym(const PLLRoot *root, const char *name) {
  // There are two possible ways to get the exported symbol. First, by manually
  // scanning all exports, and secondly, by using the exported symbol hash
  // table. Use both methods, and assert that they provide an equivalent result.
  void *sym_slow = NULL;
  void *sym_hash = NULL;
  bool sym_slow_valid = GetExportedSymSlow(root, name, &sym_slow);
  bool sym_hash_valid = PLLModule(root).GetExportedSym(name, &sym_hash);
  ASSERT_EQ(sym_slow_valid, sym_hash_valid);
  ASSERT_EQ(sym_slow, sym_hash);

  // The bloom filter errs on the side of returning "true", and may return true
  // even if the symbol is not actually exported. This means that we cannot
  // assert a "false" result from the bloom filter, since an alternative (but
  // equally valid) bloom filter may return "true" for the same input.
  uint32_t hash = PLLModule::HashString(name);
  if (sym_slow_valid) {
    ASSERT(PLLModule(root).IsMaybeExported(hash));
  }
  return sym_slow;
}

void TestImportReloc(const PLLRoot *pll_root,
                     const char *imported_sym_name,
                     int import_addend,
                     const char *dest_name,
                     int offset_from_dest) {
  printf("Checking for relocation that assigns to \"%s+%d\" "
         "the value of \"%s+%d\"\n",
         dest_name, offset_from_dest, imported_sym_name, import_addend);

  void *dest_sym_addr = GetExportedSym(pll_root, dest_name);
  ASSERT_NE(dest_sym_addr, NULL);
  uintptr_t addr_to_modify = (uintptr_t) dest_sym_addr + offset_from_dest;

  // Check the addend.
  ASSERT_EQ(*(uintptr_t *) addr_to_modify, import_addend);

  // Search for a relocation that applies to the given target address.  We
  // do not require the relocations to appear in a specific order in the
  // import list.
  bool found = false;
  for (size_t index = 0; index < pll_root->import_count; index++) {
    if (pll_root->imported_ptrs[index] == (void *) addr_to_modify) {
      ASSERT(!found);
      found = true;

      // Check name of symbol being imported.
      ASSERT_EQ(strcmp(GetImportedSymbolName(pll_root, index),
                       imported_sym_name), 0);
    }
  }
  ASSERT(found);
}

// Resolves any imports that refer to |symbol_name| with the given value.
void ResolveReferenceToSym(const PLLRoot *pll_root,
                           const char *symbol_name,
                           uintptr_t value) {
  bool found = false;
  for (size_t index = 0; index < pll_root->import_count; index++) {
    if (strcmp(GetImportedSymbolName(pll_root, index), symbol_name) == 0) {
      found = true;
      *(uintptr_t *) pll_root->imported_ptrs[index] += value;
    }
  }
  ASSERT(found);
}

const PLLRoot *LoadTranslatedPLL(const char *test_dso_file) {
  printf("Testing %s...\n", test_dso_file);
  void *pso_root;
  int err = pnacl_load_elf_file(test_dso_file, &pso_root);
  ASSERT_EQ(err, 0);
  const PLLRoot *pll_root = (PLLRoot *) pso_root;

  ASSERT_EQ(pll_root->format_version, kFormatVersion);

  VerifyHashTable(pll_root);

  // The bloom filter bitmask must be one less than a power of 2.
  ASSERT_EQ((pll_root->bloom_filter_maskwords_bitmask + 1) &
            pll_root->bloom_filter_maskwords_bitmask, 0);

  return pll_root;
}

void TestCoreFunctionality(const char *test_dso_file) {
  const PLLRoot *pll_root = LoadTranslatedPLL(test_dso_file);

  // Test dependencies.
  ASSERT_EQ(pll_root->dependencies_count, 2);
  ASSERT_EQ(strcmp(pll_root->dependencies_list, "test_pll_a.so"), 0);
  ASSERT_EQ(strcmp(pll_root->dependencies_list + strlen("test_pll_a.so") + 1,
                   "test_pll_b.so"), 0);

  // Test exports.

  DumpExportedSymbols(pll_root);

  ASSERT_EQ(GetExportedSym(pll_root, "does_not_exist"), NULL);

  int *var = (int *) GetExportedSym(pll_root, "var");
  ASSERT_NE(var, NULL);
  ASSERT_EQ(*var, 2345);

  int *(*get_var)(void) =
    (int *(*)(void)) (uintptr_t) GetExportedSym(pll_root, "get_var");
  ASSERT_NE(get_var, NULL);
  ASSERT_EQ(get_var(), var);

  void *example_func = GetExportedSym(pll_root, "example_func");
  ASSERT_NE(example_func, NULL);

  int *var_alias = (int *) GetExportedSym(pll_root, "var_alias");
  ASSERT_EQ(var_alias, var);

  void *example_func_alias = GetExportedSym(pll_root, "example_func_alias");
  ASSERT_EQ(example_func_alias, example_func);

  // For "var", "get_var", "example_func", "var_alias", and
  // "example_func_alias".
  int expected_exports = 5;

  // Test imports referenced by variables.  We can test these directly, by
  // checking that the relocations refer to the correct addresses.

  DumpImportedSymbols(pll_root);

  TestImportReloc(pll_root, "imported_var", 0, "reloc_var", 0);
  TestImportReloc(pll_root, "imported_var", 0, "reloc_var_const", 0);
  TestImportReloc(pll_root, "imported_var_addend", sizeof(int),
                  "reloc_var_addend", 0);
  TestImportReloc(pll_root, "imported_var_addend", sizeof(int),
                  "reloc_var_const_addend", 0);
  TestImportReloc(pll_root, "imported_var2", sizeof(int) * 100,
                  "reloc_var_offset", sizeof(int));
  TestImportReloc(pll_root, "imported_var3", sizeof(int) * 200,
                  "reloc_var_offset", sizeof(int) * 2);
  TestImportReloc(pll_root, "imported_var", sizeof(int) * 300,
                  "reloc_var_const_offset", sizeof(int));
  TestImportReloc(pll_root, "imported_var", sizeof(int) * 400,
                  "reloc_var_const_offset", sizeof(int) * 2);
  // For the 8 calls to TestImportReloc().
  int expected_imports = 8;
  // For "reloc_var", "reloc_var_const", "reloc_var_addend",
  // "reloc_var_addend_const", "reloc_var_offset" and "reloc_var_const_offset".
  expected_exports += 6;

  // Test that local (non-imported) relocations still work and that they
  // don't get mixed up with relocations for imports.
  int **local_reloc_var = (int **) GetExportedSym(pll_root, "local_reloc_var");
  ASSERT_EQ(**local_reloc_var, 1234);
  // For "local_reloc_var".
  expected_exports += 1;

  // Test imports referenced by functions.  We can only test these
  // indirectly, by checking that the functions' return values change when
  // we apply relocations.

  uintptr_t (*get_imported_var)() =
    (uintptr_t (*)()) (uintptr_t) GetExportedSym(pll_root, "get_imported_var");
  uintptr_t (*get_imported_var_addend)() =
    (uintptr_t (*)()) (uintptr_t) GetExportedSym(pll_root,
                                                 "get_imported_var_addend");
  uintptr_t (*get_imported_func)() =
    (uintptr_t (*)()) (uintptr_t) GetExportedSym(pll_root,
                                                 "get_imported_func");
  ASSERT_EQ(get_imported_var(), 0);
  ASSERT_EQ(get_imported_var_addend(), sizeof(int));
  ASSERT_EQ(get_imported_func(), 0);
  uintptr_t example_ptr1 = 0x100000;
  uintptr_t example_ptr2 = 0x200000;
  uintptr_t example_ptr3 = 0x300000;
  ResolveReferenceToSym(pll_root, "imported_var", example_ptr1);
  ResolveReferenceToSym(pll_root, "imported_var_addend", example_ptr2);
  ResolveReferenceToSym(pll_root, "imported_func", example_ptr3);
  ASSERT_EQ(get_imported_var(), example_ptr1);
  ASSERT_EQ(get_imported_var_addend(), example_ptr2 + sizeof(int));
  ASSERT_EQ(get_imported_func(), example_ptr3);
  // For "get_imported_var", "get_imported_var_addend" and "get_imported_func".
  expected_exports += 3;
  // For "imported_var", "imported_var_addend" and "imported_func".
  expected_imports += 3;

  ASSERT_EQ(pll_root->export_count, expected_exports);
  ASSERT_EQ(pll_root->import_count, expected_imports);
}

PLLTLSBlockGetter *g_tls_block_getter;
const uintptr_t kTlsBase = 0xabcd;  // Dummy pointer value for testing.

void *TLSGetter(PLLTLSBlockGetter *closure) {
  // Check that the PLL passes the correct pointer.
  ASSERT_EQ(closure, g_tls_block_getter);
  return (void *) kTlsBase;
}

void TestTLSVar(const PLLRoot *pll_root, const char *func_name, size_t offset) {
  printf("Testing TLS var returned by \"%s\"\n", func_name);
  auto getter_func =
    (void *(*)()) (uintptr_t) GetExportedSym(pll_root, func_name);
  ASSERT_NE(getter_func, NULL);
  ASSERT_EQ((uintptr_t) getter_func() - kTlsBase, offset);
}

PLLTLSVarGetter *g_tls_var_getter;

void *TLSVarGetter(PLLTLSVarGetter *closure) {
  // Realistically, this function would use more information from the dynamic
  // linker to actually return a pointer to the corresponding export.
  // For now, however, we're just going to return the TLS base.
  ASSERT_EQ(closure, g_tls_var_getter);
  return (void *) kTlsBase;
}

void TestImportedTLSVar(const PLLRoot *pll_root, const char *func_name,
                        const char *var_name, size_t var_index) {
  printf("Testing imported TLS var returned by \"%s\"\n", func_name);
  ASSERT_NE(pll_root->imported_tls_ptrs, NULL);
  PLLTLSVarGetter *var_getter = &pll_root->imported_tls_ptrs[var_index];
  g_tls_var_getter = var_getter;
  // The values of arg1 and arg2 don't matter at the moment, since we aren't
  // actually importing these TLS variables from a corresponding exporting
  // module.
  var_getter->func = TLSVarGetter;

  auto getter_func =
    (void *(*)()) (uintptr_t) GetExportedSym(pll_root, func_name);
  ASSERT_NE(getter_func, NULL);
  ASSERT_EQ((uintptr_t) getter_func(), kTlsBase);

  ASSERT_EQ(strcmp(GetImportedTlsSymbolName(pll_root, var_index), var_name), 0);
}

void TestTLS(const char *test_dso_file) {
  const PLLRoot *pll_root = LoadTranslatedPLL(test_dso_file);

  // Test thread-local variables (TLS).

  // Fill out the function that the module will call back to for locating
  // TLS variables.
  ASSERT_NE(pll_root->tls_block_getter, NULL);
  pll_root->tls_block_getter->func = TLSGetter;
  pll_root->tls_block_getter->arg = (void *) 0x6543;
  g_tls_block_getter = pll_root->tls_block_getter;

  // Check that TLS variables are given the correct offsets from the TLS
  // block's base, which is returned by TLSGetter.
  TestTLSVar(pll_root, "get_tls_var1", 0);
  TestTLSVar(pll_root, "get_tls_var1_addend", 4);
  TestTLSVar(pll_root, "get_tls_var2", 4);
  TestTLSVar(pll_root, "get_tls_var_aligned", 256);
  TestTLSVar(pll_root, "get_tls_bss_var1", 256 + 4);
  TestTLSVar(pll_root, "get_tls_bss_var_aligned", 512);

  // Check that the template for the TLS block has the correct size,
  // alignment and contents.
  ASSERT_EQ(pll_root->tls_template_data_size, 256 + 4);
  ASSERT_EQ(pll_root->tls_template_total_size, 512 + 4);
  ASSERT_EQ(pll_root->tls_template_alignment, 256);
  char *tls_template = (char *) pll_root->tls_template;
  ASSERT_EQ(*(int *) tls_template, 123);  // Value of tls_var1
  ASSERT_EQ(*(int *) (tls_template + 4), 0);  // Addend for tls_var2
  ASSERT_EQ(*(int *) (tls_template + 256), 345);  // Value of tls_var_aligned

  // Test TLS imports
  DumpImportedTlsSymbols(pll_root);
  ASSERT_EQ(pll_root->import_tls_count, 2);
  TestImportedTLSVar(pll_root, "get_tls_var_exported1", "tls_var_exported1", 0);
  TestImportedTLSVar(pll_root, "get_tls_var_exported2", "tls_var_exported2", 1);

  // Test TLS exports. Offsets of the TLS variables are exported, rather than
  // the TLS variables themselves.
  int exported_vars_count = 5;
  int exported_funcs_count = 8;
  ASSERT_EQ(pll_root->export_count, exported_vars_count + exported_funcs_count);
  void *symbol;
  ASSERT(PLLModule(pll_root).GetExportedSym("tls_var1", &symbol));
  ASSERT_EQ((uintptr_t) symbol, 0);
  ASSERT(PLLModule(pll_root).GetExportedSym("tls_var2", &symbol));
  ASSERT_EQ((uintptr_t) symbol, 4);
  ASSERT(PLLModule(pll_root).GetExportedSym("tls_var_aligned", &symbol));
  ASSERT_EQ((uintptr_t) symbol, 256);
  ASSERT(PLLModule(pll_root).GetExportedSym("tls_bss_var1", &symbol));
  ASSERT_EQ((uintptr_t) symbol, 260);
  ASSERT(PLLModule(pll_root).GetExportedSym("tls_bss_var_aligned", &symbol));
  ASSERT_EQ((uintptr_t) symbol, 512);
}

}  // namespace

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: pll_symbols_test <ELF file> <ELF file>\n");
    return 1;
  }

  TestCoreFunctionality(argv[1]);
  TestTLS(argv[2]);

  return 0;
}
