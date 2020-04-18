/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_STORAGE_AREA_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_STORAGE_AREA_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {

class WebURL;

class WebStorageArea {
 public:
  virtual ~WebStorageArea() = default;

  enum Result {
    kResultOK = 0,
    kResultBlockedByQuota,
    kResultLast = kResultBlockedByQuota
  };

  // The number of key/value pairs in the storage area.
  virtual unsigned length() = 0;

  // Get a value for a specific key. Valid key indices are 0 through
  // length() - 1.  Indexes may change on any Set/RemoveItem call. Will return
  // null if the index provided is out of range.
  virtual WebString Key(unsigned index) = 0;

  // Get the value that corresponds to a specific key. This returns null if
  // there is no entry for that key.
  virtual WebString GetItem(const WebString& key) = 0;

  // Set the value that corresponds to a specific key. Result will either be
  // ResultOK or some particular error. The value is NOT set when there's an
  // error.  |page_url| is the url that should be used if a storage event fires.
  virtual void SetItem(const WebString& key,
                       const WebString& new_value,
                       const WebURL& page_url,
                       Result& result) {
    WebString unused;
    SetItem(key, new_value, page_url, result, unused);
  }

  // Remove the value associated with a particular key. |page_url| is the url
  // that should be used if a storage event fires.
  virtual void RemoveItem(const WebString& key, const WebURL& page_url) {
    WebString unused;
    RemoveItem(key, page_url, unused);
  }

  // Clear all key/value pairs. |page_url| is the url that should be used if a
  // storage event fires.
  virtual void Clear(const WebURL& page_url) {
    bool unused;
    Clear(page_url, unused);
  }

  // DEPRECATED - being replaced by the async variants above which do not return
  // old_values or block until completion.
  virtual void SetItem(const WebString& key,
                       const WebString& new_value,
                       const WebURL&,
                       Result&,
                       WebString& old_value) {}
  virtual void RemoveItem(const WebString& key,
                          const WebURL& page_url,
                          WebString& old_value) {}
  virtual void Clear(const WebURL& page_url, bool& something_cleared) {}
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_STORAGE_AREA_H_
