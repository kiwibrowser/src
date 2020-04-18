// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ONC_ONC_EXPORT_H_
#define COMPONENTS_ONC_ONC_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(ONC_IMPLEMENTATION)
#define ONC_EXPORT __declspec(dllexport)
#else
#define ONC_EXPORT __declspec(dllimport)
#endif  // defined(ONC_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(ONC_IMPLEMENTATION)
#define ONC_EXPORT __attribute__((visibility("default")))
#else
#define ONC_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define ONC_EXPORT
#endif

#endif  // COMPONENTS_ONC_ONC_EXPORT_H_
