// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/removable_storage_writer/removable_storage_writer.h"

#include <utility>

#include "base/files/file_path.h"

RemovableStorageWriter::RemovableStorageWriter(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {}

RemovableStorageWriter::~RemovableStorageWriter() = default;

void RemovableStorageWriter::Write(
    const base::FilePath& source,
    const base::FilePath& target,
    chrome::mojom::RemovableStorageWriterClientPtr client) {
  writer_.Write(source, target, std::move(client));
}

void RemovableStorageWriter::Verify(
    const base::FilePath& source,
    const base::FilePath& target,
    chrome::mojom::RemovableStorageWriterClientPtr client) {
  writer_.Verify(source, target, std::move(client));
}
