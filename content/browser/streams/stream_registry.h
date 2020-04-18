// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_STREAMS_STREAM_REGISTRY_H_
#define CONTENT_BROWSER_STREAMS_STREAM_REGISTRY_H_

#include <stddef.h>

#include <map>
#include <set>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "content/browser/streams/stream_register_observer.h"
#include "content/common/content_export.h"
#include "url/gurl.h"

namespace content {

class Stream;

// Maintains a mapping of blob: URLs to active streams.
class CONTENT_EXPORT StreamRegistry {
 public:
  StreamRegistry();
  virtual ~StreamRegistry();

  // Registers a stream, and sets its URL.
  void RegisterStream(Stream* stream);

  // Clones a stream.  Returns true on success, or false if |src_url| doesn't
  // exist.
  bool CloneStream(const GURL& url, const GURL& src_url);

  void UnregisterStream(const GURL& url);

  // Called by Stream instances to request increase of memory usage. If the
  // total memory usage for this registry is going to exceed the limit,
  // returns false. Otherwise, updates |total_memory_usage_| and returns true.
  //
  // |current_size| is the up-to-date size of ByteStream of the Stream instance
  // and |increase| must be the amount of data going to be added to the Stream
  // instance.
  bool UpdateMemoryUsage(const GURL& url, size_t current_size, size_t increase);

  // Gets the stream associated with |url|.  Returns NULL if there is no such
  // stream.
  scoped_refptr<Stream> GetStream(const GURL& url);

  void set_max_memory_usage_for_testing(size_t size) {
    max_memory_usage_ = size;
  }

  void SetRegisterObserver(const GURL& url, StreamRegisterObserver* observer);
  void RemoveRegisterObserver(const GURL& url);

  // If the reader is aborted before the stream is registered, call this method
  // to reduce the memory consumption. After this method is called,
  // RegisterStream doesn't register the stream of the URL.
  void AbortPendingStream(const GURL& url);

 private:
  typedef std::map<GURL, scoped_refptr<Stream> > StreamMap;

  StreamMap streams_;
  std::map<GURL, StreamRegisterObserver*> register_observers_;
  std::set<GURL> reader_aborted_urls_;

  size_t total_memory_usage_;

  // Maximum amount of memory allowed to use for Stream instances registered
  // with this registry.
  size_t max_memory_usage_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(StreamRegistry);
};

}  // namespace content

#endif  // CONTENT_BROWSER_STREAMS_STREAM_REGISTRY_H_
