// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_DOWNLOADED_TEMP_FILE_IMPL_H_
#define CONTENT_BROWSER_LOADER_DOWNLOADED_TEMP_FILE_IMPL_H_

#include "base/macros.h"
#include "content/common/content_export.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace content {

// DownloadedTempFileImpl is created on the download_to_file mode of resource
// loading. The instance is held by the client and lives until the client
// destroys the interface, slightly after the URLLoader is destroyed.
class CONTENT_EXPORT DownloadedTempFileImpl final
    : public network::mojom::DownloadedTempFile {
 public:
  // Creates a DownloadedTempFileImpl, binds it as a strong interface, and
  // returns the interface ptr to be passed to the client.
  // That means:
  //  * The DownloadedTempFile object created here is essentially owned by the
  //    client. It keeps alive until the client destroys the other endpoint.
  static network::mojom::DownloadedTempFilePtr Create(int child_id,
                                                      int request_id);

  DownloadedTempFileImpl(int child_id, int request_id);
  ~DownloadedTempFileImpl() override;

 private:
  const int child_id_;
  const int request_id_;

  DISALLOW_COPY_AND_ASSIGN(DownloadedTempFileImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_DOWNLOADED_TEMP_FILE_IMPL_H_
