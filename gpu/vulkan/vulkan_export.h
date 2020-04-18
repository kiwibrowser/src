// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_VULKAN_VULKAN_EXPORT_H_
#define GPU_VULKAN_VULKAN_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(VULKAN_IMPLEMENTATION)
#define VULKAN_EXPORT __declspec(dllexport)
#else
#define VULKAN_EXPORT __declspec(dllimport)
#endif  // defined(GL_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(VULKAN_IMPLEMENTATION)
#define VULKAN_EXPORT __attribute__((visibility("default")))
#else
#define VULKAN_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define VULKAN_EXPORT
#endif

#endif  // GPU_VULKAN_VULKAN_EXPORT_H_
