// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/file_proxy_wrapper.h"

namespace remoting {

// static
std::unique_ptr<FileProxyWrapper> FileProxyWrapper::Create() {
  // TODO(jarhar): Implement FileProxyWrapper for windows.
  NOTIMPLEMENTED();
  return nullptr;
}

}  // namespace remoting
