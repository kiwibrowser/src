// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webui/shared_resources_data_source.h"

#include <stddef.h>

#include "base/containers/hash_tables.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_client.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/js/grit/mojo_bindings_resources.h"
#include "mojo/public/js/grit/mojo_bindings_resources_map.h"
#include "ui/base/layout.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/resources/grit/webui_resources.h"
#include "ui/resources/grit/webui_resources_map.h"

#if defined(OS_WIN)
#include "base/strings/utf_string_conversions.h"
#endif

namespace content {

namespace {

struct IdrGzipped {
  int idr;
  bool gzipped;
};
using ResourcesMap = base::hash_map<std::string, IdrGzipped>;

// TODO(rkc): Once we have a separate source for apps, remove '*/apps/' aliases.
const char* const kPathAliases[][2] = {
    {"../../../third_party/polymer/v1_0/components-chromium/", "polymer/v1_0/"},
    {"../../../third_party/web-animations-js/sources/",
     "polymer/v1_0/web-animations-js/"},
    {"../../views/resources/default_100_percent/common/", "images/apps/"},
    {"../../views/resources/default_200_percent/common/", "images/2x/apps/"},
    {"../../webui/resources/cr_components/", "cr_components/"},
    {"../../webui/resources/cr_elements/", "cr_elements/"},
};

void AddResource(const std::string& path,
                 int resource_id,
                 bool gzipped,
                 ResourcesMap* resources_map) {
  IdrGzipped idr_gzipped = {resource_id, gzipped};
  if (!resources_map->insert(std::make_pair(path, idr_gzipped)).second)
    NOTREACHED() << "Redefinition of '" << path << "'";
}

const ResourcesMap* CreateResourcesMap() {
  ResourcesMap* result = new ResourcesMap();
  for (size_t i = 0; i < kWebuiResourcesSize; ++i) {
    const auto& resource = kWebuiResources[i];
    AddResource(resource.name, resource.value, resource.gzipped, result);
    for (const char* const (&alias)[2] : kPathAliases) {
      if (base::StartsWith(resource.name, alias[0],
                           base::CompareCase::SENSITIVE)) {
        std::string resource_name(resource.name);
        AddResource(alias[1] + resource_name.substr(strlen(alias[0])),
                    resource.value, resource.gzipped, result);
      }
    }
  }
  for (size_t i = 0; i < kMojoBindingsResourcesSize; ++i) {
    const auto& resource = kMojoBindingsResources[i];
    if (resource.value == IDR_MOJO_BINDINGS_JS) {
      AddResource("js/mojo_bindings.js", resource.value, resource.gzipped,
                  result);
      break;
    }
  }
  return result;
}

const ResourcesMap& GetResourcesMap() {
  // This pointer will be intentionally leaked on shutdown.
  static const ResourcesMap* resources_map = CreateResourcesMap();
  return *resources_map;
}

int GetIdrForPath(const std::string& path) {
  const ResourcesMap& resources_map = GetResourcesMap();
  auto it = resources_map.find(path);
  return it != resources_map.end() ? it->second.idr : -1;
}

}  // namespace

SharedResourcesDataSource::SharedResourcesDataSource() {
}

SharedResourcesDataSource::~SharedResourcesDataSource() {
}

std::string SharedResourcesDataSource::GetSource() const {
  return kChromeUIResourcesHost;
}

void SharedResourcesDataSource::StartDataRequest(
    const std::string& path,
    const ResourceRequestInfo::WebContentsGetter& wc_getter,
    const URLDataSource::GotDataCallback& callback) {
  int idr = GetIdrForPath(path);
  DCHECK_NE(-1, idr) << " path: " << path;
  scoped_refptr<base::RefCountedMemory> bytes;

  if (idr == IDR_WEBUI_CSS_TEXT_DEFAULTS) {
    std::string css = webui::GetWebUiCssTextDefaults();
    bytes = base::RefCountedString::TakeString(&css);
  } else if (idr == IDR_WEBUI_CSS_TEXT_DEFAULTS_MD) {
    std::string css = webui::GetWebUiCssTextDefaultsMd();
    bytes = base::RefCountedString::TakeString(&css);
  } else {
    bytes = GetContentClient()->GetDataResourceBytes(idr);
  }

  callback.Run(bytes.get());
}

bool SharedResourcesDataSource::AllowCaching() const {
  // Should not be cached to reflect dynamically-generated contents that may
  // depend on the current locale.
  return false;
}

std::string SharedResourcesDataSource::GetMimeType(
    const std::string& path) const {
  if (path.empty())
    return "text/html";

#if defined(OS_WIN)
  base::FilePath file(base::UTF8ToWide(path));
  std::string extension = base::WideToUTF8(file.FinalExtension());
#else
  base::FilePath file(path);
  std::string extension = file.FinalExtension();
#endif

  if (!extension.empty())
    extension.erase(0, 1);

  if (extension == "html")
    return "text/html";

  if (extension == "css")
    return "text/css";

  if (extension == "js")
    return "application/javascript";

  if (extension == "png")
    return "image/png";

  if (extension == "gif")
    return "image/gif";

  if (extension == "svg")
    return "image/svg+xml";

  if (extension == "woff2")
    return "application/font-woff2";

  NOTREACHED() << path;
  return "text/plain";
}

scoped_refptr<base::SingleThreadTaskRunner>
SharedResourcesDataSource::TaskRunnerForRequestPath(
    const std::string& path) const {
  int idr = GetIdrForPath(path);
  if (idr == IDR_WEBUI_CSS_TEXT_DEFAULTS ||
      idr == IDR_WEBUI_CSS_TEXT_DEFAULTS_MD) {
    // Use UI thread to load CSS since its construction touches non-thread-safe
    // gfx::Font names in ui::ResourceBundle.
    return BrowserThread::GetTaskRunnerForThread(BrowserThread::UI);
  }

  return nullptr;
}

std::string
SharedResourcesDataSource::GetAccessControlAllowOriginForOrigin(
    const std::string& origin) const {
  // For now we give access only for "chrome://*" origins.
  // According to CORS spec, Access-Control-Allow-Origin header doesn't support
  // wildcards, so we need to set its value explicitly by passing the |origin|
  // back.
  std::string allowed_origin_prefix = kChromeUIScheme;
  allowed_origin_prefix += "://";
  if (!base::StartsWith(origin, allowed_origin_prefix,
                        base::CompareCase::SENSITIVE)) {
    return "null";
  }
  return origin;
}

bool SharedResourcesDataSource::IsGzipped(const std::string& path) const {
  auto it = GetResourcesMap().find(path);
  DCHECK(it != GetResourcesMap().end()) << "missing shared resource: " << path;
  return it != GetResourcesMap().end() ? it->second.gzipped : false;
}

}  // namespace content
