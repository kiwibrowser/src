// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ZUCCHINI_ZUCCHINI_INTEGRATION_H_
#define COMPONENTS_ZUCCHINI_ZUCCHINI_INTEGRATION_H_

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "components/zucchini/zucchini.h"

namespace zucchini {

// Applies the patch in |patch_file| to the bytes in |old_file| and writes the
// result to |new_file|. Since this uses memory mapped files, crashes are
// expected in case of I/O errors. On Windows, |new_file| is kept iff returned
// code is kStatusSuccess or if |force_keep == true|, and is deleted otherwise.
// For UNIX systems the caller needs to do cleanup since it has ownership of the
// base::File params and Zucchini has no knowledge of which base::FilePath to
// delete.
status::Code Apply(base::File&& old_file,
                   base::File&& patch_file,
                   base::File&& new_file,
                   bool force_keep = false);

// Applies the patch in |patch_path| to the bytes in |old_path| and writes the
// result to |new_path|. Since this uses memory mapped files, crashes are
// expected in case of I/O errors. |new_path| is kept iff returned code is
// kStatusSuccess or if |force_keep == true|, and is deleted otherwise.
status::Code Apply(const base::FilePath& old_path,
                   const base::FilePath& patch_path,
                   const base::FilePath& new_path,
                   bool force_keep = false);

}  // namespace zucchini

#endif  // COMPONENTS_ZUCCHINI_ZUCCHINI_INTEGRATION_H_
