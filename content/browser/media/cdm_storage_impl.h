// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CDM_STORAGE_IMPL_H_
#define CONTENT_BROWSER_MEDIA_CDM_STORAGE_IMPL_H_

#include <set>
#include <string>

#include "base/callback_forward.h"
#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "content/public/browser/frame_service_base.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"
#include "mojo/public/cpp/bindings/strong_associated_binding_set.h"

namespace storage {
class FileSystemContext;
}

namespace content {
class CdmFileImpl;
class RenderFrameHost;

// This class implements the media::mojom::CdmStorage using the
// PluginPrivateFileSystem for backwards compatibility with CDMs running
// as a pepper plugin.
class CONTENT_EXPORT CdmStorageImpl final
    : public content::FrameServiceBase<media::mojom::CdmStorage> {
 public:

  // Check if |cdm_file_system_id| is valid.
  static bool IsValidCdmFileSystemId(const std::string& cdm_file_system_id);

  // Create a CdmStorageImpl object for |cdm_file_system_id| and bind it to
  // |request|.
  static void Create(RenderFrameHost* render_frame_host,
                     const std::string& cdm_file_system_id,
                     media::mojom::CdmStorageRequest request);

  // media::mojom::CdmStorage implementation.
  void Open(const std::string& file_name, OpenCallback callback) final;

 private:
  // File system should only be opened once, so keep track if it has already
  // been opened (or is in the process of opening). State is kError if an error
  // happens while opening the file system.
  enum class FileSystemState { kUnopened, kOpening, kOpened, kError };

  CdmStorageImpl(RenderFrameHost* render_frame_host,
                 const std::string& cdm_file_system_id,
                 scoped_refptr<storage::FileSystemContext> file_system_context,
                 media::mojom::CdmStorageRequest request);
  ~CdmStorageImpl() final;

  // Called when the file system is opened.
  void OnFileSystemOpened(base::File::Error error);

  // After the file system is opened, called to create a CdmFile object.
  void CreateCdmFile(const std::string& file_name, OpenCallback callback);

  // Called after the CdmFileImpl object has opened the file for reading.
  void OnCdmFileInitialized(std::unique_ptr<CdmFileImpl> cdm_file_impl,
                            OpenCallback callback,
                            base::File file);

  // Files are stored in the PluginPrivateFileSystem, so keep track of the
  // CDM file system ID in order to open the files in the correct context.
  const std::string cdm_file_system_id_;
  scoped_refptr<storage::FileSystemContext> file_system_context_;

  // The PluginPrivateFileSystem only needs to be opened once.
  FileSystemState file_system_state_ = FileSystemState::kUnopened;

  // As multiple calls to Open() could happen while the file system is being
  // opened asynchronously, keep track of the requests so they can be
  // processed once the file system is open.
  using PendingOpenData = std::pair<std::string, OpenCallback>;
  std::vector<PendingOpenData> pending_open_calls_;

  // Once the PluginPrivateFileSystem is opened, keep track of the URI that
  // refers to it.
  std::string file_system_root_uri_;

  // This is the child process that will actually read and write the file(s)
  // returned, and it needs permission to access the file(s).
  const int child_process_id_;

  // Keep track of all media::mojom::CdmFile bindings, as each CdmFileImpl
  // object keeps a reference to |this|. If |this| goes away unexpectedly,
  // all remaining CdmFile bindings will be closed.
  mojo::StrongAssociatedBindingSet<media::mojom::CdmFile> cdm_file_bindings_;

  base::WeakPtrFactory<CdmStorageImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CdmStorageImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CDM_STORAGE_IMPL_H_
