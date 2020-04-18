// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "callback.h"

void Foo(base::OnceClosure) {}

void Test() {
  base::OnceClosure cb = base::BindOnce([] {});
  Foo(base::BindOnce([] {}));

  using namespace base;

  OnceClosure cb2 = BindOnce([] {});
  Foo(BindOnce([] {}));

  Closure cb3 = base::Bind([] {});
}
