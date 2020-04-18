// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_WIN_MF_INITIALIZER_EXPORT_H_
#define MEDIA_BASE_WIN_MF_INITIALIZER_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(MF_INITIALIZER_IMPLEMENTATION)
#define MF_INITIALIZER_EXPORT __declspec(dllexport)
#else
#define MF_INITIALIZER_EXPORT __declspec(dllimport)
#endif  // defined(MF_INITIALIZER_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(MF_INITIALIZER_IMPLEMENTATION)
#define MF_INITIALIZER_EXPORT __attribute__((visibility("default")))
#else
#define MF_INITIALIZER_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define MF_INITIALIZER_EXPORT
#endif

#endif  // MEDIA_BASE_WIN_MF_INITIALIZER_EXPORT_H_
