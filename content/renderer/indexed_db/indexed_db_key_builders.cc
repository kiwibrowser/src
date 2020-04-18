// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/indexed_db/indexed_db_key_builders.h"

#include <stddef.h>

#include <algorithm>
#include <string>
#include <vector>

#include "base/logging.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_key.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_key_path.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_key_range.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_types.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"

using blink::WebIDBKey;
using blink::WebIDBKeyRange;
using blink::WebIDBKeyView;
using blink::kWebIDBKeyTypeArray;
using blink::kWebIDBKeyTypeBinary;
using blink::kWebIDBKeyTypeDate;
using blink::kWebIDBKeyTypeInvalid;
using blink::kWebIDBKeyTypeMin;
using blink::kWebIDBKeyTypeNull;
using blink::kWebIDBKeyTypeNumber;
using blink::kWebIDBKeyTypeString;
using blink::WebVector;
using blink::WebString;

namespace {

content::IndexedDBKey::KeyArray CopyKeyArray(blink::WebIDBKeyArrayView array) {
  content::IndexedDBKey::KeyArray result;
  const size_t array_size = array.size();
  result.reserve(array_size);
  for (size_t i = 0; i < array_size; ++i)
    result.emplace_back(content::IndexedDBKeyBuilder::Build(array[i]));
  return result;
}

std::vector<base::string16> CopyArray(const WebVector<WebString>& array) {
  std::vector<base::string16> result;
  result.reserve(array.size());
  for (const WebString& element : array)
    result.emplace_back(element.Utf16());
  return result;
}

}  // anonymous namespace

namespace content {

// static
IndexedDBKey IndexedDBKeyBuilder::Build(blink::WebIDBKeyView key) {
  switch (key.KeyType()) {
    case kWebIDBKeyTypeArray:
      return IndexedDBKey(CopyKeyArray(key.ArrayView()));
    case kWebIDBKeyTypeBinary: {
      const blink::WebData data = key.Binary();
      std::string key_string;
      key_string.reserve(data.size());

      data.ForEachSegment([&key_string](const char* segment,
                                        size_t segment_size,
                                        size_t segment_offset) {
        key_string.append(segment, segment_size);
        return true;
      });
      return IndexedDBKey(key_string);
    }
    case kWebIDBKeyTypeString:
      return IndexedDBKey(key.String().Utf16());
    case kWebIDBKeyTypeDate:
      return IndexedDBKey(key.Date(), kWebIDBKeyTypeDate);
    case kWebIDBKeyTypeNumber:
      return IndexedDBKey(key.Number(), kWebIDBKeyTypeNumber);
    case kWebIDBKeyTypeNull:
    case kWebIDBKeyTypeInvalid:
      return IndexedDBKey(key.KeyType());
    case kWebIDBKeyTypeMin:
    default:
      NOTREACHED();
      return IndexedDBKey();
  }
}

// static
WebIDBKey WebIDBKeyBuilder::Build(const IndexedDBKey& key) {
  switch (key.type()) {
    case kWebIDBKeyTypeArray: {
      const IndexedDBKey::KeyArray& array = key.array();
      WebVector<WebIDBKey> web_idb_keys;
      web_idb_keys.reserve(array.size());
      for (const IndexedDBKey& array_element : array)
        web_idb_keys.emplace_back(Build(array_element));
      return WebIDBKey::CreateArray(std::move(web_idb_keys));
    }
    case kWebIDBKeyTypeBinary:
      return WebIDBKey::CreateBinary(key.binary());
    case kWebIDBKeyTypeString:
      return WebIDBKey::CreateString(WebString::FromUTF16(key.string()));
    case kWebIDBKeyTypeDate:
      return WebIDBKey::CreateDate(key.date());
    case kWebIDBKeyTypeNumber:
      return WebIDBKey::CreateNumber(key.number());
    case kWebIDBKeyTypeInvalid:
      return WebIDBKey::CreateInvalid();
    case kWebIDBKeyTypeNull:
      return WebIDBKey::CreateNull();
    case kWebIDBKeyTypeMin:
    default:
      NOTREACHED();
      return WebIDBKey::CreateInvalid();
  }
}

// static
IndexedDBKeyRange IndexedDBKeyRangeBuilder::Build(
    const WebIDBKeyRange& key_range) {
  return IndexedDBKeyRange(IndexedDBKeyBuilder::Build(key_range.Lower()),
                           IndexedDBKeyBuilder::Build(key_range.Upper()),
                           key_range.LowerOpen(), key_range.UpperOpen());
}

// static
IndexedDBKeyRange IndexedDBKeyRangeBuilder::Build(WebIDBKeyView key) {
  return IndexedDBKeyRange(IndexedDBKeyBuilder::Build(key),
                           IndexedDBKeyBuilder::Build(key),
                           false /* lower_open */, false /* upper_open */);
}

// static
WebIDBKeyRange WebIDBKeyRangeBuilder::Build(
    const IndexedDBKeyRange& key_range) {
  return WebIDBKeyRange(WebIDBKeyBuilder::Build(key_range.lower()),
                        WebIDBKeyBuilder::Build(key_range.upper()),
                        key_range.lower_open(), key_range.upper_open());
}

// static
IndexedDBKeyPath IndexedDBKeyPathBuilder::Build(
    const blink::WebIDBKeyPath& key_path) {
  switch (key_path.KeyPathType()) {
    case blink::kWebIDBKeyPathTypeString:
      return IndexedDBKeyPath(key_path.String().Utf16());
    case blink::kWebIDBKeyPathTypeArray:
      return IndexedDBKeyPath(CopyArray(key_path.Array()));
    case blink::kWebIDBKeyPathTypeNull:
      return IndexedDBKeyPath();
    default:
      NOTREACHED();
      return IndexedDBKeyPath();
  }
}

// static
blink::WebIDBKeyPath WebIDBKeyPathBuilder::Build(
    const IndexedDBKeyPath& key_path) {
  switch (key_path.type()) {
    case blink::kWebIDBKeyPathTypeString:
      return blink::WebIDBKeyPath::Create(
          WebString::FromUTF16(key_path.string()));
    case blink::kWebIDBKeyPathTypeArray: {
      WebVector<WebString> key_path_vector(key_path.array().size());
      std::transform(key_path.array().begin(), key_path.array().end(),
                     key_path_vector.begin(),
                     [](const typename base::string16& s) {
                       return WebString::FromUTF16(s);
                     });
      return blink::WebIDBKeyPath::Create(key_path_vector);
    }
    case blink::kWebIDBKeyPathTypeNull:
      return blink::WebIDBKeyPath::CreateNull();
    default:
      NOTREACHED();
      return blink::WebIDBKeyPath::CreateNull();
  }
}

}  // namespace content
