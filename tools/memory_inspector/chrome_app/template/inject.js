// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Script injected into the Memory Inspector webpage which disables native
 * tracing.
 */
var script = document.createElement('script');
script.textContent = 'window.DISABLE_NATIVE_TRACING = true;';
document.head.appendChild(script);
