// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/downloaded_temp_file_impl.h"

#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

// static
network::mojom::DownloadedTempFilePtr DownloadedTempFileImpl::Create(
    int child_id,
    int request_id) {
  mojo::InterfacePtr<network::mojom::DownloadedTempFile> ptr;
  mojo::MakeStrongBinding(
      std::make_unique<DownloadedTempFileImpl>(child_id, request_id),
      mojo::MakeRequest(&ptr));
  return ptr;
}

DownloadedTempFileImpl::~DownloadedTempFileImpl() {
  ResourceDispatcherHostImpl::Get()->UnregisterDownloadedTempFile(child_id_,
                                                                  request_id_);
}
DownloadedTempFileImpl::DownloadedTempFileImpl(int child_id, int request_id)
    : child_id_(child_id), request_id_(request_id) {}

}  // namespace content
