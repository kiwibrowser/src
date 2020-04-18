// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_APP_LIST_PRESENTER_APP_LIST_PRESENTER_EXPORT_H_
#define ASH_APP_LIST_PRESENTER_APP_LIST_PRESENTER_EXPORT_H_

// Defines APP_LIST_PRESENTER_EXPORT so that functionality implemented by the
// app_list presenter module can be exported to consumers.

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(APP_LIST_PRESENTER_IMPLEMENTATION)
#define APP_LIST_PRESENTER_EXPORT __declspec(dllexport)
#else
#define APP_LIST_PRESENTER_EXPORT __declspec(dllimport)
#endif  // defined(APP_LIST_PRESENTER_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(APP_LIST_PRESENTER_IMPLEMENTATION)
#define APP_LIST_PRESENTER_EXPORT __attribute__((visibility("default")))
#else
#define APP_LIST_PRESENTER_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define APP_LIST_PRESENTER_EXPORT
#endif

#endif  // ASH_APP_LIST_PRESENTER_APP_LIST_PRESENTER_EXPORT_H_
