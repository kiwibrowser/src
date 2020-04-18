// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TOUCH_HUD_ASH_TOUCH_HUD_EXPORT_H_
#define ASH_TOUCH_HUD_ASH_TOUCH_HUD_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(ASH_TOUCH_HUD_IMPLEMENTATION)
#define ASH_TOUCH_HUD_EXPORT __declspec(dllexport)
#else
#define ASH_TOUCH_HUD_EXPORT __declspec(dllimport)
#endif  // defined(ASH_TOUCH_HUD_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(ASH_TOUCH_HUD_IMPLEMENTATION)
#define ASH_TOUCH_HUD_EXPORT __attribute__((visibility("default")))
#else
#define ASH_TOUCH_HUD_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define ASH_TOUCH_HUD_EXPORT
#endif

#endif  // ASH_TOUCH_HUD_ASH_TOUCH_HUD_EXPORT_H_
