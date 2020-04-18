// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/extension_install_prompt_test_utils.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_file_value_serializer.h"
#include "base/path_service.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/common/chrome_paths.h"
#include "extensions/common/extension.h"

using extensions::Extension;

namespace chrome {

scoped_refptr<extensions::Extension> LoadInstallPromptExtension(
    const char* extension_dir_name,
    const char* manifest_file) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  scoped_refptr<Extension> extension;

  base::FilePath path;
  base::PathService::Get(chrome::DIR_TEST_DATA, &path);
  path = path.AppendASCII("extensions")
             .AppendASCII(extension_dir_name)
             .AppendASCII(manifest_file);

  std::string error;
  JSONFileValueDeserializer deserializer(path);
  std::unique_ptr<base::DictionaryValue> value =
      base::DictionaryValue::From(deserializer.Deserialize(NULL, &error));
  if (!value.get()) {
    LOG(ERROR) << error;
    return extension;
  }

  extension = Extension::Create(
      path.DirName(), extensions::Manifest::INVALID_LOCATION, *value,
      Extension::NO_FLAGS, &error);
  if (!extension.get())
    LOG(ERROR) << error;

  return extension;
}

scoped_refptr<Extension> LoadInstallPromptExtension() {
  return LoadInstallPromptExtension("install_prompt", "extension.json");
}

gfx::Image LoadInstallPromptIcon() {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath path;
  base::PathService::Get(chrome::DIR_TEST_DATA, &path);
  path = path.AppendASCII("extensions")
             .AppendASCII("install_prompt")
             .AppendASCII("icon.png");

  std::string file_contents;
  base::ReadFileToString(path, &file_contents);

  return gfx::Image::CreateFrom1xPNGBytes(
      reinterpret_cast<const unsigned char*>(file_contents.c_str()),
      file_contents.length());
}

std::unique_ptr<ExtensionInstallPrompt::Prompt> BuildExtensionInstallPrompt(
    Extension* extension) {
  std::unique_ptr<ExtensionInstallPrompt::Prompt> prompt(
      new ExtensionInstallPrompt::Prompt(
          ExtensionInstallPrompt::INSTALL_PROMPT));
  prompt->set_extension(extension);
  prompt->set_icon(LoadInstallPromptIcon());
  return prompt;
}

std::unique_ptr<ExtensionInstallPrompt::Prompt>
BuildExtensionPostInstallPermissionsPrompt(Extension* extension) {
  std::unique_ptr<ExtensionInstallPrompt::Prompt> prompt(
      new ExtensionInstallPrompt::Prompt(
          ExtensionInstallPrompt::POST_INSTALL_PERMISSIONS_PROMPT));
  prompt->set_extension(extension);
  prompt->set_icon(LoadInstallPromptIcon());
  return prompt;
}

}  // namespace chrome
