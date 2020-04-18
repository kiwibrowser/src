// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/shell_integration_linux.h"

#include <stddef.h>

#include <algorithm>
#include <cstdlib>
#include <map>
#include <vector>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_path_override.h"
#include "chrome/common/chrome_constants.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using ::testing::ElementsAre;

namespace shell_integration_linux {

namespace {

// Provides mock environment variables values based on a stored map.
class MockEnvironment : public base::Environment {
 public:
  MockEnvironment() {}

  void Set(base::StringPiece name, const std::string& value) {
    variables_[name.as_string()] = value;
  }

  bool GetVar(base::StringPiece variable_name, std::string* result) override {
    if (base::ContainsKey(variables_, variable_name.as_string())) {
      *result = variables_[variable_name.as_string()];
      return true;
    }

    return false;
  }

  bool SetVar(base::StringPiece variable_name,
              const std::string& new_value) override {
    ADD_FAILURE();
    return false;
  }

  bool UnSetVar(base::StringPiece variable_name) override {
    ADD_FAILURE();
    return false;
  }

 private:
  std::map<std::string, std::string> variables_;

  DISALLOW_COPY_AND_ASSIGN(MockEnvironment);
};

// This helps EXPECT_THAT(..., ElementsAre(...)) print out more meaningful
// failure messages.
std::vector<std::string> FilePathsToStrings(
    const std::vector<base::FilePath>& paths) {
  std::vector<std::string> values;
  for (const auto& path : paths)
    values.push_back(path.value());
  return values;
}

bool WriteEmptyFile(const base::FilePath& path) {
  return base::WriteFile(path, "", 0) == 0;
}

bool WriteString(const base::FilePath& path, const base::StringPiece& str) {
  int bytes_written = base::WriteFile(path, str.data(), str.size());
  if (bytes_written < 0)
    return false;

  return static_cast<size_t>(bytes_written) == str.size();
}

}  // namespace

TEST(ShellIntegrationTest, GetDataWriteLocation) {
  content::TestBrowserThreadBundle test_browser_thread_bundle;

  // Test that it returns $XDG_DATA_HOME.
  {
    MockEnvironment env;
    base::ScopedPathOverride home_override(base::DIR_HOME,
                                           base::FilePath("/home/user"),
                                           true /* absolute? */,
                                           false /* create? */);
    env.Set("XDG_DATA_HOME", "/user/path");
    base::FilePath path = GetDataWriteLocation(&env);
    EXPECT_EQ("/user/path", path.value());
  }

  // Test that $XDG_DATA_HOME falls back to $HOME/.local/share.
  {
    MockEnvironment env;
    base::ScopedPathOverride home_override(base::DIR_HOME,
                                           base::FilePath("/home/user"),
                                           true /* absolute? */,
                                           false /* create? */);
    base::FilePath path = GetDataWriteLocation(&env);
    EXPECT_EQ("/home/user/.local/share", path.value());
  }
}

TEST(ShellIntegrationTest, GetDataSearchLocations) {
  content::TestBrowserThreadBundle test_browser_thread_bundle;

  // Test that it returns $XDG_DATA_HOME + $XDG_DATA_DIRS.
  {
    MockEnvironment env;
    base::ScopedPathOverride home_override(base::DIR_HOME,
                                           base::FilePath("/home/user"),
                                           true /* absolute? */,
                                           false /* create? */);
    env.Set("XDG_DATA_HOME", "/user/path");
    env.Set("XDG_DATA_DIRS", "/system/path/1:/system/path/2");
    EXPECT_THAT(
        FilePathsToStrings(GetDataSearchLocations(&env)),
        ElementsAre("/user/path",
                    "/system/path/1",
                    "/system/path/2"));
  }

  // Test that $XDG_DATA_HOME falls back to $HOME/.local/share.
  {
    MockEnvironment env;
    base::ScopedPathOverride home_override(base::DIR_HOME,
                                           base::FilePath("/home/user"),
                                           true /* absolute? */,
                                           false /* create? */);
    env.Set("XDG_DATA_DIRS", "/system/path/1:/system/path/2");
    EXPECT_THAT(
        FilePathsToStrings(GetDataSearchLocations(&env)),
        ElementsAre("/home/user/.local/share",
                    "/system/path/1",
                    "/system/path/2"));
  }

  // Test that if neither $XDG_DATA_HOME nor $HOME are specified, it still
  // succeeds.
  {
    MockEnvironment env;
    env.Set("XDG_DATA_DIRS", "/system/path/1:/system/path/2");
    std::vector<std::string> results =
        FilePathsToStrings(GetDataSearchLocations(&env));
    ASSERT_EQ(3U, results.size());
    EXPECT_FALSE(results[0].empty());
    EXPECT_EQ("/system/path/1", results[1]);
    EXPECT_EQ("/system/path/2", results[2]);
  }

  // Test that $XDG_DATA_DIRS falls back to the two default paths.
  {
    MockEnvironment env;
    base::ScopedPathOverride home_override(base::DIR_HOME,
                                           base::FilePath("/home/user"),
                                           true /* absolute? */,
                                           false /* create? */);
    env.Set("XDG_DATA_HOME", "/user/path");
    EXPECT_THAT(
        FilePathsToStrings(GetDataSearchLocations(&env)),
        ElementsAre("/user/path",
                    "/usr/local/share",
                    "/usr/share"));
  }
}

TEST(ShellIntegrationTest, GetExistingShortcutLocations) {
  base::FilePath kProfilePath("Profile 1");
  const char kExtensionId[] = "test_extension";
  const char kTemplateFilename[] = "chrome-test_extension-Profile_1.desktop";
  base::FilePath kTemplateFilepath(kTemplateFilename);
  const char kNoDisplayDesktopFile[] = "[Desktop Entry]\nNoDisplay=true";

  content::TestBrowserThreadBundle test_browser_thread_bundle;

  // No existing shortcuts.
  {
    MockEnvironment env;
    web_app::ShortcutLocations result =
        GetExistingShortcutLocations(&env, kProfilePath, kExtensionId);
    EXPECT_FALSE(result.on_desktop);
    EXPECT_EQ(web_app::APP_MENU_LOCATION_NONE,
              result.applications_menu_location);

    EXPECT_FALSE(result.in_quick_launch_bar);
  }

  // Shortcut on desktop.
  {
    base::ScopedTempDir temp_dir;
    ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
    base::FilePath desktop_path = temp_dir.GetPath();

    MockEnvironment env;
    ASSERT_TRUE(base::CreateDirectory(desktop_path));
    ASSERT_TRUE(WriteEmptyFile(desktop_path.Append(kTemplateFilename)));
    web_app::ShortcutLocations result = GetExistingShortcutLocations(
        &env, kProfilePath, kExtensionId, desktop_path);
    EXPECT_TRUE(result.on_desktop);
    EXPECT_EQ(web_app::APP_MENU_LOCATION_NONE,
              result.applications_menu_location);

    EXPECT_FALSE(result.in_quick_launch_bar);
  }

  // Shortcut in applications directory.
  {
    base::ScopedTempDir temp_dir;
    ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
    base::FilePath apps_path = temp_dir.GetPath().Append("applications");

    MockEnvironment env;
    env.Set("XDG_DATA_HOME", temp_dir.GetPath().value());
    ASSERT_TRUE(base::CreateDirectory(apps_path));
    ASSERT_TRUE(WriteEmptyFile(apps_path.Append(kTemplateFilename)));
    web_app::ShortcutLocations result =
        GetExistingShortcutLocations(&env, kProfilePath, kExtensionId);
    EXPECT_FALSE(result.on_desktop);
    EXPECT_EQ(web_app::APP_MENU_LOCATION_SUBDIR_CHROMEAPPS,
              result.applications_menu_location);

    EXPECT_FALSE(result.in_quick_launch_bar);
  }

  // Shortcut in applications directory with NoDisplay=true.
  {
    base::ScopedTempDir temp_dir;
    ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
    base::FilePath apps_path = temp_dir.GetPath().Append("applications");

    MockEnvironment env;
    env.Set("XDG_DATA_HOME", temp_dir.GetPath().value());
    ASSERT_TRUE(base::CreateDirectory(apps_path));
    ASSERT_TRUE(WriteString(apps_path.Append(kTemplateFilename),
                            kNoDisplayDesktopFile));
    web_app::ShortcutLocations result =
        GetExistingShortcutLocations(&env, kProfilePath, kExtensionId);
    // Doesn't count as being in applications menu.
    EXPECT_FALSE(result.on_desktop);
    EXPECT_EQ(web_app::APP_MENU_LOCATION_HIDDEN,
              result.applications_menu_location);
    EXPECT_FALSE(result.in_quick_launch_bar);
  }

  // Shortcut on desktop and in applications directory.
  {
    base::ScopedTempDir temp_dir1;
    ASSERT_TRUE(temp_dir1.CreateUniqueTempDir());
    base::FilePath desktop_path = temp_dir1.GetPath();

    base::ScopedTempDir temp_dir2;
    ASSERT_TRUE(temp_dir2.CreateUniqueTempDir());
    base::FilePath apps_path = temp_dir2.GetPath().Append("applications");

    MockEnvironment env;
    ASSERT_TRUE(base::CreateDirectory(desktop_path));
    ASSERT_TRUE(WriteEmptyFile(desktop_path.Append(kTemplateFilename)));
    env.Set("XDG_DATA_HOME", temp_dir2.GetPath().value());
    ASSERT_TRUE(base::CreateDirectory(apps_path));
    ASSERT_TRUE(WriteEmptyFile(apps_path.Append(kTemplateFilename)));
    web_app::ShortcutLocations result = GetExistingShortcutLocations(
        &env, kProfilePath, kExtensionId, desktop_path);
    EXPECT_TRUE(result.on_desktop);
    EXPECT_EQ(web_app::APP_MENU_LOCATION_SUBDIR_CHROMEAPPS,
              result.applications_menu_location);
    EXPECT_FALSE(result.in_quick_launch_bar);
  }
}

TEST(ShellIntegrationTest, GetExistingShortcutContents) {
  const char kTemplateFilename[] = "shortcut-test.desktop";
  base::FilePath kTemplateFilepath(kTemplateFilename);
  const char kTestData1[] = "a magical testing string";
  const char kTestData2[] = "a different testing string";

  content::TestBrowserThreadBundle test_browser_thread_bundle;

  // Test that it searches $XDG_DATA_HOME/applications.
  {
    base::ScopedTempDir temp_dir;
    ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

    MockEnvironment env;
    env.Set("XDG_DATA_HOME", temp_dir.GetPath().value());
    // Create a file in a non-applications directory. This should be ignored.
    ASSERT_TRUE(
        WriteString(temp_dir.GetPath().Append(kTemplateFilename), kTestData2));
    ASSERT_TRUE(
        base::CreateDirectory(temp_dir.GetPath().Append("applications")));
    ASSERT_TRUE(WriteString(
        temp_dir.GetPath().Append("applications").Append(kTemplateFilename),
        kTestData1));
    std::string contents;
    ASSERT_TRUE(
        GetExistingShortcutContents(&env, kTemplateFilepath, &contents));
    EXPECT_EQ(kTestData1, contents);
  }

  // Test that it falls back to $HOME/.local/share/applications.
  {
    base::ScopedTempDir temp_dir;
    ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

    MockEnvironment env;
    base::ScopedPathOverride home_override(base::DIR_HOME, temp_dir.GetPath(),
                                           true /* absolute? */,
                                           false /* create? */);
    ASSERT_TRUE(base::CreateDirectory(
        temp_dir.GetPath().Append(".local/share/applications")));
    ASSERT_TRUE(WriteString(temp_dir.GetPath()
                                .Append(".local/share/applications")
                                .Append(kTemplateFilename),
                            kTestData1));
    std::string contents;
    ASSERT_TRUE(
        GetExistingShortcutContents(&env, kTemplateFilepath, &contents));
    EXPECT_EQ(kTestData1, contents);
  }

  // Test that it searches $XDG_DATA_DIRS/applications.
  {
    base::ScopedTempDir temp_dir;
    ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

    MockEnvironment env;
    env.Set("XDG_DATA_DIRS", temp_dir.GetPath().value());
    ASSERT_TRUE(
        base::CreateDirectory(temp_dir.GetPath().Append("applications")));
    ASSERT_TRUE(WriteString(
        temp_dir.GetPath().Append("applications").Append(kTemplateFilename),
        kTestData2));
    std::string contents;
    ASSERT_TRUE(
        GetExistingShortcutContents(&env, kTemplateFilepath, &contents));
    EXPECT_EQ(kTestData2, contents);
  }

  // Test that it searches $X/applications for each X in $XDG_DATA_DIRS.
  {
    base::ScopedTempDir temp_dir1;
    ASSERT_TRUE(temp_dir1.CreateUniqueTempDir());
    base::ScopedTempDir temp_dir2;
    ASSERT_TRUE(temp_dir2.CreateUniqueTempDir());

    MockEnvironment env;
    env.Set("XDG_DATA_DIRS",
            temp_dir1.GetPath().value() + ":" + temp_dir2.GetPath().value());
    // Create a file in a non-applications directory. This should be ignored.
    ASSERT_TRUE(
        WriteString(temp_dir1.GetPath().Append(kTemplateFilename), kTestData1));
    // Only create a findable desktop file in the second path.
    ASSERT_TRUE(
        base::CreateDirectory(temp_dir2.GetPath().Append("applications")));
    ASSERT_TRUE(WriteString(
        temp_dir2.GetPath().Append("applications").Append(kTemplateFilename),
        kTestData2));
    std::string contents;
    ASSERT_TRUE(
        GetExistingShortcutContents(&env, kTemplateFilepath, &contents));
    EXPECT_EQ(kTestData2, contents);
  }
}

TEST(ShellIntegrationTest, GetExtensionShortcutFilename) {
  base::FilePath kProfilePath("a/b/c/Profile Name?");
  const char kExtensionId[] = "extensionid";
  EXPECT_EQ(base::FilePath("chrome-extensionid-Profile_Name_.desktop"),
            GetExtensionShortcutFilename(kProfilePath, kExtensionId));
}

TEST(ShellIntegrationTest, GetExistingProfileShortcutFilenames) {
  base::FilePath kProfilePath("a/b/c/Profile Name?");
  const char kApp1Filename[] = "chrome-extension1-Profile_Name_.desktop";
  const char kApp2Filename[] = "chrome-extension2-Profile_Name_.desktop";
  const char kUnrelatedAppFilename[] = "chrome-extension-Other_Profile.desktop";

  content::TestBrowserThreadBundle test_browser_thread_bundle;

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  ASSERT_TRUE(WriteEmptyFile(temp_dir.GetPath().Append(kApp1Filename)));
  ASSERT_TRUE(WriteEmptyFile(temp_dir.GetPath().Append(kApp2Filename)));
  // This file should not be returned in the results.
  ASSERT_TRUE(WriteEmptyFile(temp_dir.GetPath().Append(kUnrelatedAppFilename)));
  std::vector<base::FilePath> paths =
      GetExistingProfileShortcutFilenames(kProfilePath, temp_dir.GetPath());
  // Path order is arbitrary. Sort the output for consistency.
  std::sort(paths.begin(), paths.end());
  EXPECT_THAT(paths,
              ElementsAre(base::FilePath(kApp1Filename),
                          base::FilePath(kApp2Filename)));
}

TEST(ShellIntegrationTest, GetWebShortcutFilename) {
  const struct {
    const char* const path;
    const char* const url;
  } test_cases[] = {
    { "http___foo_.desktop", "http://foo" },
    { "http___foo_bar_.desktop", "http://foo/bar/" },
    { "http___foo_bar_a=b&c=d.desktop", "http://foo/bar?a=b&c=d" },

    // Now we're starting to be more evil...
    { "http___foo_.desktop", "http://foo/bar/baz/../../../../../" },
    { "http___foo_.desktop", "http://foo/bar/././../baz/././../" },
    { "http___.._.desktop", "http://../../../../" },
  };
  for (size_t i = 0; i < arraysize(test_cases); i++) {
    EXPECT_EQ(std::string(chrome::kBrowserProcessExecutableName) + "-" +
              test_cases[i].path,
              GetWebShortcutFilename(GURL(test_cases[i].url)).value()) <<
        " while testing " << test_cases[i].url;
  }
}

TEST(ShellIntegrationTest, GetDesktopFileContents) {
  const base::FilePath kChromeExePath("/opt/google/chrome/google-chrome");
  const struct {
    const char* const url;
    const char* const title;
    const char* const icon_name;
    const char* const categories;
    bool nodisplay;
    const char* const expected_output;
  } test_cases[] = {
    // Real-world case.
    { "http://gmail.com",
      "GMail",
      "chrome-http__gmail.com",
      "",
      false,

      "#!/usr/bin/env xdg-open\n"
      "[Desktop Entry]\n"
      "Version=1.0\n"
      "Terminal=false\n"
      "Type=Application\n"
      "Name=GMail\n"
      "Exec=/opt/google/chrome/google-chrome --app=http://gmail.com/\n"
      "Icon=chrome-http__gmail.com\n"
      "StartupWMClass=gmail.com\n"
    },

    // Make sure that empty icons are replaced by the chrome icon.
    { "http://gmail.com",
      "GMail",
      "",
      "",
      false,

      "#!/usr/bin/env xdg-open\n"
      "[Desktop Entry]\n"
      "Version=1.0\n"
      "Terminal=false\n"
      "Type=Application\n"
      "Name=GMail\n"
      "Exec=/opt/google/chrome/google-chrome --app=http://gmail.com/\n"
#if defined(GOOGLE_CHROME_BUILD)
      "Icon=google-chrome\n"
#else
      "Icon=chromium-browser\n"
#endif
      "StartupWMClass=gmail.com\n"
    },

    // Test adding categories and NoDisplay=true.
    { "http://gmail.com",
      "GMail",
      "chrome-http__gmail.com",
      "Graphics;Education;",
      true,

      "#!/usr/bin/env xdg-open\n"
      "[Desktop Entry]\n"
      "Version=1.0\n"
      "Terminal=false\n"
      "Type=Application\n"
      "Name=GMail\n"
      "Exec=/opt/google/chrome/google-chrome --app=http://gmail.com/\n"
      "Icon=chrome-http__gmail.com\n"
      "Categories=Graphics;Education;\n"
      "NoDisplay=true\n"
      "StartupWMClass=gmail.com\n"
    },

    // Now we're starting to be more evil...
    { "http://evil.com/evil --join-the-b0tnet",
      "Ownz0red\nExec=rm -rf /",
      "chrome-http__evil.com_evil",
      "",
      false,

      "#!/usr/bin/env xdg-open\n"
      "[Desktop Entry]\n"
      "Version=1.0\n"
      "Terminal=false\n"
      "Type=Application\n"
      "Name=http://evil.com/evil%20--join-the-b0tnet\n"
      "Exec=/opt/google/chrome/google-chrome "
      "--app=http://evil.com/evil%20--join-the-b0tnet\n"
      "Icon=chrome-http__evil.com_evil\n"
      "StartupWMClass=evil.com__evil%20--join-the-b0tnet\n"
    },
    { "http://evil.com/evil; rm -rf /; \"; rm -rf $HOME >ownz0red",
      "Innocent Title",
      "chrome-http__evil.com_evil",
      "",
      false,

      "#!/usr/bin/env xdg-open\n"
      "[Desktop Entry]\n"
      "Version=1.0\n"
      "Terminal=false\n"
      "Type=Application\n"
      "Name=Innocent Title\n"
      "Exec=/opt/google/chrome/google-chrome "
      "\"--app=http://evil.com/evil;%20rm%20-rf%20/;%20%22;%20rm%20"
      // Note: $ is escaped as \$ within an arg to Exec, and then
      // the \ is escaped as \\ as all strings in a Desktop file should
      // be; finally, \\ becomes \\\\ when represented in a C++ string!
      "-rf%20\\\\$HOME%20%3Eownz0red\"\n"
      "Icon=chrome-http__evil.com_evil\n"
      "StartupWMClass=evil.com__evil;%20rm%20-rf%20_;%20%22;%20"
      "rm%20-rf%20$HOME%20%3Eownz0red\n"
    },
    { "http://evil.com/evil | cat `echo ownz0red` >/dev/null",
      "Innocent Title",
      "chrome-http__evil.com_evil",
      "",
      false,

      "#!/usr/bin/env xdg-open\n"
      "[Desktop Entry]\n"
      "Version=1.0\n"
      "Terminal=false\n"
      "Type=Application\n"
      "Name=Innocent Title\n"
      "Exec=/opt/google/chrome/google-chrome "
      "--app=http://evil.com/evil%20%7C%20cat%20%60echo%20ownz0red"
      "%60%20%3E/dev/null\n"
      "Icon=chrome-http__evil.com_evil\n"
      "StartupWMClass=evil.com__evil%20%7C%20cat%20%60echo%20ownz0red"
      "%60%20%3E_dev_null\n"
    },
  };

  for (size_t i = 0; i < arraysize(test_cases); i++) {
    SCOPED_TRACE(i);
    EXPECT_EQ(
        test_cases[i].expected_output,
        GetDesktopFileContents(
            kChromeExePath,
            web_app::GenerateApplicationNameFromURL(GURL(test_cases[i].url)),
            GURL(test_cases[i].url),
            std::string(),
            base::ASCIIToUTF16(test_cases[i].title),
            test_cases[i].icon_name,
            base::FilePath(),
            test_cases[i].categories,
            test_cases[i].nodisplay));
  }
}

TEST(ShellIntegrationTest, GetDesktopFileContentsAppList) {
  const base::FilePath kChromeExePath("/opt/google/chrome/google-chrome");
  base::CommandLine command_line(kChromeExePath);
  command_line.AppendSwitch("--show-app-list");
  EXPECT_EQ(
      "#!/usr/bin/env xdg-open\n"
      "[Desktop Entry]\n"
      "Version=1.0\n"
      "Terminal=false\n"
      "Type=Application\n"
      "Name=Chrome App Launcher\n"
      "Exec=/opt/google/chrome/google-chrome --show-app-list\n"
      "Icon=chrome_app_list\n"
      "Categories=Network;WebBrowser;\n"
      "StartupWMClass=chrome-app-list\n",
      GetDesktopFileContentsForCommand(
          command_line,
          "chrome-app-list",
          GURL(),
          base::ASCIIToUTF16("Chrome App Launcher"),
          "chrome_app_list",
          "Network;WebBrowser;",
          false));
}

TEST(ShellIntegrationTest, GetDirectoryFileContents) {
  const struct {
    const char* const title;
    const char* const icon_name;
    const char* const expected_output;
  } test_cases[] = {
    // Real-world case.
    { "Chrome Apps",
      "chrome-apps",

      "[Desktop Entry]\n"
      "Version=1.0\n"
      "Type=Directory\n"
      "Name=Chrome Apps\n"
      "Icon=chrome-apps\n"
    },

    // Make sure that empty icons are replaced by the chrome icon.
    { "Chrome Apps",
      "",

      "[Desktop Entry]\n"
      "Version=1.0\n"
      "Type=Directory\n"
      "Name=Chrome Apps\n"
#if defined(GOOGLE_CHROME_BUILD)
      "Icon=google-chrome\n"
#else
      "Icon=chromium-browser\n"
#endif
    },
  };

  for (size_t i = 0; i < arraysize(test_cases); i++) {
    SCOPED_TRACE(i);
    EXPECT_EQ(test_cases[i].expected_output,
              GetDirectoryFileContents(base::ASCIIToUTF16(test_cases[i].title),
                                       test_cases[i].icon_name));
  }
}

TEST(ShellIntegrationTest, WmClass) {
  base::CommandLine command_line((base::FilePath()));
  EXPECT_EQ("foo", internal::GetProgramClassName(command_line, "foo.desktop"));
  EXPECT_EQ("Foo", internal::GetProgramClassClass(command_line, "foo.desktop"));

  command_line.AppendSwitchASCII("class", "baR");
  EXPECT_EQ("foo", internal::GetProgramClassName(command_line, "foo.desktop"));
  EXPECT_EQ("baR", internal::GetProgramClassClass(command_line, "foo.desktop"));

  command_line = base::CommandLine(base::FilePath());
  command_line.AppendSwitchASCII("user-data-dir", "/tmp/baz");
  EXPECT_EQ("foo (/tmp/baz)",
            internal::GetProgramClassName(command_line, "foo.desktop"));
  EXPECT_EQ("Foo", internal::GetProgramClassClass(command_line, "foo.desktop"));
}

}  // namespace shell_integration_linux
