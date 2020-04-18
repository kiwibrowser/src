// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_PATCH_FILE_PATCHER_IMPL_H_
#define COMPONENTS_SERVICES_PATCH_FILE_PATCHER_IMPL_H_

#include <memory>

#include "base/files/file.h"
#include "base/macros.h"
#include "components/services/patch/public/interfaces/file_patcher.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace patch {

class FilePatcherImpl : public mojom::FilePatcher {
 public:
  explicit FilePatcherImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~FilePatcherImpl() override;

 private:
  // patch::mojom::FilePatcher:
  void PatchFileBsdiff(base::File input_file,
                       base::File patch_file,
                       base::File output_file,
                       PatchFileBsdiffCallback callback) override;
  void PatchFileCourgette(base::File input_file,
                          base::File patch_file,
                          base::File output_file,
                          PatchFileCourgetteCallback callback) override;

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(FilePatcherImpl);
};

}  // namespace patch

#endif  // COMPONENTS_SERVICES_PATCH_FILE_PATCHER_IMPL_H_
