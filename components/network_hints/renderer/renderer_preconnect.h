// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A RendererPreconnect instance is maintained for each RenderThread.
// URL strings are typically added to the embedded queue during rendering.
// The first addition to the queue (transitioning from empty to having
// some names) causes a processing task to be added to the Renderer Thread.
// The processing task gathers all buffered URLs, and send them via IPC
// to the browser.
// This class counts repeated requests for the same URL and requests that
// number of connections be preconnected. If multiple requests are sent
// separately the net stack will just keep re-using the first pending
// connection so we allow for the time between the parsing of the tags and
// when the task is scheduled to accumulate multiple requests.

#ifndef COMPONENTS_NETWORK_HINTS_RENDERER_RENDERER_PRECONNECT_H_
#define COMPONENTS_NETWORK_HINTS_RENDERER_RENDERER_PRECONNECT_H_

#include "base/macros.h"
#include "url/gurl.h"

namespace network_hints {

// An internal interface to the network_hints component for efficiently sending
// preconnect requests to the net stack.
class RendererPreconnect {
 public:
  RendererPreconnect();
  ~RendererPreconnect();

  // Submit a preconnect request for a single connection.
  void Preconnect(const GURL& url, bool allow_credentials);

 private:

  DISALLOW_COPY_AND_ASSIGN(RendererPreconnect);
};  // class RendererPreconnect

}  // namespace network_hints

#endif  // COMPONENTS_NETWORK_HINTS_RENDERER_RENDERER_PRECONNECT_H_
