// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ==UserScript==
// @name           Document Idle Test
// @namespace      test
// @description    This script tests document-idle
// @include        *
// @run-at         document-idle
// ==/UserScript==

alert(document.readyState);
