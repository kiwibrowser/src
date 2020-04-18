/*
 * Copyright 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <setjmp.h>
#include <stdio.h>

int func1(jmp_buf* env, int x) {
  if (x == 0) longjmp(*env, 1);
  return 1;
}

int func2(jmp_buf* env, int x) {
  if (x == 0) longjmp(*env, 2);
  return func1(env, x - 1) + 1;
}

int func3(jmp_buf* env, int x) {
  if (x == 0) longjmp(*env, 3);
  return func2(env, x - 1) + 1;
}

int main(int argc, char** argv) {
  jmp_buf env;
  int value;

  value = setjmp(env);
  if (value == 0) {
    func3(&env, 0);
    assert(0);
  } else {
    assert(3 == value);
  }

  value = setjmp(env);
  if (value == 0) {
    func3(&env, 1);
    assert(0);
  } else {
    assert(2 == value);
  }

  value = setjmp(env);
  if (value == 0) {
    func3(&env, 2);
    assert(0);
  } else {
    assert(1 == value);
  }

  value = setjmp(env);
  assert(value == 0);
  assert(func3(&env, 3) == 3);

  return 0;
}
