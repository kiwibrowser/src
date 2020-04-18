// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CDM_CDM_MANAGER_EXPORT_H_
#define MEDIA_CDM_CDM_MANAGER_EXPORT_H_

// Define CDM_MANAGER_EXPORT so that functionality implemented by the
// cdm_manager component can be exported to consumers.

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(CDM_MANAGER_IMPLEMENTATION)
#define CDM_MANAGER_EXPORT __declspec(dllexport)
#else
#define CDM_MANAGER_EXPORT __declspec(dllimport)
#endif  // defined(CDM_MANAGER_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(CDM_MANAGER_IMPLEMENTATION)
#define CDM_MANAGER_EXPORT __attribute__((visibility("default")))
#else
#define CDM_MANAGER_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define CDM_MANAGER_EXPORT
#endif

#endif  // MEDIA_BASE_CDM_MANAGER_EXPORT_H_
