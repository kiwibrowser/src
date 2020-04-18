// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/plugin_list.h"

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

namespace {

base::FilePath::CharType kFooPath[] = FILE_PATH_LITERAL("/plugins/foo.plugin");
base::FilePath::CharType kBarPath[] = FILE_PATH_LITERAL("/plugins/bar.plugin");
const char* kFooName = "Foo Plugin";
const char* kFooMimeType = "application/x-foo-mime-type";
const char* kFooFileType = "foo";

bool Equals(const WebPluginInfo& a, const WebPluginInfo& b) {
  return (a.name == b.name &&
          a.path == b.path &&
          a.version == b.version &&
          a.desc == b.desc);
}

bool Contains(const std::vector<WebPluginInfo>& list,
              const WebPluginInfo& plugin) {
  for (std::vector<WebPluginInfo>::const_iterator it = list.begin();
       it != list.end(); ++it) {
    if (Equals(*it, plugin))
      return true;
  }
  return false;
}

}  // namespace

class PluginListTest : public testing::Test {
 public:
  PluginListTest()
      : foo_plugin_(base::ASCIIToUTF16(kFooName),
                    base::FilePath(kFooPath),
                    base::ASCIIToUTF16("1.2.3"),
                    base::ASCIIToUTF16("foo")),
        bar_plugin_(base::ASCIIToUTF16("Bar Plugin"),
                    base::FilePath(kBarPath),
                    base::ASCIIToUTF16("2.3.4"),
                    base::ASCIIToUTF16("bar")) {
  }

  void SetUp() override {
    plugin_list_.RegisterInternalPlugin(bar_plugin_, false);
    foo_plugin_.mime_types.push_back(
        WebPluginMimeType(kFooMimeType, kFooFileType, std::string()));
    plugin_list_.RegisterInternalPlugin(foo_plugin_, false);
  }

 protected:
  PluginList plugin_list_;
  WebPluginInfo foo_plugin_;
  WebPluginInfo bar_plugin_;
};

TEST_F(PluginListTest, GetPlugins) {
  std::vector<WebPluginInfo> plugins;
  plugin_list_.GetPlugins(&plugins);
  EXPECT_EQ(2u, plugins.size());
  EXPECT_TRUE(Contains(plugins, foo_plugin_));
  EXPECT_TRUE(Contains(plugins, bar_plugin_));
}

TEST_F(PluginListTest, BadPluginDescription) {
  WebPluginInfo plugin_3043(
      base::string16(), base::FilePath(FILE_PATH_LITERAL("/myplugin.3.0.43")),
      base::string16(), base::string16());
  // Simulate loading of the plugins.
  plugin_list_.RegisterInternalPlugin(plugin_3043, false);
  // Now we should have them in the state we specified above.
  plugin_list_.RefreshPlugins();
  std::vector<WebPluginInfo> plugins;
  plugin_list_.GetPlugins(&plugins);
  ASSERT_TRUE(Contains(plugins, plugin_3043));
}

TEST_F(PluginListTest, GetPluginInfoArray) {
  const char kTargetUrl[] = "http://example.com/test.foo";
  GURL target_url(kTargetUrl);
  std::vector<WebPluginInfo> plugins;
  std::vector<std::string> actual_mime_types;

  // The file type of the URL is supported by foo_plugin_. However,
  // GetPluginInfoArray should not match foo_plugin_ because the MIME type is
  // application/octet-stream.
  plugin_list_.GetPluginInfoArray(target_url,
                                  "application/octet-stream",
                                  false, // allow_wildcard
                                  NULL,  // use_stale
                                  &plugins,
                                  &actual_mime_types);
  EXPECT_EQ(0u, plugins.size());
  EXPECT_EQ(0u, actual_mime_types.size());

  // foo_plugin_ matches due to the MIME type.
  plugins.clear();
  actual_mime_types.clear();
  plugin_list_.GetPluginInfoArray(target_url,
                                  kFooMimeType,
                                  false, // allow_wildcard
                                  NULL,  // use_stale
                                  &plugins,
                                  &actual_mime_types);
  EXPECT_EQ(1u, plugins.size());
  EXPECT_TRUE(Contains(plugins, foo_plugin_));
  ASSERT_EQ(1u, actual_mime_types.size());
  EXPECT_EQ(kFooMimeType, actual_mime_types.front());

  // foo_plugin_ matches due to the file type and empty MIME type.
  plugins.clear();
  actual_mime_types.clear();
  plugin_list_.GetPluginInfoArray(target_url,
                                  "",
                                  false, // allow_wildcard
                                  NULL,  // use_stale
                                  &plugins,
                                  &actual_mime_types);
  EXPECT_EQ(1u, plugins.size());
  EXPECT_TRUE(Contains(plugins, foo_plugin_));
  ASSERT_EQ(1u, actual_mime_types.size());
  EXPECT_EQ(kFooMimeType, actual_mime_types.front());
}

}  // namespace content
