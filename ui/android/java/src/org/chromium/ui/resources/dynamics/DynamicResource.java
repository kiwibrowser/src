// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.resources.dynamics;

import org.chromium.ui.resources.Resource;

/**
 * A representation of a dynamic resource.  The contents of the resource might change from frame to
 * frame.
 */
public interface DynamicResource extends Resource {
    /**
     * Note that this is called for every access to the resource during a frame.  If a resource is
     * dirty, it should not be dirty again during the same looper call.
     *
     * TODO(dtrainor): Add checks so that a dynamic resource **can't** be built more than once each
     * frame.
     *
     * @return Whether or not this resource is dirty and the CC component should be rebuilt.
     */
    boolean isDirty();
}