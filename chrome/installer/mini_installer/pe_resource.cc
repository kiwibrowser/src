// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/mini_installer/pe_resource.h"

PEResource::PEResource(HRSRC resource, HMODULE module)
    : resource_(resource), module_(module) {
}

PEResource::PEResource(const wchar_t* name, const wchar_t* type, HMODULE module)
    : resource_(NULL), module_(module) {
  resource_ = ::FindResource(module, name, type);
}

bool PEResource::IsValid() {
  return (NULL != resource_);
}

size_t PEResource::Size() {
  return ::SizeofResource(module_, resource_);
}

bool PEResource::WriteToDisk(const wchar_t* full_path) {
  // Resource handles are not real HGLOBALs so do not attempt to close them.
  // Windows frees them whenever there is memory pressure.
  HGLOBAL data_handle = ::LoadResource(module_, resource_);
  if (NULL == data_handle) {
    return false;
  }
  void* data = ::LockResource(data_handle);
  if (NULL == data) {
    return false;
  }
  size_t resource_size = Size();
  HANDLE out_file = ::CreateFile(full_path, GENERIC_WRITE, 0, NULL,
                                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (INVALID_HANDLE_VALUE == out_file) {
    return false;
  }
  DWORD written = 0;
  if (!::WriteFile(out_file, data, static_cast<DWORD>(resource_size),
                   &written, NULL)) {
      ::CloseHandle(out_file);
      return false;
  }
  return ::CloseHandle(out_file) ? true : false;
}
