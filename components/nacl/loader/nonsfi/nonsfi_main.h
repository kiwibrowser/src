// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NACL_LOADER_NONSFI_NONSFI_MAIN_H_
#define COMPONENTS_NACL_LOADER_NONSFI_NONSFI_MAIN_H_

namespace nacl {
namespace nonsfi {

// Launch NaCl with Non SFI mode. This takes the ownership of |nexe_file|.
void MainStart(int nexe_file);

}  // namespace nonsfi
}  // namespace nacl

#endif  // COMPONENTS_NACL_LOADER_NONSFI_NONSFI_MAIN_H_
