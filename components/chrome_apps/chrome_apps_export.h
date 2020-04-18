// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CHROME_APPS_CHROME_APPS_EXPORT_H_
#define COMPONENTS_CHROME_APPS_CHROME_APPS_EXPORT_H_

// Defines CHROME_APPS_EXPORT so that functionality implemented by the
// CHROME_APPS module can be exported to consumers.

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(CHROME_APPS_IMPLEMENTATION)
#define CHROME_APPS_EXPORT __declspec(dllexport)
#else
#define CHROME_APPS_EXPORT __declspec(dllimport)
#endif  // defined(CHROME_APPS_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(CHROME_APPS_IMPLEMENTATION)
#define CHROME_APPS_EXPORT __attribute__((visibility("default")))
#else
#define CHROME_APPS_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define CHROME_APPS_EXPORT
#endif

#endif  // COMPONENTS_CHROME_APPS_CHROME_APPS_EXPORT_H_
