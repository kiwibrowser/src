// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_X_UI_BASE_X_EXPORT_H_
#define UI_BASE_X_UI_BASE_X_EXPORT_H_

// Defines UI_BASE_X_EXPORT so that functionality implemented by the UI module
// can be exported to consumers.

#if defined(COMPONENT_BUILD)

#if defined(WIN32)

#if defined(UI_BASE_X_IMPLEMENTATION)
#define UI_BASE_X_EXPORT __declspec(dllexport)
#else
#define UI_BASE_X_EXPORT __declspec(dllimport)
#endif

#else  // !defined(WIN32)

#if defined(UI_BASE_X_IMPLEMENTATION)
#define UI_BASE_X_EXPORT __attribute__((visibility("default")))
#else
#define UI_BASE_X_EXPORT
#endif

#endif

#else  // !defined(COMPONENT_BUILD)

#define UI_BASE_X_EXPORT

#endif

#endif  // UI_BASE_X_UI_BASE_X_EXPORT_H_
