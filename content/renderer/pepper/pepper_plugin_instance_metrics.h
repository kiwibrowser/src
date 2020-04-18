// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_PEPPER_PLUGIN_INSTANCE_METRICS_H_
#define CONTENT_RENDERER_PEPPER_PEPPER_PLUGIN_INSTANCE_METRICS_H_

namespace content {

// Record size metrics for all Flash instances.
void RecordFlashSizeMetric(int width, int height);

// Records size metrics for Flash instances that are clicked.
void RecordFlashClickSizeMetric(int width, int height);

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_PEPPER_PLUGIN_INSTANCE_METRICS_H_
