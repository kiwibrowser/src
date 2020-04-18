/*
 * Copyright 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

extern int module_a_var;
extern int module_b_var;

int get_module_a_var() {
  return module_a_var;
}

int get_module_b_var() {
  return module_b_var;
}

int get_module_c_var() {
  return module_a_var - module_b_var;
}
