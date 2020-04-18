// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_BINDINGS_EXPORT_H_
#define MOJO_PUBLIC_CPP_BINDINGS_BINDINGS_EXPORT_H_

#if defined(COMPONENT_BUILD)

#if defined(WIN32)

#if defined(MOJO_CPP_BINDINGS_IMPLEMENTATION)
#define MOJO_CPP_BINDINGS_EXPORT __declspec(dllexport)
#else
#define MOJO_CPP_BINDINGS_EXPORT __declspec(dllimport)
#endif

#else  // !defined(WIN32)

#if defined(MOJO_CPP_BINDINGS_IMPLEMENTATION)
#define MOJO_CPP_BINDINGS_EXPORT __attribute((visibility("default")))
#else
#define MOJO_CPP_BINDINGS_EXPORT
#endif

#endif  // defined(WIN32)

#else  // !defined(COMPONENT_BUILD)

#define MOJO_CPP_BINDINGS_EXPORT

#endif  // defined(COMPONENT_BUILD)

#endif  // MOJO_PUBLIC_CPP_BINDINGS_BINDINGS_EXPORT_H_
