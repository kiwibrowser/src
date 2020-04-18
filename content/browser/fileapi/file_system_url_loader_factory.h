// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FILEAPI_FILE_SYSTEM_URL_LOADER_FACTORY_H_
#define CONTENT_BROWSER_FILEAPI_FILE_SYSTEM_URL_LOADER_FACTORY_H_

#include <memory>
#include <string>

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "storage/browser/fileapi/file_system_context.h"

namespace content {

class RenderFrameHost;

// Create a URLLoaderFactory to serve filesystem: requests from the given
// |file_system_context| and |storage_domain|.
CONTENT_EXPORT std::unique_ptr<network::mojom::URLLoaderFactory>
CreateFileSystemURLLoaderFactory(
    RenderFrameHost* render_frame_host,
    bool is_navigation,
    scoped_refptr<storage::FileSystemContext> file_system_context,
    const std::string& storage_domain);

}  // namespace content

#endif  // CONTENT_BROWSER_FILEAPI_FILE_SYSTEM_URL_LOADER_FACTORY_H_
