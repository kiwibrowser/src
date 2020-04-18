// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_VERSION_INFO_H_
#define UI_GL_GL_VERSION_INFO_H_

#include <set>
#include <string>
#include "base/macros.h"
#include "build/build_config.h"
#include "ui/gl/extension_set.h"
#include "ui/gl/gl_export.h"

namespace gl {

struct GL_EXPORT GLVersionInfo {
  GLVersionInfo(const char* version_str,
                const char* renderer_str,
                const ExtensionSet& exts);

  bool IsAtLeastGL(unsigned major, unsigned minor) const {
    return !is_es && (major_version > major ||
                      (major_version == major && minor_version >= minor));
  }

  bool IsLowerThanGL(unsigned major, unsigned minor) const {
    return !is_es && (major_version < major ||
                      (major_version == major && minor_version < minor));
  }

  bool IsAtLeastGLES(unsigned major, unsigned minor) const {
    return is_es && (major_version > major ||
                     (major_version == major && minor_version >= minor));
  }

  bool BehavesLikeGLES() const {
    return is_es || is_desktop_core_profile;
  }

  bool SupportsFixedType() const {
    return is_es || IsAtLeastGL(4, 1);
  }

  bool is_es;
  bool is_angle;
  bool is_d3d;
  bool is_mesa;
  bool is_swiftshader;
  unsigned major_version;
  unsigned minor_version;
  bool is_es2;
  bool is_es3;
  bool is_desktop_core_profile;
  bool is_es3_capable;
  std::string driver_vendor;
  std::string driver_version;

 private:
  void Initialize(const char* version_str,
                  const char* renderer_str,
                  const ExtensionSet& extensions);
  void ParseVersionString(const char* version_str);
  bool IsES3Capable(const ExtensionSet& extensions) const;

  DISALLOW_COPY_AND_ASSIGN(GLVersionInfo);
};

}  // namespace gl

#endif // UI_GL_GL_VERSION_INFO_H_
