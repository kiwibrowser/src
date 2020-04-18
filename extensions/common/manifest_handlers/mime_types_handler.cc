// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/manifest_handlers/mime_types_handler.h"

#include <stddef.h>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/common/webplugininfo.h"
#include "extensions/common/constants.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_constants.h"

namespace keys = extensions::manifest_keys;
namespace errors = extensions::manifest_errors;

namespace {

const char* const kMIMETypeHandlersWhitelist[] = {
    extension_misc::kPdfExtensionId,
    extension_misc::kQuickOfficeComponentExtensionId,
    extension_misc::kQuickOfficeInternalExtensionId,
    extension_misc::kQuickOfficeExtensionId,
    extension_misc::kMimeHandlerPrivateTestExtensionId};

constexpr SkColor kPdfExtensionBackgroundColor = SkColorSetRGB(82, 86, 89);
constexpr SkColor kQuickOfficeExtensionBackgroundColor =
    SkColorSetRGB(241, 241, 241);

// Stored on the Extension.
struct MimeTypesHandlerInfo : public extensions::Extension::ManifestData {
  MimeTypesHandler handler_;

  MimeTypesHandlerInfo();
  ~MimeTypesHandlerInfo() override;
};

MimeTypesHandlerInfo::MimeTypesHandlerInfo() {
}

MimeTypesHandlerInfo::~MimeTypesHandlerInfo() {
}

}  // namespace

// static
std::vector<std::string> MimeTypesHandler::GetMIMETypeWhitelist() {
  std::vector<std::string> whitelist;
  for (size_t i = 0; i < arraysize(kMIMETypeHandlersWhitelist); ++i)
    whitelist.push_back(kMIMETypeHandlersWhitelist[i]);
  return whitelist;
}

MimeTypesHandler::MimeTypesHandler() {
}

MimeTypesHandler::~MimeTypesHandler() {
}

void MimeTypesHandler::AddMIMEType(const std::string& mime_type) {
  mime_type_set_.insert(mime_type);
}

bool MimeTypesHandler::CanHandleMIMEType(const std::string& mime_type) const {
  return mime_type_set_.find(mime_type) != mime_type_set_.end();
}

bool MimeTypesHandler::HasPlugin() const {
  return !handler_url_.empty();
}

SkColor MimeTypesHandler::GetBackgroundColor() const {
  if (extension_id_ == extension_misc::kPdfExtensionId) {
    return kPdfExtensionBackgroundColor;
  }
  if (extension_id_ == extension_misc::kQuickOfficeExtensionId ||
      extension_id_ == extension_misc::kQuickOfficeInternalExtensionId ||
      extension_id_ == extension_misc::kQuickOfficeComponentExtensionId) {
    return kQuickOfficeExtensionBackgroundColor;
  }
  return content::WebPluginInfo::kDefaultBackgroundColor;
}

base::FilePath MimeTypesHandler::GetPluginPath() const {
  // TODO(raymes): Storing the extension URL in a base::FilePath is really
  // nasty. We should probably just use the extension ID as the placeholder path
  // instead.
  return base::FilePath::FromUTF8Unsafe(
      std::string(extensions::kExtensionScheme) + "://" + extension_id_ + "/");
}

// static
MimeTypesHandler* MimeTypesHandler::GetHandler(
    const extensions::Extension* extension) {
  MimeTypesHandlerInfo* info = static_cast<MimeTypesHandlerInfo*>(
      extension->GetManifestData(keys::kMimeTypesHandler));
  if (info)
    return &info->handler_;
  return NULL;
}

MimeTypesHandlerParser::MimeTypesHandlerParser() {
}

MimeTypesHandlerParser::~MimeTypesHandlerParser() {
}

bool MimeTypesHandlerParser::Parse(extensions::Extension* extension,
                                   base::string16* error) {
  const base::ListValue* mime_types_value = NULL;
  if (!extension->manifest()->GetList(keys::kMIMETypes,
                                      &mime_types_value)) {
    *error = base::ASCIIToUTF16(errors::kInvalidMimeTypesHandler);
    return false;
  }

  std::unique_ptr<MimeTypesHandlerInfo> info(new MimeTypesHandlerInfo);
  info->handler_.set_extension_id(extension->id());
  for (size_t i = 0; i < mime_types_value->GetSize(); ++i) {
    std::string filter;
    if (!mime_types_value->GetString(i, &filter)) {
      *error = base::ASCIIToUTF16(errors::kInvalidMIMETypes);
      return false;
    }
    info->handler_.AddMIMEType(filter);
  }

  std::string mime_types_handler;
  if (extension->manifest()->GetString(keys::kMimeTypesHandler,
                                       &mime_types_handler)) {
    info->handler_.set_handler_url(mime_types_handler);
  }

  extension->SetManifestData(keys::kMimeTypesHandler, std::move(info));
  return true;
}

base::span<const char* const> MimeTypesHandlerParser::Keys() const {
  static constexpr const char* kKeys[] = {keys::kMIMETypes,
                                          keys::kMimeTypesHandler};
  return kKeys;
}
