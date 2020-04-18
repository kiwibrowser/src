// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_REMOVABLE_STORAGE_WRITER_REMOVABLE_STORAGE_WRITER_H_
#define CHROME_SERVICES_REMOVABLE_STORAGE_WRITER_REMOVABLE_STORAGE_WRITER_H_

#include <memory>

#include "base/macros.h"
#include "chrome/services/removable_storage_writer/public/mojom/removable_storage_writer.mojom.h"
#include "chrome/utility/image_writer/image_writer_handler.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace base {
class FilePath;
}

class RemovableStorageWriter : public chrome::mojom::RemovableStorageWriter {
 public:
  explicit RemovableStorageWriter(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~RemovableStorageWriter() override;

 private:
  // mojom::RemovableStorageWriter implementation:
  void Write(const base::FilePath& source,
             const base::FilePath& target,
             chrome::mojom::RemovableStorageWriterClientPtr client) override;

  void Verify(const base::FilePath& source,
              const base::FilePath& target,
              chrome::mojom::RemovableStorageWriterClientPtr client) override;

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  image_writer::ImageWriterHandler writer_;

  DISALLOW_COPY_AND_ASSIGN(RemovableStorageWriter);
};

#endif  // CHROME_SERVICES_REMOVABLE_STORAGE_WRITER_REMOVABLE_STORAGE_WRITER_H_
