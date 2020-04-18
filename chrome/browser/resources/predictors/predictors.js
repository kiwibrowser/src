// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// <include src="autocomplete_action_predictor.js">
// <include src="resource_prefetch_predictor.js">

if (cr.isWindows)
  document.documentElement.setAttribute('os', 'win');

cr.ui.decorate('tabbox', cr.ui.TabBox);
