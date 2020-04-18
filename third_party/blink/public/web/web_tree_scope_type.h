// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_TREE_SCOPE_TYPE_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_TREE_SCOPE_TYPE_H_

namespace blink {

// Indicates if an item is in a document tree or in a shadow tree, per the
// Shadow DOM spec: https://w3c.github.io/webcomponents/spec/shadow/
enum class WebTreeScopeType {
  kDocument,
  kShadow,
  kLast = kShadow,
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_TREE_SCOPE_TYPE_H_
