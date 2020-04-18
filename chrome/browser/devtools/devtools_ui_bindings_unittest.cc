// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/devtools_ui_bindings.h"
#include "testing/gtest/include/gtest/gtest.h"

class DevToolsUIBindingsTest : public testing::Test {
};

TEST_F(DevToolsUIBindingsTest, SanitizeFrontendURL) {
  std::vector<std::pair<std::string, std::string>> tests = {
      {"random-string", "chrome-devtools://devtools/"},
      {"http://valid.url/but/wrong", "chrome-devtools://devtools/but/wrong"},
      {"chrome-devtools://wrong-domain/", "chrome-devtools://devtools/"},
      {"chrome-devtools://devtools/bundled/devtools.html",
       "chrome-devtools://devtools/bundled/devtools.html"},
      {"chrome-devtools://devtools:1234/bundled/devtools.html#hash",
       "chrome-devtools://devtools/bundled/devtools.html#hash"},
      {"chrome-devtools://devtools/some/random/path",
       "chrome-devtools://devtools/some/random/path"},
      {"chrome-devtools://devtools/bundled/devtools.html?experiments=true",
       "chrome-devtools://devtools/bundled/devtools.html?experiments=true"},
      {"chrome-devtools://devtools/bundled/devtools.html"
       "?some-flag=flag&v8only=true&experiments=false&debugFrontend=a"
       "&another-flag=another-flag&can_dock=false&isSharedWorker=notreally"
       "&remoteFrontend=sure",
       "chrome-devtools://devtools/bundled/devtools.html"
       "?v8only=true&experiments=true&debugFrontend=true"
       "&can_dock=true&isSharedWorker=true&remoteFrontend=true"},
      {"chrome-devtools://devtools/?ws=any-value-is-fine",
       "chrome-devtools://devtools/?ws=any-value-is-fine"},
      {"chrome-devtools://devtools/"
       "?service-backend=ws://localhost:9222/services",
       "chrome-devtools://devtools/"
       "?service-backend=ws://localhost:9222/services"},
      {"chrome-devtools://devtools/?dockSide=undocked",
       "chrome-devtools://devtools/?dockSide=undocked"},
      {"chrome-devtools://devtools/?dockSide=dock-to-bottom",
       "chrome-devtools://devtools/"},
      {"chrome-devtools://devtools/?dockSide=bottom",
       "chrome-devtools://devtools/"},
      {"chrome-devtools://devtools/?remoteBase="
       "http://example.com:1234/remote-base#hash",
       "chrome-devtools://devtools/?remoteBase="
       "https://chrome-devtools-frontend.appspot.com/"
       "serve_file//#hash"},
      {"chrome-devtools://devtools/?ws=1%26evil%3dtrue",
       "chrome-devtools://devtools/?ws=1%26evil%3dtrue"},
      {"chrome-devtools://devtools/?ws=encoded-ok'",
       "chrome-devtools://devtools/?ws=encoded-ok%27"},
      {"chrome-devtools://devtools/?remoteBase="
       "https://chrome-devtools-frontend.appspot.com/some/path/"
       "@123719741873/more/path.html",
       "chrome-devtools://devtools/?remoteBase="
       "https://chrome-devtools-frontend.appspot.com/serve_file/path/"},
      {"chrome-devtools://devtools/?remoteBase="
       "https://chrome-devtools-frontend.appspot.com/serve_file/"
       "@123719741873/inspector.html%3FdebugFrontend%3Dfalse",
       "chrome-devtools://devtools/?remoteBase="
       "https://chrome-devtools-frontend.appspot.com/serve_file/"
       "@123719741873/"},
      {"chrome-devtools://devtools/bundled/inspector.html?"
       "&remoteBase=https://chrome-devtools-frontend.appspot.com/serve_file/"
       "@b4907cc5d602ff470740b2eb6344b517edecb7b9/&can_dock=true",
       "chrome-devtools://devtools/bundled/inspector.html?"
       "remoteBase=https://chrome-devtools-frontend.appspot.com/serve_file/"
       "@b4907cc5d602ff470740b2eb6344b517edecb7b9/&can_dock=true"},
      {"chrome-devtools://devtools/?remoteFrontendUrl="
       "https://chrome-devtools-frontend.appspot.com/serve_rev/"
       "@12345/inspector.html%3FdebugFrontend%3Dfalse",
       "chrome-devtools://devtools/?remoteFrontendUrl="
       "https%3A%2F%2Fchrome-devtools-frontend.appspot.com%2Fserve_rev"
       "%2F%4012345%2Finspector.html%3FdebugFrontend%3Dtrue"},
      {"chrome-devtools://devtools/?remoteFrontendUrl="
       "https://chrome-devtools-frontend.appspot.com/serve_rev/"
       "@12345/inspector.html%22></iframe>something",
       "chrome-devtools://devtools/?remoteFrontendUrl="
       "https%3A%2F%2Fchrome-devtools-frontend.appspot.com%2Fserve_rev"
       "%2F%4012345%2Finspector.html"},
      {"chrome-devtools://devtools/?remoteFrontendUrl="
       "http://domain:1234/path/rev/a/filename.html%3Fparam%3Dvalue#hash",
       "chrome-devtools://devtools/?remoteFrontendUrl="
       "https%3A%2F%2Fchrome-devtools-frontend.appspot.com%2Fserve_rev"
       "%2Frev%2Finspector.html#hash"},
      {"chrome-devtools://devtools/?experiments=whatever&remoteFrontendUrl="
       "https://chrome-devtools-frontend.appspot.com/serve_rev/"
       "@12345/devtools.html%3Fws%3Danyvalue%26experiments%3Dlikely"
       "&unencoded=value&debugFrontend=true",
       "chrome-devtools://devtools/?experiments=true&remoteFrontendUrl="
       "https%3A%2F%2Fchrome-devtools-frontend.appspot.com%2Fserve_rev"
       "%2F%4012345%2Fdevtools.html%3Fws%3Danyvalue%26experiments%3Dtrue"
       "&debugFrontend=true"},
      {"chrome-devtools://devtools/?remoteFrontendUrl="
       "https://chrome-devtools-frontend.appspot.com/serve_rev/"
       "@12345/inspector.html%23%27",
       "chrome-devtools://devtools/?remoteFrontendUrl="
       "https%3A%2F%2Fchrome-devtools-frontend.appspot.com%2Fserve_rev"
       "%2F%4012345%2Finspector.html"},
  };

  for (const auto& pair : tests) {
    GURL url = GURL(pair.first);
    url = DevToolsUIBindings::SanitizeFrontendURL(url);
    EXPECT_EQ(pair.second, url.spec());
  }
}
