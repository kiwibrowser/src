// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_task_environment.h"
#include "build/build_config.h"
#include "components/update_client/protocol_builder.h"
#include "components/update_client/test_configurator.h"
#include "components/update_client/updater_state.h"
#include "testing/gtest/include/gtest/gtest.h"

using std::string;

namespace update_client {

TEST(BuildProtocolRequest, SessionIdProdIdVersion) {
  // Verifies that |session_id|, |prod_id| and |version| are serialized.
  const string request = BuildProtocolRequest(
      "15160585-8ADE-4D3C-839B-1281A6035D1F", "some_prod_id", "1.0", "", "", "",
      "", "", "", nullptr);
  EXPECT_NE(
      string::npos,
      request.find(" sessionid=\"{15160585-8ADE-4D3C-839B-1281A6035D1F}\" "));
  EXPECT_NE(string::npos,
            request.find(" updater=\"some_prod_id\" updaterversion=\"1.0\" "
                         "prodversion=\"1.0\" "));
}

TEST(BuildProtocolRequest, DownloadPreference) {
  // Verifies that an empty |download_preference| is not serialized.
  const string request_no_dlpref =
      BuildProtocolRequest("1", "", "", "", "", "", "", "", "", nullptr);
  EXPECT_EQ(string::npos, request_no_dlpref.find(" dlpref="));

  // Verifies that |download_preference| is serialized.
  const string request_with_dlpref = BuildProtocolRequest(
      "1", "", "", "", "", "", "some pref", "", "", nullptr);
  EXPECT_NE(string::npos, request_with_dlpref.find(" dlpref=\"some pref\""));
}

TEST(BuildProtocolRequest, UpdaterStateAttributes) {
  // When no updater state is provided, then check that the elements and
  // attributes related to the updater state are not serialized.
  std::string request =
      BuildProtocolRequest("1", "", "", "", "", "", "", "", "", nullptr)
          .c_str();
  EXPECT_EQ(std::string::npos, request.find(" domainjoined"));
  EXPECT_EQ(std::string::npos, request.find("<updater"));

  UpdaterState::Attributes attributes;
  attributes["ismachine"] = "1";
  attributes["domainjoined"] = "1";
  attributes["name"] = "Omaha";
  attributes["version"] = "1.2.3.4";
  attributes["laststarted"] = "1";
  attributes["lastchecked"] = "2";
  attributes["autoupdatecheckenabled"] = "0";
  attributes["updatepolicy"] = "-1";
  request = BuildProtocolRequest(
      "1", "", "", "", "", "", "", "", "",
      std::make_unique<UpdaterState::Attributes>(attributes));
  EXPECT_NE(std::string::npos, request.find(" domainjoined=\"1\""));
  const std::string updater_element =
      "<updater autoupdatecheckenabled=\"0\" ismachine=\"1\" "
      "lastchecked=\"2\" laststarted=\"1\" name=\"Omaha\" "
      "updatepolicy=\"-1\" version=\"1.2.3.4\"/>";
#if defined(GOOGLE_CHROME_BUILD)
  EXPECT_NE(std::string::npos, request.find(updater_element));
#else
  EXPECT_EQ(std::string::npos, request.find(updater_element));
#endif  // GOOGLE_CHROME_BUILD
}

TEST(BuildProtocolRequest, BuildUpdateCheckExtraRequestHeaders) {
  base::test::ScopedTaskEnvironment scoped_task_environment;
  auto config = base::MakeRefCounted<TestConfigurator>();

  auto headers = BuildUpdateCheckExtraRequestHeaders(config, {}, true);
  EXPECT_STREQ("fake_prodid-30.0", headers["X-Goog-Update-Updater"].c_str());
  EXPECT_STREQ("fg", headers["X-Goog-Update-Interactivity"].c_str());
  EXPECT_EQ("", headers["X-Goog-Update-AppId"]);

  headers = BuildUpdateCheckExtraRequestHeaders(config, {}, false);
  EXPECT_STREQ("fake_prodid-30.0", headers["X-Goog-Update-Updater"].c_str());
  EXPECT_STREQ("bg", headers["X-Goog-Update-Interactivity"].c_str());
  EXPECT_EQ("", headers["X-Goog-Update-AppId"]);

  headers = BuildUpdateCheckExtraRequestHeaders(
      config, {"jebgalgnebhfojomionfpkfelancnnkf"}, true);
  EXPECT_STREQ("fake_prodid-30.0", headers["X-Goog-Update-Updater"].c_str());
  EXPECT_STREQ("fg", headers["X-Goog-Update-Interactivity"].c_str());
  EXPECT_STREQ("jebgalgnebhfojomionfpkfelancnnkf",
               headers["X-Goog-Update-AppId"].c_str());

  headers = BuildUpdateCheckExtraRequestHeaders(
      config,
      {"jebgalgnebhfojomionfpkfelancnnkf", "ihfokbkgjpifbbojhneepfflplebdkc"},
      true);
  EXPECT_STREQ("fake_prodid-30.0", headers["X-Goog-Update-Updater"].c_str());
  EXPECT_STREQ("fg", headers["X-Goog-Update-Interactivity"].c_str());
  EXPECT_STREQ(
      "jebgalgnebhfojomionfpkfelancnnkf,ihfokbkgjpifbbojhneepfflplebdkc",
      headers["X-Goog-Update-AppId"].c_str());

  headers = BuildUpdateCheckExtraRequestHeaders(
      config, std::vector<std::string>(40, "jebgalgnebhfojomionfpkfelancnnkf"),
      true);
  EXPECT_STREQ(
      base::JoinString(
          std::vector<std::string>(30, "jebgalgnebhfojomionfpkfelancnnkf"), ",")
          .c_str(),
      headers["X-Goog-Update-AppId"].c_str());
}

}  // namespace update_client
