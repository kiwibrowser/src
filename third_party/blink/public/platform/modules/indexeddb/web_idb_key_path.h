/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_INDEXEDDB_WEB_IDB_KEY_PATH_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_INDEXEDDB_WEB_IDB_KEY_PATH_H_

#include "third_party/blink/public/platform/modules/indexeddb/web_idb_types.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"

namespace blink {

class WebIDBKeyPath {
 public:
  // FIXME: Update callers use constructors directly, and remove these.
  static WebIDBKeyPath Create(const WebString& string) {
    return WebIDBKeyPath(string);
  }
  static WebIDBKeyPath Create(const WebVector<WebString>& array) {
    return WebIDBKeyPath(array);
  }
  static WebIDBKeyPath CreateNull() { return WebIDBKeyPath(); }

  WebIDBKeyPath() : type_(kWebIDBKeyPathTypeNull) {}

  explicit WebIDBKeyPath(const WebString& string)
      : type_(kWebIDBKeyPathTypeString), string_(string) {}

  explicit WebIDBKeyPath(const WebVector<WebString>& array)
      : type_(kWebIDBKeyPathTypeArray), array_(array) {}

  WebIDBKeyPath(const WebIDBKeyPath& key_path) = default;

  ~WebIDBKeyPath() = default;

  WebIDBKeyPath& operator=(const WebIDBKeyPath& key_path) = default;

  WebIDBKeyPathType KeyPathType() const { return type_; }

  // Only valid for ArrayType.
  const WebVector<WebString>& Array() const { return array_; }

  // Only valid for StringType.
  const WebString& String() const { return string_; }

 private:
  WebIDBKeyPathType type_;
  WebVector<WebString> array_;
  WebString string_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_INDEXEDDB_WEB_IDB_KEY_PATH_H_
