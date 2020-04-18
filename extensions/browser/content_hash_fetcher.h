// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_CONTENT_HASH_FETCHER_H_
#define EXTENSIONS_BROWSER_CONTENT_HASH_FETCHER_H_

#include <map>
#include <set>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "extensions/browser/content_verifier/content_hash.h"
#include "extensions/common/extension_id.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace base {
class SequencedTaskRunner;
}

namespace net {
class URLFetcher;
}

namespace extensions {
namespace internals {

// This class is responsible for getting signed expected hashes for use in
// extension content verification.
//
// This class takes care of doing the network I/O work to ensure we
// have the contents of verified_contents.json files from the webstore.
//
// Note: This class manages its own lifetime. It deletes itself when
// Start() completes at OnURLFetchComplete().
//
// Note: This class is an internal implementation detail of ContentHash and is
// not be used independently.
// TODO(lazyboy): Consider changing BUILD rules to enforce the above, yet
// keeping the class unit testable.
class ContentHashFetcher : public net::URLFetcherDelegate {
 public:
  // A callback for when fetch is complete.
  // The response contents is passed through std::unique_ptr<std::string>.
  using HashFetcherCallback =
      base::OnceCallback<void(const ContentHash::ExtensionKey&,
                              const ContentHash::FetchParams&,
                              std::unique_ptr<std::string>)>;

  ContentHashFetcher(const ContentHash::ExtensionKey& extension_key,
                     const ContentHash::FetchParams& fetch_params);

  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Note: |this| is deleted once OnURLFetchComplete() completes.
  void Start(HashFetcherCallback hash_fetcher_callback);

 private:
  friend class base::RefCounted<ContentHashFetcher>;

  ~ContentHashFetcher() override;

  ContentHash::ExtensionKey extension_key_;
  ContentHash::FetchParams fetch_params_;

  HashFetcherCallback hash_fetcher_callback_;

  scoped_refptr<base::SequencedTaskRunner> response_task_runner_;

  // Alive when url fetch is ongoing.
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(ContentHashFetcher);
};

}  // namespace internals
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_CONTENT_HASH_FETCHER_H_
