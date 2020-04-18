// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Implementation of JSCompiler intrinsics for use when JSCompiler is
// not available.

/**
 * Sets the values of a global expression consisting of a
 * dot-delimited list of identifiers.
 */
const __setGlobal = (name, value) => {
  let parent = window;
  const parts = name.split('.');
  for (let i = 0; i < parts.length; i++) {
    const part = parts[i];
    if (i == parts.length - 1) {
      parent[part] = value;
    } else {
      if (!parent[part]) {
        parent[part] = {};
      }
      parent = parent[part];
    }
  }
};

const goog = {
  provide(name) {
    __setGlobal(name, {});
  },

  require(name) {
    let parent = window;
    name.split('.').forEach(part => {
      parent = parent[part];
    });
    return parent;
  },

  module: {
    declareLegacyNamespace() {},
  },

  forwardDeclare() {},

  scope(body) {
    body.call(window);
  },
};
