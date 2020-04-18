// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MUS_RENDER_WIDGET_WINDOW_TREE_CLIENT_FACTORY_H_
#define CONTENT_RENDERER_MUS_RENDER_WIDGET_WINDOW_TREE_CLIENT_FACTORY_H_

namespace content {

class ServiceManagerConnection;

void CreateRenderWidgetWindowTreeClientFactory(
    ServiceManagerConnection* connection);

}  // namespace content

#endif  // CONTENT_RENDERER_MUS_RENDER_WIDGET_WINDOW_TREE_CLIENT_FACTORY_H_
