// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ASH_EXPORT_H_
#define ASH_ASH_EXPORT_H_

// Defines ASH_EXPORT so that functionality implemented by the Ash module can
// be exported to consumers.

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(ASH_IMPLEMENTATION)
#define ASH_EXPORT __declspec(dllexport)
#else
#define ASH_EXPORT __declspec(dllimport)
#endif  // defined(ASH_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(ASH_IMPLEMENTATION)
#define ASH_EXPORT __attribute__((visibility("default")))
#else
#define ASH_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define ASH_EXPORT
#endif

#endif  // ASH_ASH_EXPORT_H_
