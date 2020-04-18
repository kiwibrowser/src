// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_CLIENT_CONTEXT_H_
#define REMOTING_CLIENT_CLIENT_CONTEXT_H_

#include <string>

#include "base/macros.h"
#include "base/threading/thread.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace remoting {

// A class that manages threads and running context for the chromoting client
// process.
class ClientContext {
 public:
  // |main_task_runner| is the task runner for the main plugin thread
  // that is used for all PPAPI calls, e.g. network and graphics.
  ClientContext(
      const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner);
  virtual ~ClientContext();

  void Start();
  void Stop();

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner() const;
  scoped_refptr<base::SingleThreadTaskRunner> decode_task_runner() const;
  scoped_refptr<base::SingleThreadTaskRunner> audio_decode_task_runner() const;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  // A thread that handles all video decode operations.
  base::Thread decode_thread_;

  // A thread that handles all audio decode operations.
  base::Thread audio_decode_thread_;

  DISALLOW_COPY_AND_ASSIGN(ClientContext);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_CLIENT_CONTEXT_H_
