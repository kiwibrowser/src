// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FILE_DESCRIPTOR_INFO_IMPL_H_
#define CONTENT_BROWSER_FILE_DESCRIPTOR_INFO_IMPL_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <vector>

#include "base/files/memory_mapped_file.h"
#include "content/common/content_export.h"
#include "content/public/browser/posix_file_descriptor_info.h"

namespace content {

class PosixFileDescriptorInfoImpl : public PosixFileDescriptorInfo {
 public:
  CONTENT_EXPORT static std::unique_ptr<PosixFileDescriptorInfo> Create();

  ~PosixFileDescriptorInfoImpl() override;
  void Share(int id, base::PlatformFile fd) override;
  void ShareWithRegion(int id,
                       base::PlatformFile fd,
                       const base::MemoryMappedFile::Region& region) override;
  void Transfer(int id, base::ScopedFD fd) override;
  const base::FileHandleMappingVector& GetMapping() const override;
  base::FileHandleMappingVector GetMappingWithIDAdjustment(
      int delta) const override;
  base::PlatformFile GetFDAt(size_t i) const override;
  int GetIDAt(size_t i) const override;
  const base::MemoryMappedFile::Region& GetRegionAt(size_t i) const override;
  size_t GetMappingSize() const override;
  bool OwnsFD(base::PlatformFile file) const override;
  base::ScopedFD ReleaseFD(base::PlatformFile file) override;

 private:
  PosixFileDescriptorInfoImpl();

  void AddToMapping(int id,
                    base::PlatformFile fd,
                    const base::MemoryMappedFile::Region& region);
  bool HasID(int id) const;
  base::FileHandleMappingVector mapping_;
  // Maps the ID of a FD to the region to use for that FD, the whole file if not
  // in the map.
  std::map<int, base::MemoryMappedFile::Region> ids_to_regions_;
  std::vector<base::ScopedFD> owned_descriptors_;
};
}  // namespace content

#endif  // CONTENT_BROWSER_FILE_DESCRIPTOR_INFO_IMPL_H_
