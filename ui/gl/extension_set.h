// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_EXTENSION_SET_H_
#define UI_GL_EXTENSION_SET_H_

#include "base/containers/flat_set.h"
#include "base/strings/string_piece.h"
#include "ui/gl/gl_export.h"

namespace gl {

using ExtensionSet = base::flat_set<base::StringPiece>;

GL_EXPORT ExtensionSet
MakeExtensionSet(const base::StringPiece& extensions_string);

GL_EXPORT bool HasExtension(const ExtensionSet& extension_set,
                            const base::StringPiece& extension);

template <size_t N>
inline bool HasExtension(const ExtensionSet& extension_set,
                         const char (&extension)[N]) {
  return HasExtension(extension_set, base::StringPiece(extension, N - 1));
}

GL_EXPORT std::string MakeExtensionString(const ExtensionSet& extension_set);

}  // namespace gl

#endif  // UI_GL_EXTENSION_SET_H_
