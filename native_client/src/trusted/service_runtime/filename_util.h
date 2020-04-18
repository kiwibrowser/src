/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_FILENAME_UTIL_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_FILENAME_UTIL_H_

#include <stdlib.h>

#include <string>
#include <vector>

/*
 * Semantically convert the absolute path (possibly including "//", "/.", and
 * "/..") and convert it to the canonical pathname. This is done manually
 * (rather than calling a standard library "realpath") to avoid any races during
 * pathname resolution.
 *
 * @param[in] abs_path The absolute path to be verified.
 * @param[out] real_path The canonical pathname.
 * @param[out] required_subpaths Subpaths which must exist for real_path to be
 *             valid. For example, when "/foo/bar/.." is canonicalized to
 *             "/foo", this list contains "/foo/bar".
 */
void CanonicalizeAbsolutePath(const std::string &abs_path,
                              std::string *real_path,
                              std::vector<std::string> *required_subpaths);

#endif /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_FILENAME_UTIL_H_ */
