// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_LIBGTKUI_EXPORT_H_
#define CHROME_BROWSER_UI_LIBGTKUI_LIBGTKUI_EXPORT_H_

// Defines LIBGTKUI_EXPORT so that functionality implemented by our limited
// gtk2 module can be exported to consumers.

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#error "LIBGTKUI does not build on Windows."

#else  // defined(WIN32)
#if defined(LIBGTKUI_IMPLEMENTATION)
#define LIBGTKUI_EXPORT __attribute__((visibility("default")))
#else
#define LIBGTKUI_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define LIBGTKUI_EXPORT
#endif

#endif  // CHROME_BROWSER_UI_LIBGTKUI_LIBGTKUI_EXPORT_H_
