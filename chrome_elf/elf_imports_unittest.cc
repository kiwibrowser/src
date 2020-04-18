// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <windows.h>

#include <algorithm>
#include <vector>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/files/memory_mapped_file.h"
#include "base/path_service.h"
#include "base/strings/pattern.h"
#include "base/strings/string_util.h"
#include "base/test/launcher/test_launcher.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "base/test/test_suite.h"
#include "base/win/pe_image.h"
#include "build/build_config.h"
#include "chrome/install_static/test/scoped_install_details.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class ELFImportsTest : public testing::Test {
 protected:
  static bool ImportsCallback(const base::win::PEImage &image,
                              LPCSTR module,
                              PIMAGE_THUNK_DATA name_table,
                              PIMAGE_THUNK_DATA iat,
                              PVOID cookie) {
    std::vector<std::string>* import_list =
        reinterpret_cast<std::vector<std::string>*>(cookie);
    import_list->push_back(module);
    return true;
  }

  void GetImports(const base::FilePath& module_path,
                  std::vector<std::string>* imports) {
    ASSERT_TRUE(imports != NULL);

    base::MemoryMappedFile module_mmap;

    ASSERT_TRUE(module_mmap.Initialize(module_path));
    base::win::PEImageAsData pe_image_data(
        reinterpret_cast<HMODULE>(const_cast<uint8_t*>(module_mmap.data())));
    pe_image_data.EnumImportChunks(ELFImportsTest::ImportsCallback, imports);
  }
};

// Run this test only in Release builds.
//
// This test makes sure that chrome_elf.dll has only certain types of imports.
// However, it directly and indirectly depends on base, which has lots more
// imports than are allowed here.
//
// In release builds, the offending imports are all stripped since this
// depends on a relatively small portion of base. In GYP, this works in debug
// builds as well because static libraries are used for the sandbox and base
// targets and the files that use e.g. user32.dll happen to not get brought
// into the build in the first place (due to the way static libraries are
// linked where only the required .o files are included). But we don't bother
// differentiating GYP and GN builds for this purpose.
//
// If you break this test, you may have changed base or the Windows sandbox
// such that more system imports are required to link.
#if defined(NDEBUG) && !defined(COMPONENT_BUILD)

TEST_F(ELFImportsTest, ChromeElfSanityCheck) {
  base::FilePath dll;
  ASSERT_TRUE(base::PathService::Get(base::DIR_EXE, &dll));
  dll = dll.Append(L"chrome_elf.dll");

  std::vector<std::string> elf_imports;
  GetImports(dll, &elf_imports);

  // Check that ELF has imports.
  ASSERT_LT(0u, elf_imports.size())
      << "Ensure the chrome_elf_import_unittests "
         "target was built, instead of chrome_elf_import_unittests.exe";

  static const char* const kValidFilePatterns[] = {
    "KERNEL32.dll",
    "RPCRT4.dll",
#if defined(ADDRESS_SANITIZER) && defined(COMPONENT_BUILD)
    "clang_rt.asan_dynamic-i386.dll",
#endif
    "ADVAPI32.dll",
    // On 64 bit the Version API's like VerQueryValue come from VERSION.dll.
    // It depends on kernel32, advapi32 and api-ms-win-crt*.dll. This should
    // be ok.
    "VERSION.dll",
  };

  // Make sure all of ELF's imports are in the valid imports list.
  for (const std::string& import : elf_imports) {
    bool match = false;
    for (const char* kValidFilePattern : kValidFilePatterns) {
      if (base::MatchPattern(import, kValidFilePattern)) {
        match = true;
        break;
      }
    }
    ASSERT_TRUE(match) << "Illegal import in chrome_elf.dll: " << import;
  }
}

TEST_F(ELFImportsTest, ChromeElfLoadSanityTest) {
  // chrome_elf will try to launch crashpad_handler by reinvoking the current
  // binary with --type=crashpad-handler if not already running that way. To
  // avoid that, we relaunch and run the real test body manually, adding that
  // command line argument, as we're only trying to confirm that user32.dll
  // doesn't get loaded by import table when chrome_elf.dll does.
  base::CommandLine new_test =
      base::CommandLine(base::CommandLine::ForCurrentProcess()->GetProgram());
  new_test.AppendSwitchASCII(
      base::kGTestFilterFlag,
      "ELFImportsTest.DISABLED_ChromeElfLoadSanityTestImpl");
  new_test.AppendSwitchASCII("type", "crashpad-handler");
  new_test.AppendSwitch("gtest_also_run_disabled_tests");
  new_test.AppendSwitch("single-process-tests");

  std::string output;
  ASSERT_TRUE(base::GetAppOutput(new_test, &output));
  std::string crash_string =
      "OK ] ELFImportsTest.DISABLED_ChromeElfLoadSanityTestImpl";

  if (output.find(crash_string) == std::string::npos) {
    GTEST_FAIL() << "Couldn't find\n" << crash_string << "\n in output\n "
                 << output;
  }
}

// Note: This test is not actually disabled, it's just tagged disabled so that
// the real run (above, in ChromeElfLoadSanityTest) can run it with an argument
// added to the command line.
TEST_F(ELFImportsTest, DISABLED_ChromeElfLoadSanityTestImpl) {
  base::FilePath dll;
  ASSERT_TRUE(base::PathService::Get(base::DIR_EXE, &dll));
  dll = dll.Append(L"chrome_elf.dll");

  // We don't expect user32 to be loaded in chrome_elf_import_unittests. If this
  // test case fails, then it means that a dependency on user32 has crept into
  // the chrome_elf_imports_unittests executable, which needs to be removed.
  // NOTE: it may be a secondary dependency of another system DLL.  If so,
  // try adding a "/DELAYLOAD:<blah>.dll" to the build.gn file.
  ASSERT_EQ(nullptr, ::GetModuleHandle(L"user32.dll"));

  HMODULE chrome_elf_module_handle = ::LoadLibrary(dll.value().c_str());
  EXPECT_TRUE(chrome_elf_module_handle != nullptr);
  // Loading chrome_elf.dll should not load user32.dll
  EXPECT_EQ(nullptr, ::GetModuleHandle(L"user32.dll"));
  EXPECT_TRUE(!!::FreeLibrary(chrome_elf_module_handle));
}

#endif  // NDEBUG && !COMPONENT_BUILD

TEST_F(ELFImportsTest, ChromeExeSanityCheck) {
  std::vector<std::string> exe_imports;

  base::FilePath exe;
  ASSERT_TRUE(base::PathService::Get(base::DIR_EXE, &exe));
  exe = exe.Append(L"chrome.exe");
  GetImports(exe, &exe_imports);

  // Check that chrome.exe has imports.
  ASSERT_LT(0u, exe_imports.size())
      << "Ensure the chrome_elf_import_unittests "
         "target was built, instead of chrome_elf_import_unittests.exe";

  // Chrome.exe's first import must be ELF.
  EXPECT_EQ("chrome_elf.dll", exe_imports[0])
      << "Illegal import order in chrome.exe (ensure the "
         "chrome_elf_import_unittest "
         "target was built, instead of just chrome_elf_import_unittests.exe)";
}

}  // namespace

int main(int argc, char** argv) {
  // Ensure that the CommandLine instance honors the command line passed in
  // instead of the default behavior on Windows which is to use the shell32
  // CommandLineToArgvW API. The chrome_elf_imports_unittests test suite should
  // not depend on user32 directly or indirectly (For the curious shell32
  // depends on user32)
  base::CommandLine::InitUsingArgvForTesting(argc, argv);

  install_static::ScopedInstallDetails scoped_install_details;

  base::TestSuite test_suite(argc, argv);
  return base::LaunchUnitTests(
      argc, argv,
      base::Bind(&base::TestSuite::Run, base::Unretained(&test_suite)));
}
