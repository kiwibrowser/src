// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ==UserScript==
// @name           Document Start Test
// @namespace      test
// @description    This script tests document-start
// @include        *
// @run-at         document-start
// ==/UserScript==

alert(document.readyState);
