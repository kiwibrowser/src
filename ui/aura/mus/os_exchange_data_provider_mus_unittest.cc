// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/os_exchange_data_provider_mus.h"

#include <memory>

#include "base/files/file_util.h"
#include "base/pickle.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "build/build_config.h"
#include "net/base/filename_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/base/dragdrop/os_exchange_data_provider_factory.h"
#include "ui/events/platform/platform_event_source.h"
#include "url/gurl.h"

using ui::Clipboard;
using ui::OSExchangeData;

namespace aura {

// This file is a copy/paste of the unit tests in os_exchange_data_unittest.cc,
// with additional SetUp() to force the mus override. I thought about changeing
// the OSExchangeData test suite to a parameterized test suite, but I've
// previously had problems with that feature of gtest.

class OSExchangeDataProviderMusTest
    : public PlatformTest,
      public ui::OSExchangeDataProviderFactory::Factory {
 public:
  OSExchangeDataProviderMusTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

  // Overridden from PlatformTest:
  void SetUp() override {
    PlatformTest::SetUp();
    old_factory_ = ui::OSExchangeDataProviderFactory::TakeFactory();
    ui::OSExchangeDataProviderFactory::SetFactory(this);
  }

  void TearDown() override {
    ui::OSExchangeDataProviderFactory::TakeFactory();
    ui::OSExchangeDataProviderFactory::SetFactory(old_factory_);
    PlatformTest::TearDown();
  }

  // Overridden from ui::OSExchangeDataProviderFactory::Factory:
  std::unique_ptr<OSExchangeData::Provider> BuildProvider() override {
    return std::make_unique<OSExchangeDataProviderMus>();
  }

 private:
  ui::OSExchangeDataProviderFactory::Factory* old_factory_ = nullptr;
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(OSExchangeDataProviderMusTest, StringDataGetAndSet) {
  OSExchangeData data;
  base::string16 input = base::ASCIIToUTF16("I can has cheezburger?");
  EXPECT_FALSE(data.HasString());
  data.SetString(input);
  EXPECT_TRUE(data.HasString());

  OSExchangeData data2(
      std::unique_ptr<OSExchangeData::Provider>(data.provider().Clone()));
  base::string16 output;
  EXPECT_TRUE(data2.HasString());
  EXPECT_TRUE(data2.GetString(&output));
  EXPECT_EQ(input, output);
  std::string url_spec = "http://www.goats.com/";
  GURL url(url_spec);
  base::string16 title;
  EXPECT_FALSE(data2.GetURLAndTitle(OSExchangeData::DO_NOT_CONVERT_FILENAMES,
                                    &url, &title));
  // No URLs in |data|, so url should be untouched.
  EXPECT_EQ(url_spec, url.spec());
}

TEST_F(OSExchangeDataProviderMusTest, TestURLExchangeFormats) {
  OSExchangeData data;
  std::string url_spec = "http://www.google.com/";
  GURL url(url_spec);
  base::string16 url_title = base::ASCIIToUTF16("www.google.com");
  EXPECT_FALSE(data.HasURL(OSExchangeData::DO_NOT_CONVERT_FILENAMES));
  data.SetURL(url, url_title);
  EXPECT_TRUE(data.HasURL(OSExchangeData::DO_NOT_CONVERT_FILENAMES));

  OSExchangeData data2(
      std::unique_ptr<OSExchangeData::Provider>(data.provider().Clone()));

  // URL spec and title should match
  GURL output_url;
  base::string16 output_title;
  EXPECT_TRUE(data2.HasURL(OSExchangeData::DO_NOT_CONVERT_FILENAMES));
  EXPECT_TRUE(data2.GetURLAndTitle(OSExchangeData::DO_NOT_CONVERT_FILENAMES,
                                   &output_url, &output_title));
  EXPECT_EQ(url_spec, output_url.spec());
  EXPECT_EQ(url_title, output_title);
  base::string16 output_string;

  // URL should be the raw text response
  EXPECT_TRUE(data2.GetString(&output_string));
  EXPECT_EQ(url_spec, base::UTF16ToUTF8(output_string));
}

// Test that setting the URL does not overwrite a previously set custom string.
TEST_F(OSExchangeDataProviderMusTest, URLAndString) {
  OSExchangeData data;
  base::string16 string = base::ASCIIToUTF16("I can has cheezburger?");
  data.SetString(string);
  std::string url_spec = "http://www.google.com/";
  GURL url(url_spec);
  base::string16 url_title = base::ASCIIToUTF16("www.google.com");
  data.SetURL(url, url_title);

  base::string16 output_string;
  EXPECT_TRUE(data.GetString(&output_string));
  EXPECT_EQ(string, output_string);

  GURL output_url;
  base::string16 output_title;
  EXPECT_TRUE(data.GetURLAndTitle(OSExchangeData::DO_NOT_CONVERT_FILENAMES,
                                  &output_url, &output_title));
  EXPECT_EQ(url_spec, output_url.spec());
  EXPECT_EQ(url_title, output_title);
}

TEST_F(OSExchangeDataProviderMusTest, TestFileToURLConversion) {
  OSExchangeData data;
  EXPECT_FALSE(data.HasURL(OSExchangeData::DO_NOT_CONVERT_FILENAMES));
  EXPECT_FALSE(data.HasURL(OSExchangeData::CONVERT_FILENAMES));
  EXPECT_FALSE(data.HasFile());

  base::FilePath current_directory;
  ASSERT_TRUE(base::GetCurrentDirectory(&current_directory));

  data.SetFilename(current_directory);

  {
    EXPECT_FALSE(data.HasURL(OSExchangeData::DO_NOT_CONVERT_FILENAMES));
    GURL actual_url;
    base::string16 actual_title;
    EXPECT_FALSE(data.GetURLAndTitle(OSExchangeData::DO_NOT_CONVERT_FILENAMES,
                                     &actual_url, &actual_title));
    EXPECT_EQ(GURL(), actual_url);
    EXPECT_EQ(base::string16(), actual_title);
  }

  {
    EXPECT_TRUE(data.HasURL(OSExchangeData::CONVERT_FILENAMES));
    GURL actual_url;
    base::string16 actual_title;
    EXPECT_TRUE(data.GetURLAndTitle(OSExchangeData::CONVERT_FILENAMES,
                                    &actual_url, &actual_title));
    // Some Mac OS versions return the URL in file://localhost form instead
    // of file:///, so we compare the url's path not its absolute string.
    EXPECT_EQ(net::FilePathToFileURL(current_directory).path(),
              actual_url.path());
    EXPECT_EQ(base::string16(), actual_title);
  }
  EXPECT_TRUE(data.HasFile());
  base::FilePath actual_path;
  EXPECT_TRUE(data.GetFilename(&actual_path));
  EXPECT_EQ(current_directory, actual_path);
}

TEST_F(OSExchangeDataProviderMusTest, TestPickledData) {
  const Clipboard::FormatType kTestFormat =
      Clipboard::GetFormatType("application/vnd.chromium.test");

  base::Pickle saved_pickle;
  saved_pickle.WriteInt(1);
  saved_pickle.WriteInt(2);
  OSExchangeData data;
  data.SetPickledData(kTestFormat, saved_pickle);

  OSExchangeData copy(
      std::unique_ptr<OSExchangeData::Provider>(data.provider().Clone()));
  EXPECT_TRUE(copy.HasCustomFormat(kTestFormat));

  base::Pickle restored_pickle;
  EXPECT_TRUE(copy.GetPickledData(kTestFormat, &restored_pickle));
  base::PickleIterator iterator(restored_pickle);
  int value;
  EXPECT_TRUE(iterator.ReadInt(&value));
  EXPECT_EQ(1, value);
  EXPECT_TRUE(iterator.ReadInt(&value));
  EXPECT_EQ(2, value);
}

TEST_F(OSExchangeDataProviderMusTest, TestHTML) {
  OSExchangeData data;
  GURL url("http://www.google.com/");
  base::string16 html = base::ASCIIToUTF16(
      "<HTML>\n<BODY>\n"
      "<b>bold.</b> <i><b>This is bold italic.</b></i>\n"
      "</BODY>\n</HTML>");
  data.SetHtml(html, url);

  OSExchangeData copy(
      std::unique_ptr<OSExchangeData::Provider>(data.provider().Clone()));
  base::string16 read_html;
  EXPECT_TRUE(copy.GetHtml(&read_html, &url));
  EXPECT_EQ(html, read_html);
}

}  // namespace aura
