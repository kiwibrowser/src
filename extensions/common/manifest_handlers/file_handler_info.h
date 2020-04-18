// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_MANIFEST_HANDLERS_FILE_HANDLER_INFO_H_
#define EXTENSIONS_COMMON_MANIFEST_HANDLERS_FILE_HANDLER_INFO_H_

#include <set>
#include <string>
#include <vector>

#include "base/macros.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handler.h"

namespace extensions {

struct FileHandlerInfo {
  FileHandlerInfo();
  FileHandlerInfo(const FileHandlerInfo& other);
  ~FileHandlerInfo();

  // The id of this handler.
  std::string id;

  // File extensions associated with this handler.
  std::set<std::string> extensions;

  // MIME types associated with this handler.
  std::set<std::string> types;

  // True if the handler can manage directories.
  bool include_directories;

  // A verb describing the intent of the handler.
  std::string verb;
};

typedef std::vector<FileHandlerInfo> FileHandlersInfo;

struct FileHandlers : public Extension::ManifestData {
  FileHandlers();
  ~FileHandlers() override;

  FileHandlersInfo file_handlers;

  static const FileHandlersInfo* GetFileHandlers(const Extension* extension);
};

// Parses the "file_handlers" manifest key.
class FileHandlersParser : public ManifestHandler {
 public:
  FileHandlersParser();
  ~FileHandlersParser() override;

  bool Parse(Extension* extension, base::string16* error) override;

 private:
  base::span<const char* const> Keys() const override;

  DISALLOW_COPY_AND_ASSIGN(FileHandlersParser);
};

namespace file_handler_verbs {

// Supported verbs for file handlers.
extern const char kOpenWith[];
extern const char kAddTo[];
extern const char kPackWith[];
extern const char kShareWith[];

}  // namespace file_handler_verbs

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_MANIFEST_HANDLERS_FILE_HANDLER_INFO_H_
