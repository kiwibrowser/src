// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/blob_reader.h"

#include <limits>
#include <utility>

#include "base/format_macros.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

BlobReader::BlobReader(content::BrowserContext* browser_context,
                       const std::string& blob_uuid,
                       BlobReadCallback callback)
    : callback_(callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  GURL blob_url = GURL(std::string("blob:uuid/") + blob_uuid);
  DCHECK(blob_url.is_valid());

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("blob_reader", R"(
        semantics {
          sender: "BlobReader"
          description:
            "Blobs are used for a variety of use cases, and are basically "
            "immutable blocks of data. See https://chromium.googlesource.com/"
            "chromium/src/+/master/storage/browser/blob/README.md for an "
            "explanation of blobs and their implementation in Chrome. These "
            "can be created by scripts in a website, web platform features, or "
            "internally in the browser."
          trigger:
            "Request for reading the contents of a blob."
          data:
            "A reference to a Blob, File, or CacheStorage entry created from "
            "script, a web platform feature, or browser internals."
          destination: LOCAL
        }
        policy {
          cookies_allowed: NO
          setting: "This feature cannot be disabled by settings."
          policy_exception_justification:
            "Not implemented. This is a local data fetch request and has no "
            "network activity."
        })");
  fetcher_ = net::URLFetcher::Create(blob_url, net::URLFetcher::GET, this,
                                     traffic_annotation);
  fetcher_->SetRequestContext(
      content::BrowserContext::GetDefaultStoragePartition(browser_context)
          ->GetURLRequestContext());
}

BlobReader::~BlobReader() { DCHECK_CURRENTLY_ON(content::BrowserThread::UI); }

void BlobReader::SetByteRange(int64_t offset, int64_t length) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK_GE(offset, 0);
  CHECK_GT(length, 0);
  CHECK_LE(offset, std::numeric_limits<int64_t>::max() - length);

  net::HttpRequestHeaders headers;
  headers.SetHeader(
      net::HttpRequestHeaders::kRange,
      base::StringPrintf("bytes=%" PRId64 "-%" PRId64, offset,
                         offset + length - 1));
  fetcher_->SetExtraRequestHeaders(headers.ToString());
}

void BlobReader::Start() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  fetcher_->Start();
}

// Overridden from net::URLFetcherDelegate.
void BlobReader::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::unique_ptr<std::string> response(new std::string);
  int64_t first = 0, last = 0, length = 0;
  source->GetResponseAsString(response.get());
  if (source->GetResponseHeaders())
    source->GetResponseHeaders()->GetContentRangeFor206(&first, &last, &length);
  callback_.Run(std::move(response), length);

  delete this;
}
