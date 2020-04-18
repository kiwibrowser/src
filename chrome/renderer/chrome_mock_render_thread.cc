// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/chrome_mock_render_thread.h"

#include "base/single_thread_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"

ChromeMockRenderThread::ChromeMockRenderThread() {
}

ChromeMockRenderThread::~ChromeMockRenderThread() {
}

scoped_refptr<base::SingleThreadTaskRunner>
ChromeMockRenderThread::GetIOTaskRunner() {
  return io_task_runner_;
}

void ChromeMockRenderThread::set_io_task_runner(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
  io_task_runner_ = task_runner;
}
