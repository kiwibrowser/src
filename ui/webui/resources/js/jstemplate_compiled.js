// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file serves as a proxy to bring the included js file from /third_party
// into its correct location under the resources directory tree, whence it is
// delivered via a chrome://resources URL.  See ../webui_resources.grd.

// Note: this <include> is not behind a single-line comment because the first
// line of the file is source code (so the first line would be skipped) instead
// of a licence header.
// clang-format off
<include src="../../../../third_party/jstemplate/jstemplate_compiled.js">
