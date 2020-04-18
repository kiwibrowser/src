// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_COMMON_JSON_RESPONSE_FETCHER_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_COMMON_JSON_RESPONSE_FETCHER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

class GURL;

namespace base {
class DictionaryValue;
class Value;
}

namespace content {
class BrowserContext;
}

namespace network {
class SimpleURLLoader;
}

namespace app_list {

// A class that fetches a JSON formatted response from a server and uses a
// sandboxed utility process to parse it to a DictionaryValue.
// TODO(rkc): Add the ability to give control of handling http failures to
// the consumers of this class.
class JSONResponseFetcher {
 public:
  // Callback to pass back the parsed json dictionary returned from the server.
  // Invoked with NULL if there is an error.
  typedef base::Callback<void(std::unique_ptr<base::DictionaryValue>)> Callback;

  JSONResponseFetcher(const Callback& callback,
                      content::BrowserContext* browser_context);
  ~JSONResponseFetcher();

  // Starts to fetch results for the given |query_url|.
  void Start(const GURL& query_url);
  void Stop();

 private:
  // Callbacks for SafeJsonParser.
  void OnJsonParseSuccess(std::unique_ptr<base::Value> parsed_json);
  void OnJsonParseError(const std::string& error);

  // Invoked from SimpleURLLoader after download is complete.
  void OnSimpleLoaderComplete(std::unique_ptr<std::string> response_body);

  Callback callback_;
  content::BrowserContext* browser_context_;
  std::unique_ptr<network::SimpleURLLoader> simple_loader_;
  base::WeakPtrFactory<JSONResponseFetcher> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(JSONResponseFetcher);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_COMMON_JSON_RESPONSE_FETCHER_H_
