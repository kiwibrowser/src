// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SQL_SQL_EXPORT_H_
#define SQL_SQL_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(SQL_IMPLEMENTATION)
#define SQL_EXPORT __declspec(dllexport)
#else
#define SQL_EXPORT __declspec(dllimport)
#endif  // defined(SQL_IMPLEMENTATION)

#else // defined(WIN32)
#if defined(SQL_IMPLEMENTATION)
#define SQL_EXPORT __attribute__((visibility("default")))
#else
#define SQL_EXPORT
#endif
#endif

#else // defined(COMPONENT_BUILD)
#define SQL_EXPORT
#endif

#endif  // SQL_SQL_EXPORT_H_
