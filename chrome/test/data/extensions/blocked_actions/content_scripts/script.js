// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// If the script was really injected at document_start, then document.body will
// be null. If it's not null, then we didn't inject at document_start.
// Store the result in window.localStorage so that it's accessible from the
// main world script context in the browsertest.
window.localStorage.setItem('extResult', document.body ? 'failure' : 'success');
