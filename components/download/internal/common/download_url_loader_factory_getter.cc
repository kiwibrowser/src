// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/common/download_url_loader_factory_getter.h"

#include "components/download/public/common/download_task_runner.h"

namespace download {

DownloadURLLoaderFactoryGetter::DownloadURLLoaderFactoryGetter() = default;

DownloadURLLoaderFactoryGetter::~DownloadURLLoaderFactoryGetter() = default;

void DownloadURLLoaderFactoryGetter::DeleteOnCorrectThread() const {
  if (GetIOTaskRunner() && !GetIOTaskRunner()->BelongsToCurrentThread()) {
    GetIOTaskRunner()->DeleteSoon(FROM_HERE, this);
    return;
  }
  delete this;
}

}  // namespace download
