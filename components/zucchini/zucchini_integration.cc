// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zucchini/zucchini_integration.h"

#include <utility>

#include "base/logging.h"
#include "components/zucchini/buffer_view.h"
#include "components/zucchini/mapped_file.h"
#include "components/zucchini/patch_reader.h"

namespace zucchini {

namespace {

struct FileNames {
  FileNames() : is_dummy(true) {
    // Use fake names.
    old_name = old_name.AppendASCII("old_name");
    patch_name = patch_name.AppendASCII("patch_name");
    new_name = new_name.AppendASCII("new_name");
  }

  FileNames(const base::FilePath& old_name,
            const base::FilePath& patch_name,
            const base::FilePath& new_name)
      : old_name(old_name),
        patch_name(patch_name),
        new_name(new_name),
        is_dummy(false) {}

  base::FilePath old_name;
  base::FilePath patch_name;
  base::FilePath new_name;

  // A flag to decide whether the filenames are only for error output.
  const bool is_dummy;
};

status::Code ApplyCommon(base::File&& old_file_handle,
                         base::File&& patch_file_handle,
                         base::File&& new_file_handle,
                         const FileNames& names,
                         bool force_keep) {
  MappedFileReader patch_file(std::move(patch_file_handle));
  if (patch_file.HasError()) {
    LOG(ERROR) << "Error with file " << names.patch_name.value() << ": "
               << patch_file.error();
    return status::kStatusFileReadError;
  }

  auto patch_reader =
      zucchini::EnsemblePatchReader::Create(patch_file.region());
  if (!patch_reader.has_value()) {
    LOG(ERROR) << "Error reading patch header.";
    return status::kStatusPatchReadError;
  }

  MappedFileReader old_file(std::move(old_file_handle));
  if (old_file.HasError()) {
    LOG(ERROR) << "Error with file " << names.old_name.value() << ": "
               << old_file.error();
    return status::kStatusFileReadError;
  }

  zucchini::PatchHeader header = patch_reader->header();
  // By default, delete output on destruction, to avoid having lingering files
  // in case of a failure. On Windows deletion can be done by the OS.
  base::FilePath file_path;
  if (!names.is_dummy)
    file_path = base::FilePath(names.new_name);

  MappedFileWriter new_file(file_path, std::move(new_file_handle),
                            header.new_size);
  if (new_file.HasError()) {
    LOG(ERROR) << "Error with file " << names.new_name.value() << ": "
               << new_file.error();
    return status::kStatusFileWriteError;
  }

  if (force_keep)
    new_file.Keep();

  zucchini::status::Code result =
      zucchini::Apply(old_file.region(), *patch_reader, new_file.region());
  if (result != status::kStatusSuccess) {
    LOG(ERROR) << "Fatal error encountered while applying patch.";
    return result;
  }

  // Successfully patch |new_file|. Explicitly request file to be kept.
  if (!new_file.Keep())
    return status::kStatusFileWriteError;
  return status::kStatusSuccess;
}

}  // namespace

status::Code Apply(base::File&& old_file_handle,
                   base::File&& patch_file_handle,
                   base::File&& new_file_handle,
                   bool force_keep) {
  const FileNames file_names;
  return ApplyCommon(std::move(old_file_handle), std::move(patch_file_handle),
                     std::move(new_file_handle), file_names, force_keep);
}

status::Code Apply(const base::FilePath& old_path,
                   const base::FilePath& patch_path,
                   const base::FilePath& new_path,
                   bool force_keep) {
  using base::File;
  File old_file(old_path, File::FLAG_OPEN | File::FLAG_READ);
  File patch_file(patch_path, File::FLAG_OPEN | File::FLAG_READ);
  File new_file(new_path, File::FLAG_CREATE_ALWAYS | File::FLAG_READ |
                              File::FLAG_WRITE | File::FLAG_SHARE_DELETE |
                              File::FLAG_CAN_DELETE_ON_CLOSE);
  const FileNames file_names(old_path, patch_path, new_path);
  return ApplyCommon(std::move(old_file), std::move(patch_file),
                     std::move(new_file), file_names, force_keep);
}

}  // namespace zucchini
