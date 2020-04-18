/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* This is an example library, for testing the ConvertToPSO pass. */

/* Test imports referenced by variables */

extern int imported_var;
extern int imported_var_addend;
extern int imported_var2;
extern int imported_var3;

/* Relocation without an addend. */
int *reloc_var = &imported_var;
/* Const relocation without an addend. */
int *const reloc_var_const = &imported_var;
/* Relocation with an addend. */
int *reloc_var_addend = &imported_var_addend + 1;
/* Const relocation with an addend. */
int *const reloc_var_const_addend = &imported_var_addend + 1;

/* Relocations in a compound initializer, with addends. */
int *reloc_var_offset[] = { 0, &imported_var2 + 100, &imported_var3 + 200, 0 };

/* Const relocations in a compound initializer, with addends. */
int *const reloc_var_const_offset[] = { 0, &imported_var + 300,
                                        &imported_var + 400, 0 };

/*
 * Local (non-imported) relocation.  Test that these still work and that
 * they don't get mixed up with relocations for imports.
 */
static int local_var = 1234;
int *local_reloc_var = &local_var;

/* Test imports referenced by functions */

int *get_imported_var(void) {
  return &imported_var;
}

int *get_imported_var_addend(void) {
  return &imported_var_addend + 1;
}

void imported_func(void);
typedef void (*func_type_t)(void);

func_type_t get_imported_func(void) {
  return imported_func;
}

/* Test exports */

int var = 2345;

int *get_var(void) {
  return &var;
}

int example_func(void) {
  return 5678;
}

/* Test aliases */

extern int var_alias __attribute__((alias("var")));

void example_func_alias(void) __attribute__((alias("example_func")));
