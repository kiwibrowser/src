// Copyright 2016 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <pthread.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/untrusted/pll_loader/pll_loader.h"


namespace {

template <typename VarType>
void CheckTlsVar(ModuleSet *modset, const char *name_of_getter,
                 int alignment, VarType expected_value) {
  auto getter = (VarType *(*)()) (uintptr_t) modset->GetSym(name_of_getter);
  ASSERT_NE(getter, NULL);
  VarType *var_ptr = getter();
  ASSERT_NE(var_ptr, NULL);
  ASSERT_EQ((uintptr_t) var_ptr & (alignment - 1), 0);
  ASSERT_EQ(*var_ptr, expected_value);
}

struct ThreadArgs {
  int *(*tls_var_getter)();
  void *tls_var_on_main_thread;
};

void *ThreadFunc(void *thread_arg) {
  ThreadArgs *args = (ThreadArgs *) thread_arg;
  int *var_ptr = args->tls_var_getter();
  ASSERT_NE(var_ptr, NULL);
  ASSERT_EQ(*var_ptr, 123);
  ASSERT_NE(var_ptr, args->tls_var_on_main_thread);
  return NULL;
}

}  // namespace

int main(int argc, char **argv) {
  if (argc != 5) {
    fprintf(stderr, "Usage: pll_loader_test <Directory path> <3 ELF files>\n");
    return 1;
  }

  const char *module_directory = argv[1];
  const char *module_a_soname = argv[2];
  const char *module_b_soname = argv[3];
  const char *module_tls_soname = argv[4];

  ModuleSet modset;
  std::vector<std::string> search_path;
  search_path.push_back(module_directory);
  modset.SetSonameSearchPath(search_path);

  // "module_a_var" should only be resolvable after we load module A.
  int *module_a_var = (int *) modset.GetSym("module_a_var");
  ASSERT_EQ(module_a_var, NULL);

  ASSERT_NE(modset.AddBySoname(module_a_soname), NULL);
  module_a_var = (int *) modset.GetSym("module_a_var");
  ASSERT_NE(module_a_var, NULL);
  ASSERT_EQ(*module_a_var, 2345);

  // Demonstrate that the same soname cannot be loaded multiple times.
  ASSERT_EQ(modset.AddBySoname(module_a_soname), NULL);

  ASSERT_NE(modset.AddBySoname(module_b_soname), NULL);
  int *module_b_var = (int *) modset.GetSym("module_b_var");
  ASSERT_NE(module_b_var, NULL);
  ASSERT_EQ(*module_b_var, 1234);

  ASSERT_NE(modset.AddBySoname(module_tls_soname), NULL);

  modset.ResolveRefs();

  // Check that "addr_of_module_a_var" has been correctly relocated to
  // point to the other module.
  int **addr_of_module_a_var = (int **) modset.GetSym("addr_of_module_a_var");
  ASSERT_NE(addr_of_module_a_var, NULL);
  ASSERT_EQ(*addr_of_module_a_var, module_a_var);

  // Test that TLS variables are instantiated and aligned properly.
  CheckTlsVar<int>(&modset, "get_tls_var1", sizeof(int), 123);
  CheckTlsVar<int *>(&modset, "get_tls_var2", sizeof(int), module_a_var);
  CheckTlsVar<int>(&modset, "get_tls_var_aligned", 256, 345);
  CheckTlsVar<int>(&modset, "get_tls_bss_var1", sizeof(int), 0);
  CheckTlsVar<int>(&modset, "get_tls_bss_var_aligned", 256, 0);
  CheckTlsVar<int>(&modset, "get_tls_var_exported1", sizeof(int), 1234);
  CheckTlsVar<int>(&modset, "get_tls_var_exported2", sizeof(int), 5678);

  // Test that TLS variables are instantiated separately for each thread.
  ThreadArgs thread_args;
  thread_args.tls_var_getter =
      (int *(*)()) (uintptr_t) modset.GetSym("get_tls_var1");
  thread_args.tls_var_on_main_thread = thread_args.tls_var_getter();
  pthread_t tid;
  int err = pthread_create(&tid, NULL, ThreadFunc, &thread_args);
  ASSERT_EQ(err, 0);
  err = pthread_join(tid, NULL);
  ASSERT_EQ(err, 0);
  // Check that the TLS var is still given the same address.  This
  // should not have been affected by launching a child thread.
  ASSERT_EQ(thread_args.tls_var_getter(), thread_args.tls_var_on_main_thread);

  return 0;
}
