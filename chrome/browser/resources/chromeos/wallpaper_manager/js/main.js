// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Global wallpaperManager reference useful for poking at from the console.
 */
var wallpaperManager;

function init() {
  WallpaperManager.initStrings(function() {
    wallpaperManager = new WallpaperManager(document.body);
  });
}

document.addEventListener('DOMContentLoaded', init);
