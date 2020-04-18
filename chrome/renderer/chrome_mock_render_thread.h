// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_CHROME_MOCK_RENDER_THREAD_H_
#define CHROME_RENDERER_CHROME_MOCK_RENDER_THREAD_H_

#include <string>

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "content/public/test/mock_render_thread.h"

// Extends content::MockRenderThread to know about extension messages.
class ChromeMockRenderThread : public content::MockRenderThread {
 public:
  ChromeMockRenderThread();
  ~ChromeMockRenderThread() override;

  // content::RenderThread overrides.
  scoped_refptr<base::SingleThreadTaskRunner> GetIOTaskRunner() override;

  //////////////////////////////////////////////////////////////////////////
  // The following functions are called by the test itself.

  void set_io_task_runner(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);

 protected:
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(ChromeMockRenderThread);
};

#endif  // CHROME_RENDERER_CHROME_MOCK_RENDER_THREAD_H_
