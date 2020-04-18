// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_STORAGE_BROWSER_EXPORT_H__
#define STORAGE_BROWSER_STORAGE_BROWSER_EXPORT_H__

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(STORAGE_BROWSER_IMPLEMENTATION)
#define STORAGE_EXPORT __declspec(dllexport)
#else
#define STORAGE_EXPORT __declspec(dllimport)
#endif  // defined(STORAGE_BROWSER_IMPLEMENTATION)

#else // defined(WIN32)
#if defined(STORAGE_BROWSER_IMPLEMENTATION)
#define STORAGE_EXPORT __attribute__((visibility("default")))
#else
#define STORAGE_EXPORT
#endif
#endif

#else // defined(COMPONENT_BUILD)
#define STORAGE_EXPORT
#endif

#endif  // STORAGE_BROWSER_STORAGE_BROWSER_EXPORT_H__
