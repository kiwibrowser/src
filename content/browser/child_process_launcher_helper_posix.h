// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_CHILD_PROCESS_LAUNCHER_HELPER_POSIX_H_
#define CONTENT_BROWSER_CHILD_PROCESS_LAUNCHER_HELPER_POSIX_H_

#include <memory>

#include "base/files/file.h"
#include "base/files/memory_mapped_file.h"
#include "services/catalog/public/cpp/manifest_parsing_util.h"

namespace base {
class CommandLine;
class FilePath;
}  // namespace base

namespace mojo {
namespace edk {
struct InternalPlatformHandle;
}  // namespace mojo
}  // namespace edk

// Contains the common functionalities between the various POSIX child process
// launcher implementations.

namespace content {

class PosixFileDescriptorInfo;

namespace internal {

std::unique_ptr<PosixFileDescriptorInfo> CreateDefaultPosixFilesToMap(
    int child_process_id,
    const mojo::edk::InternalPlatformHandle& mojo_client_handle,
    bool include_service_required_files,
    const std::string& process_type,
    base::CommandLine* command_line);

// Called by the service manager to register the files that should be mapped for
// a service in the child process.
void SetFilesToShareForServicePosix(const std::string& service_name,
                                    catalog::RequiredFileMap required_files);

// Called from unit_tests in order to reset all previously registered files.
void ResetFilesToShareForTestingPosix();

// Opens the file in read mode at the given path. Note that the path should be
// relative and the way it is resolved is platform specific.
// |region| is set to the region of the file that should be read.
base::File OpenFileToShare(const base::FilePath& path,
                           base::MemoryMappedFile::Region* region);

}  // namespace internal

}  // namespace content

#endif  // CONTENT_BROWSER_CHILD_PROCESS_LAUNCHER_HELPER_POSIX_H_
