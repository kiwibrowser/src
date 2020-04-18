/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/filename_util.h"

#include <stdlib.h>

#include <string>
#include <vector>

#include "native_client/src/shared/platform/nacl_check.h"

void AppendPartsToPath(const std::vector<std::string> &parts,
                       std::string *path) {
  for (size_t i = 0; i != parts.size(); i++) {
    path->append("/");
    path->append(parts[i]);
  }
}

void CanonicalizeAbsolutePath(const std::string &abs_path,
                              std::string *real_path,
                              std::vector<std::string> *required_subpaths) {
  CHECK(abs_path[0] == '/');

  real_path->clear();
  required_subpaths->clear();
  const char *pos = abs_path.c_str();

  /* Skip over all leading slashes. */
  while (*pos == '/') {
    pos++;
  }

  size_t real_path_len = 0;
  std::vector<std::string> parts;
  bool has_trailing_slash = false;
  while (*pos) {
    /*
     * Find the end of the pathname component -- either another slash, or a NULL
     * character.
     */
    CHECK(*pos != '/');
    const char *start = pos;
    while (*pos != '/' && *pos)
      pos++;
    std::string part(start, pos);

    /* Find the start of the next pathname component, if it exists. */
    has_trailing_slash = false;
    while (*pos == '/') {
      has_trailing_slash = true;
      pos++;
    }

    /* Use '..' to delete the parent path, and ignore '.' */
    if (part == "..") {
      if (!parts.empty()) {
        std::string sub_directory;
        AppendPartsToPath(parts, &sub_directory);
        /* We are verifying a directory exists, so add the trailing slash. */
        sub_directory.append("/");
        required_subpaths->push_back(sub_directory);

        real_path_len -= parts.back().length();
        parts.pop_back();
      }
    } else if (part != ".") {
      real_path_len += part.length();
      parts.push_back(part);
    } else {
      /*
       * A path ending with '.' refers to a directory, and should have a
       * trailing slash.
       */
      has_trailing_slash = true;
    }
  }

  /* Construct the canonical absolute path from the string parts. */
  if (!parts.empty()) {
    /* Add space for all slashes. */
    real_path_len += parts.size() + (has_trailing_slash ? 1 : 0);
    real_path->reserve(real_path_len);
    AppendPartsToPath(parts, real_path);
    if (has_trailing_slash)
      real_path->append("/");
  } else {
    real_path->append("/");
  }
}
