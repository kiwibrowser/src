// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "mojo/public/tools/fuzzers/fuzz.mojom.h"
#include "mojo/public/tools/fuzzers/fuzz_impl.h"

FuzzImpl::FuzzImpl(fuzz::mojom::FuzzInterfaceRequest request)
    : binding_(this, std::move(request)) {}

FuzzImpl::~FuzzImpl() {}

void FuzzImpl::FuzzBasic() {}

void FuzzImpl::FuzzBasicResp(FuzzBasicRespCallback callback) {
  std::move(callback).Run();
}

void FuzzImpl::FuzzBasicSyncResp(FuzzBasicSyncRespCallback callback) {
  std::move(callback).Run();
}

void FuzzImpl::FuzzArgs(fuzz::mojom::FuzzStructPtr a,
                        fuzz::mojom::FuzzStructPtr b) {}

void FuzzImpl::FuzzArgsResp(fuzz::mojom::FuzzStructPtr a,
                            fuzz::mojom::FuzzStructPtr b,
                            FuzzArgsRespCallback callback) {
  std::move(callback).Run();
}

void FuzzImpl::FuzzArgsSyncResp(fuzz::mojom::FuzzStructPtr a,
                                fuzz::mojom::FuzzStructPtr b,
                                FuzzArgsSyncRespCallback callback) {
  std::move(callback).Run();
}

void FuzzImpl::FuzzAssociated(
    fuzz::mojom::FuzzDummyInterfaceAssociatedRequest req) {
  associated_bindings_.AddBinding(this, std::move(req));
}

void FuzzImpl::Ping() {}
