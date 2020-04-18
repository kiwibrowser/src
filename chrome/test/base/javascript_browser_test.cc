// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/javascript_browser_test.h"

#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/common/chrome_paths.h"
#include "components/nacl/common/buildflags.h"
#include "content/public/browser/web_ui.h"
#include "net/base/filename_util.h"

// static
const base::FilePath::CharType
    JavaScriptBrowserTest::kA11yAuditLibraryJSPath[] =
        FILE_PATH_LITERAL("third_party/accessibility-audit/axs_testing.js");

// static
const base::FilePath::CharType JavaScriptBrowserTest::kChaiJSPath[] =
    FILE_PATH_LITERAL("third_party/chaijs/chai.js");

// static
const base::FilePath::CharType JavaScriptBrowserTest::kMockJSPath[] =
    FILE_PATH_LITERAL("chrome/third_party/mock4js/mock4js.js");

// static
const base::FilePath::CharType JavaScriptBrowserTest::kWebUILibraryJS[] =
    FILE_PATH_LITERAL("test_api.js");

// static
const base::FilePath::CharType JavaScriptBrowserTest::kWebUITestFolder[] =
    FILE_PATH_LITERAL("webui");

void JavaScriptBrowserTest::AddLibrary(const base::FilePath& library_path) {
  user_libraries_.push_back(library_path);
}

JavaScriptBrowserTest::JavaScriptBrowserTest() {
}

JavaScriptBrowserTest::~JavaScriptBrowserTest() {
}

void JavaScriptBrowserTest::SetUpOnMainThread() {
  base::FilePath test_data_directory;
  ASSERT_TRUE(
      base::PathService::Get(chrome::DIR_TEST_DATA, &test_data_directory));
  test_data_directory = test_data_directory.Append(kWebUITestFolder);
  library_search_paths_.push_back(test_data_directory);

// When the sanitizers (ASAN/MSAN/TSAN) are enabled, the WebUI tests
// which use this generated directory are disabled in the build.
// However, the generated directory is there if NaCl is enabled --
// though it's usually disabled on the bots when the sanitizers are
// enabled. Also, it seems some ChromeOS-specific tests use the
// js2gtest GN template.
#if (!defined(MEMORY_SANITIZER) && !defined(ADDRESS_SANITIZER) && \
     !defined(LEAK_SANITIZER)) ||                                 \
    BUILDFLAG(ENABLE_NACL) || defined(OS_CHROMEOS)
  base::FilePath gen_test_data_directory;
  ASSERT_TRUE(base::PathService::Get(chrome::DIR_GEN_TEST_DATA,
                                     &gen_test_data_directory));
  library_search_paths_.push_back(gen_test_data_directory);
#endif

  base::FilePath source_root_directory;
  ASSERT_TRUE(
      base::PathService::Get(base::DIR_SOURCE_ROOT, &source_root_directory));
  library_search_paths_.push_back(source_root_directory);

  AddLibrary(base::FilePath(kMockJSPath));
  AddLibrary(base::FilePath(kChaiJSPath));
  AddLibrary(base::FilePath(kWebUILibraryJS));
}

// TODO(dtseng): Make this return bool (success/failure) and remove ASSERt_TRUE
// calls.
void JavaScriptBrowserTest::BuildJavascriptLibraries(
    std::vector<base::string16>* libraries) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  ASSERT_TRUE(libraries != NULL);
  std::vector<base::FilePath>::iterator user_libraries_iterator;
  for (user_libraries_iterator = user_libraries_.begin();
       user_libraries_iterator != user_libraries_.end();
       ++user_libraries_iterator) {
    std::string library_content;
    base::FilePath library_absolute_path;
    if (user_libraries_iterator->IsAbsolute()) {
      library_absolute_path = *user_libraries_iterator;
      ASSERT_TRUE(
          base::ReadFileToString(library_absolute_path, &library_content))
          << user_libraries_iterator->value();
    } else {
      bool ok = false;
      std::vector<base::FilePath>::iterator library_search_path_iterator;
      for (library_search_path_iterator = library_search_paths_.begin();
           library_search_path_iterator != library_search_paths_.end();
           ++library_search_path_iterator) {
        library_absolute_path = base::MakeAbsoluteFilePath(
            library_search_path_iterator->Append(*user_libraries_iterator));
        ok = base::ReadFileToString(library_absolute_path, &library_content);
        if (ok)
          break;
      }
      ASSERT_TRUE(ok) << "User library not found: "
                      << user_libraries_iterator->value();
    }
    library_content.append(";\n");

    // This magic code puts filenames in stack traces.
    library_content.append("//# sourceURL=");
    library_content.append(
        net::FilePathToFileURL(library_absolute_path).spec());
    library_content.append("\n");
    libraries->push_back(base::UTF8ToUTF16(library_content));
  }
}

base::string16 JavaScriptBrowserTest::BuildRunTestJSCall(
    bool is_async,
    const std::string& function_name,
    std::vector<base::Value> test_func_args) {
  std::vector<std::unique_ptr<base::Value>> arguments;
  arguments.push_back(std::make_unique<base::Value>(is_async));
  arguments.push_back(std::make_unique<base::Value>(function_name));
  auto baked_argument_list = std::make_unique<base::ListValue>();
  for (const auto& arg : test_func_args)
    baked_argument_list->Append(arg.CreateDeepCopy());
  arguments.push_back(std::move(baked_argument_list));

  std::vector<const base::Value*> ptr_vector;
  ptr_vector.reserve(arguments.size());
  for (const auto& argument : arguments)
    ptr_vector.push_back(argument.get());
  return content::WebUI::GetJavascriptCall(std::string("runTest"), ptr_vector);
}
