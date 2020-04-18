// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Author: Ken Chen <kenchen@google.com>
//
// Library support to remap process executable elf segment with hugepages.
//
// ReloadElfTextInHugePages() will search for an ELF executable segment,
// and remap it using hugepage.

#ifndef CHROMEOS_HUGEPAGE_TEXT_HUGEPAGE_TEXT_H_
#define CHROMEOS_HUGEPAGE_TEXT_HUGEPAGE_TEXT_H_

#include <string>
#include "chromeos/chromeos_export.h"

#if defined(__clang__) || defined(__GNUC__)
#define ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#else
#define ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif

namespace chromeos {

// This function will scan ELF
// segments and attempt to remap text segment from small page to hugepage.
// When this function returns, text segments that are naturally aligned on
// hugepage size will be backed by hugepages.  In the event of errors, the
// remapping operation will be aborted and rolled back, e.g. they are all
// soft fail.
CHROMEOS_EXPORT extern void ReloadElfTextInHugePages(void);
}  // namespace chromeos

#endif  // CHROMEOS_HUGEPAGE_TEXT_HUGEPAGE_TEXT_H_
